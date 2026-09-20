#include "ComputeFeatureNeighborCAxisMisalignments.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

ComputeFeatureNeighborCAxisMisalignments::ComputeFeatureNeighborCAxisMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                   ComputeFeatureNeighborCAxisMisalignmentsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureNeighborCAxisMisalignments::~ComputeFeatureNeighborCAxisMisalignments() noexcept = default;

Result<> ComputeFeatureNeighborCAxisMisalignments::operator()()
{
  const auto& crystalStructuresStoreRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStoreRef.getNumberOfTuples();
  std::vector<uint32> crystalStructuresCache(numCrystalStructures);
  if(Result<> ioResult = crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.data(), numCrystalStructures)); ioResult.invalid())
  {
    return ConvertResult(std::move(ioResult));
  }

  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize phaseIdx = 1; phaseIdx < numCrystalStructures; ++phaseIdx)
  {
    const auto crystalStructureType = crystalStructuresCache[phaseIdx];
    const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
    allPhasesHexagonal = allPhasesHexagonal && isHex;
    noPhasesHexagonal = noPhasesHexagonal && !isHex;
  }

  if(noPhasesHexagonal)
  {
    return MakeErrorResult(-1562, "No phases that have a crystal symmetry of Hexagonal (6/mmm or 6/m) were found.");
  }

  Result<> result;
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back({-1563, "Non Hexagonal phases were found. All calculations for non Hexagonal phases will be skipped and a NaN value inserted."});
  }

  // The neighbor traversal accesses feature data in random order.
  const auto& featurePhasesStoreRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath).getDataStoreRef();
  const usize totalFeatures = featurePhasesStoreRef.getNumberOfTuples();
  std::vector<int32> featurePhasesCache(totalFeatures);
  if(Result<> ioResult = featurePhasesStoreRef.copyIntoBuffer(0, nonstd::span<int32>(featurePhasesCache.data(), totalFeatures)); ioResult.invalid())
  {
    return MergeResults(std::move(result), std::move(ioResult));
  }

  const auto& avgQuatsStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath).getDataStoreRef();
  const usize numQuatComps = avgQuatsStoreRef.getNumberOfComponents();
  const usize quatSize = totalFeatures * numQuatComps;
  std::vector<float32> featureAvgQuat(quatSize);
  if(Result<> ioResult = avgQuatsStoreRef.copyIntoBuffer(0, nonstd::span<float32>(featureAvgQuat.data(), quatSize)); ioResult.invalid())
  {
    return MergeResults(std::move(result), std::move(ioResult));
  }

  auto& neighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborListArrayPath);
  auto& cAxisMisalignmentList = m_DataStructure.getDataRefAs<NeighborList<float32>>(m_InputValues->CAxisMisalignmentListArrayName);

  Float32Array* avgCAxisMisalignmentPtr = nullptr;
  std::vector<float32> avgCAxisBuf;
  if(m_InputValues->FindAvgMisals)
  {
    avgCAxisMisalignmentPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgCAxisMisalignmentsArrayName);
    avgCAxisBuf.resize(totalFeatures, 0.0f);
  }

  std::vector<std::vector<float32>> misalignmentLists(totalFeatures);

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const int32 currentPhaseIdx = featurePhasesCache[featureIdx];
    if(currentPhaseIdx < 0 || static_cast<usize>(currentPhaseIdx) >= numCrystalStructures)
    {
      return MakeErrorResult(-1564, fmt::format("Feature Phases array '{}' has value {} at feature index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in [0, {}).",
                                                m_InputValues->FeaturePhasesArrayPath.toString(), currentPhaseIdx, featureIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                                numCrystalStructures, numCrystalStructures));
    }
    const uint32 currentCrystalStructure = crystalStructuresCache[currentPhaseIdx];

    const usize quatTupleIndex1 = featureIdx * numQuatComps;
    ebsdlib::OrientationMatrixDType oMatrix1 =
        ebsdlib::QuaternionDType(featureAvgQuat[quatTupleIndex1], featureAvgQuat[quatTupleIndex1 + 1], featureAvgQuat[quatTupleIndex1 + 2], featureAvgQuat[quatTupleIndex1 + 3]).toOrientationMatrix();

    // The transposed matrix maps crystal [001] into the sample frame.
    Eigen::Vector3d c1 = oMatrix1.transpose() * cAxis;
    c1.normalize();

    const NeighborList<int>::VectorType& currentNeighborList = neighborList[featureIdx];
    auto& currentMisalignmentList = misalignmentLists[featureIdx];
    currentMisalignmentList.resize(currentNeighborList.size(), -1.0);
    // The denominator counts only same-phase hexagonal neighbors.
    usize hexNeighborListSize = currentNeighborList.size();
    for(usize neighborListIdx = 0; neighborListIdx < currentNeighborList.size(); neighborListIdx++)
    {
      const int32 neighborFeatureIdx = currentNeighborList[neighborListIdx];
      const int32 neighborFeaturePhaseIdx = featurePhasesCache[neighborFeatureIdx];
      if(neighborFeaturePhaseIdx < 0 || static_cast<usize>(neighborFeaturePhaseIdx) >= numCrystalStructures)
      {
        return MakeErrorResult(-1564, fmt::format("Feature Phases array '{}' has value {} at feature index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in [0, {}).",
                                                  m_InputValues->FeaturePhasesArrayPath.toString(), neighborFeaturePhaseIdx, neighborFeatureIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                                  numCrystalStructures, numCrystalStructures));
      }
      const uint32 neighborCrystalStructure = crystalStructuresCache[neighborFeaturePhaseIdx];

      if(currentCrystalStructure == neighborCrystalStructure &&
         (currentCrystalStructure == ebsdlib::CrystalStructure::Hexagonal_High || currentCrystalStructure == ebsdlib::CrystalStructure::Hexagonal_Low))
      {
        const usize quatTupleIndex2 = neighborFeatureIdx * numQuatComps;
        ebsdlib::OrientationMatrixDType oMatrix2 =
            ebsdlib::QuaternionDType(featureAvgQuat[quatTupleIndex2], featureAvgQuat[quatTupleIndex2 + 1], featureAvgQuat[quatTupleIndex2 + 2], featureAvgQuat[quatTupleIndex2 + 3])
                .toOrientationMatrix();

        Eigen::Vector3d c2 = oMatrix2.transpose() * cAxis;
        c2.normalize();

        float64 w = ImageRotationUtilities::CosBetweenVectors(c1, c2);
        w = std::clamp(w, -1.0, 1.0);
        w = std::acos(w);
        if(w > Constants::k_PiOver2D)
        {
          w = Constants::k_PiD - w;
        }

        currentMisalignmentList[neighborListIdx] = static_cast<float32>(w * Constants::k_180OverPiD);

        if(m_InputValues->FindAvgMisals)
        {
          avgCAxisBuf[featureIdx] += currentMisalignmentList[neighborListIdx];
        }
      }
      else
      {
        if(m_InputValues->FindAvgMisals)
        {
          hexNeighborListSize--;
        }
        currentMisalignmentList[neighborListIdx] = std::nanf("");
      }
    }

    if(m_InputValues->FindAvgMisals)
    {
      if(hexNeighborListSize > 0)
      {
        avgCAxisBuf[featureIdx] = static_cast<float32>(static_cast<float64>(avgCAxisBuf[featureIdx]) / static_cast<float64>(hexNeighborListSize));
      }
      else
      {
        avgCAxisBuf[featureIdx] = std::nanf("");
      }
    }

    cAxisMisalignmentList.setList(featureIdx, {currentMisalignmentList.begin(), currentMisalignmentList.end()});
  }

  if(m_InputValues->FindAvgMisals)
  {
    if(Result<> ioResult = avgCAxisMisalignmentPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(avgCAxisBuf.data(), totalFeatures)); ioResult.invalid())
    {
      return MergeResults(std::move(result), std::move(ioResult));
    }
  }

  return result;
}
