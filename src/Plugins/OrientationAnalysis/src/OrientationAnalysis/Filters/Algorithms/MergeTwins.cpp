#include "MergeTwins.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include <random>

using namespace nx::core;

// -----------------------------------------------------------------------------
MergeTwins::MergeTwins(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeTwinsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MergeTwins::~MergeTwins() noexcept = default;

// -----------------------------------------------------------------------------
int MergeTwins::getSeed(int32 newFid) const
{
  auto& phases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  auto& featureParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();
  auto& cellFeaturesAttMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->NewCellFeatureAttributeMatrixPath);

  auto numFeatures = static_cast<int32>(phases.getNumberOfTuples());

  int32 seed = -1;

  // Precalculate some constants
  int32 totalFMinus1 = numFeatures - 1;

  usize counter = 0;
  std::mt19937_64 generator(m_InputValues->Seed); // Standard mersenne_twister_engine seeded
  std::uniform_real_distribution<float32> distribution(0, 1);
  auto randFeature = static_cast<int32>(distribution(generator) * static_cast<float32>(totalFMinus1));
  while(seed == -1 && counter < numFeatures)
  {
    if(randFeature > totalFMinus1)
    {
      randFeature = randFeature - numFeatures;
    }
    if(featureParentIds[randFeature] == -1)
    {
      seed = randFeature;
    }
    randFeature++;
    counter++;
  }
  if(seed >= 0)
  {
    featureParentIds[seed] = newFid;
    std::vector<usize> tDims(1, newFid + 1);
    cellFeaturesAttMatrix.resizeTuples(tDims); // this will resize the active array as well
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool MergeTwins::determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid)
{
  auto& phases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  auto& featureParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();
  auto& crystalStructures = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  auto& avgQuats = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath)->getDataStoreRef();
  auto axisToleranceRad = m_InputValues->AxisTolerance * numbers::pi_v<float32> / 180.0f;

  bool twin = false;
  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

  if(featureParentIds[neighborFeature] == -1 && phases[referenceFeature] > 0 && phases[neighborFeature] > 0)
  {
    uint32 phase1 = crystalStructures[phases[referenceFeature]];

    QuatF q1(avgQuats[referenceFeature * 4], avgQuats[referenceFeature * 4 + 1], avgQuats[referenceFeature * 4 + 2], avgQuats[referenceFeature * 4 + 3]);
    QuatF q2(avgQuats[neighborFeature * 4], avgQuats[neighborFeature * 4 + 1], avgQuats[neighborFeature * 4 + 2], avgQuats[neighborFeature * 4 + 3]);

    uint32 phase2 = crystalStructures[phases[neighborFeature]];
    if(phase1 == phase2 && (phase1 == EbsdLib::CrystalStructure::Cubic_High))
    {
      OrientationD axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
      double w = axisAngle[3];
      w *= (180.0f / numbers::pi);
      double axisDiff111 = std::acos(std::fabs(axisAngle[0]) * 0.57735f + std::fabs(axisAngle[1]) * 0.57735f + fabs(axisAngle[2]) * 0.57735f);
      double angDiff60 = std::fabs(w - 60.0f);
      if(axisDiff111 < axisToleranceRad && angDiff60 < m_InputValues->AngleTolerance)
      {
        twin = true;
      }
      if(twin)
      {
        featureParentIds[neighborFeature] = newFid;
        return true;
      }
    }
  }
  return false;
}

