#include "ComputeFeatureNeighborCAxisMisalignments.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
ComputeFeatureNeighborCAxisMisalignments::ComputeFeatureNeighborCAxisMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                   ComputeFeatureNeighborCAxisMisalignmentsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighborCAxisMisalignments::~ComputeFeatureNeighborCAxisMisalignments() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureNeighborCAxisMisalignments::operator()()
{
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize i = 1; i < crystalStructures.size(); ++i)
  {
    const auto crystalStructureType = crystalStructures[i];
    const bool isHex = crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_High || crystalStructureType == EbsdLib::CrystalStructure::Hexagonal_Low;
    allPhasesHexagonal = allPhasesHexagonal && isHex;
    noPhasesHexagonal = noPhasesHexagonal && !isHex;
  }

  // If NONE of the phases are hexagonal then bail out now with an error
  if(noPhasesHexagonal)
  {
    return MakeErrorResult(-1562, "No phases that have a crystal symmetry of Hexagonal (6/mmm or 6/m) were found.");
  }

  Result<> result;
  // Throw a warning for any NON-Hex Laue Phases
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back({-1563, "Non Hexagonal phases were found. All calculations for non Hexagonal phases will be skipped and a NaN value inserted."});
  }

  auto& neighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborListArrayPath);
  const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);

  auto& cAxisMisalignmentList = m_DataStructure.getDataRefAs<NeighborList<float32>>(m_InputValues->CAxisMisalignmentListArrayName);

  Float32Array* avgCAxisMisalignmentPtr = nullptr;
  if(m_InputValues->FindAvgMisals)
  {
    avgCAxisMisalignmentPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgCAxisMisalignmentsArrayName);
  }

  const usize totalFeatures = featurePhases.getNumberOfTuples();
  const usize numQuatComps = avgQuats.getNumberOfComponents();

  std::vector<std::vector<float>> misalignmentLists;
  misalignmentLists.resize(totalFeatures);

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};
  usize hexNeighborListSize = 0;
  uint32 xtalPhase1 = 0, xtalPhase2 = 0;
  usize nName = 0;
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    xtalPhase1 = crystalStructures[featurePhases[featureIdx]];

    const usize quatTupleIndex1 = featureIdx * numQuatComps;
    OrientationD oMatrix1 =
        OrientationTransformation::qu2om<QuatD, OrientationD>({avgQuats[quatTupleIndex1], avgQuats[quatTupleIndex1 + 1], avgQuats[quatTupleIndex1 + 2], avgQuats[quatTupleIndex1 + 3]});

    // transpose the g matrix so when c-axis is multiplied by `g`
    // it will give the sample direction that the c-axis is along
    Eigen::Vector3d c1 = OrientationMatrixToGMatrixTranspose(oMatrix1) * cAxis;
    // normalize so that the dot product can be taken below without
    // dividing by the magnitudes (they would be 1)
    c1.normalize();
    misalignmentLists[featureIdx].resize(neighborList[featureIdx].size(), -1.0f);
    for(usize j = 0; j < neighborList[featureIdx].size(); j++)
    {
      nName = neighborList[featureIdx][j];
      xtalPhase2 = crystalStructures[featurePhases[nName]];
      hexNeighborListSize = neighborList[featureIdx].size();
      if(xtalPhase1 == xtalPhase2 && (xtalPhase1 == EbsdLib::CrystalStructure::Hexagonal_High || xtalPhase1 == EbsdLib::CrystalStructure::Hexagonal_Low))
      {
        const usize quatTupleIndex2 = nName * numQuatComps;
        OrientationD oMatrix2 =
            OrientationTransformation::qu2om<QuatD, OrientationD>({avgQuats[quatTupleIndex2], avgQuats[quatTupleIndex2 + 1], avgQuats[quatTupleIndex2 + 2], avgQuats[quatTupleIndex2 + 3]});

        // transpose the g matrix so when c-axis is multiplied by `g`
        // it will give the sample direction that the c-axis is along
        Eigen::Vector3d c2 = OrientationMatrixToGMatrixTranspose(oMatrix2) * cAxis;
        // normalize so that the dot product can be taken below without
        // dividing by the magnitudes (they would be 1)
        c2.normalize();

        float64 w = ImageRotationUtilities::CosBetweenVectors(c1, c2);
        w = std::clamp(w, -1.0, 1.0);
        w = std::acos(w);
        if(w > (Constants::k_PiD / 2.0))
        {
          w = Constants::k_PiD - w;
        }

        misalignmentLists[featureIdx][j] = static_cast<float32>(w * Constants::k_180OverPiD);
        if(m_InputValues->FindAvgMisals)
        {
          float32 value = avgCAxisMisalignmentPtr->getValue(featureIdx) + misalignmentLists[featureIdx][j];
          avgCAxisMisalignmentPtr->setValue(featureIdx, value);
        }
      }
      else
      {
        if(m_InputValues->FindAvgMisals)
        {
          hexNeighborListSize--;
        }
        misalignmentLists[featureIdx][j] = std::nanf("");
      }
    }
    if(m_InputValues->FindAvgMisals)
    {
      if(hexNeighborListSize > 0)
      {
        float32 value = avgCAxisMisalignmentPtr->getValue(featureIdx) / static_cast<float32>(hexNeighborListSize);
        avgCAxisMisalignmentPtr->setValue(featureIdx, value);
      }
      else
      {
        avgCAxisMisalignmentPtr->setValue(featureIdx, std::nanf(""));
      }
      hexNeighborListSize = 0;
    }
  }

  cAxisMisalignmentList.setLists(misalignmentLists);

  return result;
}
