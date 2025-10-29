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
  // Validate any Crystal Structure issues early in the process.
  // If none of the phases are hexagonal, then report and return
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

  // Get references to all the input data
  auto& neighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborListArrayPath);
  const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& featureAvgQuat = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);

  // Get references to all the Output data
  auto& cAxisMisalignmentList = m_DataStructure.getDataRefAs<NeighborList<float32>>(m_InputValues->CAxisMisalignmentListArrayName);
  Float32Array* avgCAxisMisalignmentPtr = nullptr;
  if(m_InputValues->FindAvgMisals)
  {
    avgCAxisMisalignmentPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgCAxisMisalignmentsArrayName);
  }

  const usize totalFeatures = featurePhases.getNumberOfTuples();
  const usize numQuatComps = featureAvgQuat.getNumberOfComponents();

  std::vector<std::vector<double>> misalignmentLists(totalFeatures);

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};
  usize hexNeighborListSize = 0;
  uint32 xtalPhase1 = 0;
  uint32 xtalPhase2 = 0;

  // Loop over every feature
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    // Get the crystal structure of phase 1
    xtalPhase1 = crystalStructures[featurePhases[featureIdx]];

    const usize quatTupleIndex1 = featureIdx * numQuatComps;
    OrientationD oMatrix1 = OrientationTransformation::qu2om<QuatD, OrientationD>(
        {featureAvgQuat[quatTupleIndex1], featureAvgQuat[quatTupleIndex1 + 1], featureAvgQuat[quatTupleIndex1 + 2], featureAvgQuat[quatTupleIndex1 + 3]});

    // transpose the g matrix so when c-axis is multiplied by `g`
    // it will give the sample direction that the c-axis is along
    Eigen::Vector3d c1 = OrientationMatrixToGMatrixTranspose(oMatrix1) * cAxis;
    // normalize so that the dot product can be taken below without
    // dividing by the magnitudes (they would be 1)
    c1.normalize();

    // Allocate enough room based on the current features neighbor list size
    const NeighborList<int>::VectorType& currentNeighborList = neighborList[featureIdx];
    auto& currentMisalignmentList = misalignmentLists[featureIdx];
    currentMisalignmentList.resize(currentNeighborList.size(), -1.0);
    for(usize j = 0; j < currentNeighborList.size(); j++)
    {
      int neighborFeatureId = currentNeighborList[j];
      xtalPhase2 = crystalStructures[featurePhases[neighborFeatureId]];
      hexNeighborListSize = currentNeighborList.size();

      // If both the feature and the neighbor are both Hexagonal Phases
      if(xtalPhase1 == xtalPhase2 && (xtalPhase1 == EbsdLib::CrystalStructure::Hexagonal_High || xtalPhase1 == EbsdLib::CrystalStructure::Hexagonal_Low))
      {
        const usize quatTupleIndex2 = neighborFeatureId * numQuatComps;
        OrientationD oMatrix2 = OrientationTransformation::qu2om<QuatD, OrientationD>(
            {featureAvgQuat[quatTupleIndex2], featureAvgQuat[quatTupleIndex2 + 1], featureAvgQuat[quatTupleIndex2 + 2], featureAvgQuat[quatTupleIndex2 + 3]});

        // transpose the g matrix so when c-axis is multiplied by `g`
        // it will give the sample direction that the c-axis is along
        Eigen::Vector3d c2 = OrientationMatrixToGMatrixTranspose(oMatrix2) * cAxis;
        // normalize so that the dot product can be taken below without
        // dividing by the magnitudes (they would be 1)
        c2.normalize();

        float64 w = ImageRotationUtilities::CosBetweenVectors(c1, c2);
        w = std::clamp(w, -1.0, 1.0);
        w = std::acos(w);
        if(w > Constants::k_PiOver2D)
        {
          w = Constants::k_PiD - w;
        }

        // Convert the misorientation to Degrees and store the value
        currentMisalignmentList[j] = w * Constants::k_180OverPiD;

        // If we are finding the average misorientation, then start accumulating those values
        if(m_InputValues->FindAvgMisals)
        {
          float32 value = avgCAxisMisalignmentPtr->getValue(featureIdx) + currentMisalignmentList[j];
          avgCAxisMisalignmentPtr->setValue(featureIdx, value);
        }
      }
      else // The current feature and it's neighbor do not match in crystal structures so place a NaN value
      {
        if(m_InputValues->FindAvgMisals)
        {
          hexNeighborListSize--;
        }
        currentMisalignmentList[j] = std::nanf("");
      }
    }

    // If the user has asked to find the average misorientation then run that loop here
    // on the current feature.
    if(m_InputValues->FindAvgMisals)
    {
      if(hexNeighborListSize > 0)
      {
        double value = avgCAxisMisalignmentPtr->getValue(featureIdx) / static_cast<double>(hexNeighborListSize);
        avgCAxisMisalignmentPtr->setValue(featureIdx, value);
      }
      else
      {
        avgCAxisMisalignmentPtr->setValue(featureIdx, std::nan(""));
      }
      hexNeighborListSize = 0;
    }

    cAxisMisalignmentList.setList(featureIdx, {currentMisalignmentList.begin(), currentMisalignmentList.end()});
  }

  return result;
}