// -----------------------------------------------------------------------------
Result<> MergeTwins::operator()()
{
  Result result = {};
  /* Sanity check that each phase is Cubic High (m3m) Laue class. If not then warn the user.
   * There is code later on to ensure that only m3m Laue class is used.
   */
  auto& laueClasses = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& cellParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellParentIdsArrayPath)->getDataStoreRef();
  cellParentIds.fill(-1);
  auto& featureParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();
  featureParentIds.fill(-1);
  auto& active = m_DataStructure.getDataAs<BoolArray>(m_InputValues->ActiveArrayPath)->getDataStoreRef();
  active.fill(true);

  for(usize i = 1; i < laueClasses.getSize(); i++)
  {
    if(laueClasses[i] != EbsdLib::CrystalStructure::Cubic_High)
    {
      std::string msg = fmt::format("Phase '{}' is NOT m3m crystal symmetry. Data from this phase will not be used in this filter.", i);
      result = MakeWarningVoidResult(-23500, msg);
    }
  }

  featureParentIds[0] = 0; // set feature 0 to be parent 0

  { // This code used to be in GroupFeatures Superclass
    auto& contNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->ContiguousNeighborListArrayPath);

    int32 parentCount = 1;
    int32 seed = getSeed(parentCount);
    int32 neigh;
    while(seed >= 0)
    {
      std::vector<int32> groupList = {seed};
      for(std::vector<int32>::size_type j = 0; j < groupList.size(); j++)
      {
        int32 firstFeature = groupList[j];
        auto list1size = static_cast<int32>(contNeighborList[firstFeature].size());
        for(int32 l = 0; l < list1size; l++)
        {
          neigh = contNeighborList[firstFeature][l];
          if(neigh != firstFeature)
          {
            if(determineGrouping(firstFeature, neigh, parentCount))
            {
              groupList.push_back(neigh);
            }
          }
        }
        if(m_InputValues->UseNonContiguousNeighbors)
        {
          auto& nonContNeighList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NonContiguousNeighborListArrayPath);
          int32 list2size = nonContNeighList.getListSize(firstFeature);
          for(int32 l = 0; l < list2size; l++)
          {
            neigh = nonContNeighList.getListReference(firstFeature)[l];
            if(neigh != firstFeature)
            {
              if(determineGrouping(firstFeature, neigh, parentCount))
              {
                groupList.push_back(neigh);
              }
            }
          }
        }
      }

      parentCount++;
      seed = getSeed(parentCount);
    }
  }

  usize totalFeatures = active.getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return MergeResults(
        result, ConvertResult(MakeErrorResult<OutputActions>(-23501, "The number of grouped Features was 0 or 1 which means no grouped Features were detected. A grouping value may be set too high")));
  }

  int32 numParents = 0;
  usize totalPoints = featureIds.getNumberOfTuples();
  for(usize k = 0; k < totalPoints; k++)
  {
    int32 featureName = featureIds[k];
    cellParentIds[k] = featureParentIds[featureName];
    if(featureParentIds[featureName] > numParents)
    {
      numParents = featureParentIds[featureName];
    }
  }
  numParents += 1;

  // Randomize the feature Ids for purely visual clarify. Having random Feature Ids
  // allows users visualizing the data to better discern each grain otherwise the coloring
  // would look like a smooth gradient. This is a user input parameter
  { // Randomize Parent IDs
    m_MessageHandler({IFilter::Message::Type::Info, "Randomizing Parent Ids...."});

    std::mt19937_64 gen(std::mt19937_64::default_seed); // Standard mersenne_twister_engine seeded with milliseconds
    std::uniform_real_distribution<float64> dist(0, 1);

    auto nParents = static_cast<usize>(numParents);

    std::vector<int32> parentIds(numParents);
    std::iota(parentIds.begin(), parentIds.end(), 0);

    m_MessageHandler({IFilter::Message::Type::Info, "Shuffling elements ...."});
    //--- Shuffle elements by randomly exchanging each with one other.
    for(usize i = 1; i < nParents; i++)
    {
      auto r = static_cast<usize>(std::floor(dist(gen) * static_cast<float64>(numParents - 1))); // Random remaining position.
      int32 pid_i = parentIds[i];
      parentIds[i] = parentIds[r];
      parentIds[r] = pid_i;
    }

    m_MessageHandler({IFilter::Message::Type::Info, "Adjusting Feature Ids Array...."});
    // Now adjust all the Feature ID values for each Voxel
    for(usize i = 0; i < totalPoints; ++i)
    {
      cellParentIds[i] = parentIds[cellParentIds[i]];
      featureParentIds[featureIds[i]] = cellParentIds[i];
    }
  }

  return result;
}
