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
const std::atomic_bool& ComputeFeatureNeighborCAxisMisalignments::getCancel()
{
  return m_ShouldCancel;
}

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

  if(noPhasesHexagonal)
  {
    return MakeErrorResult(
        -1562, "Finding the feature neighbor c-axis mis orientation requires at least one phase to be Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back(
        {-1563, "Finding the feature neighbor c-axis mis orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Calculations for non Hexagonal phases will be "
                "skipped and a NAN value will be used instead."});
  }

  auto& neighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NeighborListArrayPath);
  const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);

  auto& cAxisMisalignmentList = m_DataStructure.getDataRefAs<NeighborList<float32>>(m_InputValues->CAxisMisalignmentListArrayName);
  auto& avgCAxisMisalignment = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxisMisalignmentsArrayName);

  const usize totalFeatures = featurePhases.getNumberOfTuples();
  const usize numQuatComps = avgQuats.getNumberOfComponents();

  std::vector<std::vector<float>> misalignmentLists;
  misalignmentLists.resize(totalFeatures);

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};
  usize hexNeighborListSize = 0;
  uint32 phase1 = 0, phase2 = 0;
  usize nName = 0;
  for(usize i = 1; i < totalFeatures; i++)
  {
    phase1 = crystalStructures[featurePhases[i]];

    const usize quatTupleIndex1 = i * numQuatComps;
    OrientationD oMatrix1 =
        OrientationTransformation::qu2om<QuatD, OrientationD>({avgQuats[quatTupleIndex1], avgQuats[quatTupleIndex1 + 1], avgQuats[quatTupleIndex1 + 2], avgQuats[quatTupleIndex1 + 3]});

    // transpose the g matrix so when c-axis is multiplied by it
    // it will give the sample direction that the c-axis is along
    Eigen::Vector3d c1 = OrientationMatrixToGMatrixTranspose(oMatrix1) * cAxis;
    // normalize so that the dot product can be taken below without
    // dividing by the magnitudes (they would be 1)
    c1.normalize();
    misalignmentLists[i].resize(neighborList[i].size(), -1.0f);
    for(usize j = 0; j < neighborList[i].size(); j++)
    {
      nName = neighborList[i][j];
      phase2 = crystalStructures[featurePhases[nName]];
      hexNeighborListSize = neighborList[i].size();
      if(phase1 == phase2 && (phase1 == EbsdLib::CrystalStructure::Hexagonal_High || phase1 == EbsdLib::CrystalStructure::Hexagonal_Low))
      {
        const usize quatTupleIndex2 = nName * numQuatComps;
        OrientationD oMatrix2 =
            OrientationTransformation::qu2om<QuatD, OrientationD>({avgQuats[quatTupleIndex2], avgQuats[quatTupleIndex2 + 1], avgQuats[quatTupleIndex2 + 2], avgQuats[quatTupleIndex2 + 3]});

        // transpose the g matrix so when c-axis is multiplied by it
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

        misalignmentLists[i][j] = static_cast<float32>(w * Constants::k_180OverPiD);
        if(m_InputValues->FindAvgMisals)
        {
          avgCAxisMisalignment[i] += misalignmentLists[i][j];
        }
      }
      else
      {
        if(m_InputValues->FindAvgMisals)
        {
          hexNeighborListSize--;
        }
        misalignmentLists[i][j] = NAN;
      }
    }
    if(m_InputValues->FindAvgMisals)
    {
      if(hexNeighborListSize > 0)
      {
        avgCAxisMisalignment[i] /= hexNeighborListSize;
      }
      else
      {
        avgCAxisMisalignment[i] = NAN;
      }
      hexNeighborListSize = 0;
    }
  }

  for(size_t i = 1; i < totalFeatures; i++)
  {
    // Set the vector for each list into the NeighborList Object
    NeighborList<float>::SharedVectorType misaL(new std::vector<float>);
    misaL->assign(misalignmentLists[i].begin(), misalignmentLists[i].end());
    cAxisMisalignmentList.setList(static_cast<int32_t>(i), misaL);
  }

  return result;
}
