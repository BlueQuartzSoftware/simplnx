#include "GroupMicroTextureRegions.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#include <random>

using namespace nx::core;

// -----------------------------------------------------------------------------
GroupMicroTextureRegions::GroupMicroTextureRegions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   GroupMicroTextureRegionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_FeaturePhases(m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath))
, m_FeatureParentIds(m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureParentIdsArrayName))
, m_CrystalStructures(m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath))
, m_AvgQuats(m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath))
, m_Volumes(m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VolumesArrayPath))
{
}

// -----------------------------------------------------------------------------
GroupMicroTextureRegions::~GroupMicroTextureRegions() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GroupMicroTextureRegions::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void GroupMicroTextureRegions::randomizeParentIds(usize totalPoints, usize totalParentIds)
{
  // Shuffle parent IDs in [1, totalParentIds-1] via Fisher-Yates with the same
  // RNG state already seeded by operator(). Parent ID 0 is reserved (unassigned)
  // and is excluded from the shuffle so cells with no parent stay at 0.
  auto& cellParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellParentIdsArrayName);

  std::vector<int32> shuffle(totalParentIds);
  for(usize i = 0; i < totalParentIds; i++)
  {
    shuffle[i] = static_cast<int32>(i);
  }

  std::uniform_int_distribution<usize> intDist(1, totalParentIds - 1);
  for(usize i = 1; i < totalParentIds; i++)
  {
    usize r = intDist(m_Generator);
    std::swap(shuffle[i], shuffle[r]);
  }

  // Remap feature parent IDs first so cell parent IDs can index through the new mapping
  const usize numFeatures = m_FeatureParentIds.getNumberOfTuples();
  for(usize f = 0; f < numFeatures; f++)
  {
    const int32 oldId = m_FeatureParentIds[f];
    if(oldId >= 0 && static_cast<usize>(oldId) < totalParentIds)
    {
      m_FeatureParentIds[f] = shuffle[oldId];
    }
  }

  // Re-derive cell parent IDs from the shuffled feature parent IDs
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  for(usize k = 0; k < totalPoints; k++)
  {
    cellParentIds[k] = m_FeatureParentIds[featureIds[k]];
  }
}

// -----------------------------------------------------------------------------
Result<> GroupMicroTextureRegions::execute()
{
  ThrottledMessageHandler throttledMessenger(m_MessageHandler);

  NeighborList<int32>& featureNeighborListRef = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->ContiguousNeighborListArrayPath);
  NeighborList<int32>* nonContigNeighListPtr = nullptr;
  if(m_InputValues->UseNonContiguousNeighbors)
  {
    nonContigNeighListPtr = m_DataStructure.getDataAs<NeighborList<int32>>(m_InputValues->NonContiguousNeighborListArrayPath);
    if(nullptr == nonContigNeighListPtr)
    {
      return MakeErrorResult(-99345, "There was an error getting the Non-contiguous neighborlist from the DataStructure");
    }
  }

  std::vector<int32> groupList;

  int32 parentCount = 0;
  int32 featureSeed = 0;
  int32 list1size = 0;
  int32 list2size = 0;

  while(featureSeed >= 0)
  {
    parentCount++;
    featureSeed = getSeed(parentCount);
    if(featureSeed < 0)
    {
      continue;
    }

    groupList.clear();
    groupList.push_back(featureSeed);
    for(std::vector<int32>::size_type j = 0; j < groupList.size(); j++)
    {
      const int32 firstFeature = groupList[j];
      list1size = static_cast<int32>(featureNeighborListRef[firstFeature].size());
      if(m_InputValues->UseNonContiguousNeighbors)
      {
        list2size = nonContigNeighListPtr->getListSize(firstFeature);
      }
      // Walk contiguous neighbors (k=0) then optional non-contiguous neighbors (k=1)
      for(int32 k = 0; k < 2; k++)
      {
        const int32 listSize = (k == 0) ? list1size : list2size;
        for(int32 l = 0; l < listSize; l++)
        {
          int32 neigh = -1;
          if(k == 0)
          {
            neigh = featureNeighborListRef[firstFeature][l];
          }
          else if(k == 1 && m_InputValues->UseNonContiguousNeighbors)
          {
            bool ok = false;
            neigh = nonContigNeighListPtr->getValue(firstFeature, l, ok);
          }
          if(neigh >= 0 && neigh != firstFeature)
          {
            if(determineGrouping(firstFeature, neigh, parentCount))
            {
              groupList.push_back(neigh);
            }
          }
        }
      }
    }

    throttledMessenger.queueMessage("Parent Count: {}", parentCount);
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> GroupMicroTextureRegions::operator()()
{

  m_Generator = std::mt19937_64(m_InputValues->SeedValue);
  m_Distribution = std::uniform_real_distribution<float32>(0.0f, 1.0f);

  // Initialize Data
  m_AvgCAxes[0] = 0.0f;
  m_AvgCAxes[1] = 0.0f;
  m_AvgCAxes[2] = 0.0f;
  m_FeatureParentIds.fill(-1);

  // Execute the main grouping algorithm
  m_MessageHandler.sendInfoMessage(fmt::format("Start Grouping....."));

  // Execute the grouping algorithm
  Result<> result = execute();
  if(result.invalid())
  {
    return result;
  }

  // handle active array resize
  if(m_NumTuples < 2)
  {
    return MakeErrorResult(-87000, fmt::format("The number of grouped Features was {} which means no grouped features were detected. A grouping value may be set too high", m_NumTuples));
  }
  m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->NewCellFeatureAttributeMatrixName).resizeTuples(ShapeType{m_NumTuples});

  auto& cellParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellParentIdsArrayName);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const usize totalPoints = featureIds.getNumberOfTuples();
  for(usize k = 0; k < totalPoints; k++)
  {
    cellParentIds[k] = m_FeatureParentIds[featureIds[k]];
  }

  if(m_InputValues->RandomizeParentIds)
  {
    m_MessageHandler.sendInfoMessage(fmt::format("Randomizing Parent Ids"));
    randomizeParentIds(totalPoints, m_NumTuples);
  }

  return {};
}

// -----------------------------------------------------------------------------
int GroupMicroTextureRegions::getSeed(int32 newFid)
{
  usize numFeatures = m_FeaturePhases.getNumberOfTuples();

  int32 featureIdSeed = -1;

  // Precalculate some constants
  const int32 totalFMinus1 = static_cast<int32>(numFeatures) - 1;

  usize counter = 0;
  // This section finds a feature id that has not been grouped yet. It starts by
  // randomly selecting a feature id between 0 and numFeatures-1. We then start
  // looping. If the initial random value is valid then we exit the loop after
  // a single iteration. If that feature has already been grouped, then we add one
  // to the `randFeature` value and try again. If we get to the end of the range of
  // featureIds then the algorithm will loop back to featureId = 0 and start incrementing
  // from there. This is reasonably efficient as we only generate random numbers
  // as needed.
  auto randFeature = static_cast<int32>(m_Distribution(m_Generator) * static_cast<float32>(totalFMinus1));
  while(featureIdSeed == -1 && counter < numFeatures)
  {
    if(randFeature > totalFMinus1)
    {
      randFeature = randFeature - numFeatures;
    }
    if(m_FeatureParentIds.getValue(randFeature) == -1)
    {
      featureIdSeed = randFeature;
    }
    randFeature++;
    counter++;
  }

  //  // Used for debugging and demonstration
  //  if(newFid == 1)
  //  {
  //    auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VolumesArrayPath.replaceName("Centroids"));
  //    std::ofstream fout ("/tmp/GroupMicroTextureInitialVoxelSeeds.txt", std::ios_base::out | std::ios_base::app);
  //    fout << fmt::format("Feature Parent Id: {} | X: {}, Y: {}\n", voxelSeed, centroids.getComponent(voxelSeed, 0), centroids.getComponent(voxelSeed, 1));
  //  }

  if(featureIdSeed >= 0)
  {
    m_FeatureParentIds[featureIdSeed] = newFid;
    m_NumTuples = newFid + 1;

    if(m_InputValues->UseRunningAverage)
    {
      usize index = featureIdSeed * 4;
      // Get the orientation matrix (which is passive) and then transpose it to make it active transform
      ebsdlib::Matrix3X3F g1t = ebsdlib::Quaternion<float32>(m_AvgQuats.getValue(index + 0), m_AvgQuats.getValue(index + 1), m_AvgQuats.getValue(index + 2), m_AvgQuats.getValue(index + 3))
                                    .toOrientationMatrix()
                                    .toGMatrix()
                                    .transpose();
      ebsdlib::Matrix3X1F cAxis(0.0f, 0.0f, 1.0f);
      // normalize so that the dot product can be taken below without
      // dividing by the magnitudes (they would be 1)
      const ebsdlib::Matrix3X1F c1 = (g1t * cAxis).normalize();

      m_AvgCAxes = c1 * m_Volumes.getValue(featureIdSeed);
    }
  }

  return featureIdSeed;
}

// -----------------------------------------------------------------------------
bool GroupMicroTextureRegions::determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid)
{
  const int32 neighborParentId = m_FeatureParentIds.getValue(neighborFeature);
  const int32 referenceFeaturePhase = m_FeaturePhases.getValue(referenceFeature);
  const int32 neighborFeaturePhase = m_FeaturePhases.getValue(neighborFeature);

  if(neighborParentId == -1 && referenceFeaturePhase > 0 && neighborFeaturePhase > 0)
  {
    ebsdlib::Matrix3X1F c1 = {0.0f, 0.0f, 0.0f};
    ebsdlib::Matrix3X1F cAxis(0.0f, 0.0f, 1.0f);

    if(!m_InputValues->UseRunningAverage)
    {
      const usize index = referenceFeature * 4;
      // Get the orientation matrix (which is passive) and then transpose it to make it active transform
      // transpose the g matrix so when c-axis is multiplied by it,
      // it will give the sample direction that the c-axis is along
      ebsdlib::Matrix3X3F g1t = ebsdlib::Quaternion<float32>(m_AvgQuats.getValue(index + 0), m_AvgQuats.getValue(index + 1), m_AvgQuats.getValue(index + 2), m_AvgQuats.getValue(index + 3))
                                    .toOrientationMatrix()
                                    .toGMatrix()
                                    .transpose();
      c1 = (g1t * cAxis).normalize();
    }
    uint32 phase1 = m_CrystalStructures.getValue(referenceFeaturePhase);
    uint32 phase2 = m_CrystalStructures.getValue(neighborFeaturePhase);
    if(phase1 == phase2 && (phase1 == ebsdlib::CrystalStructure::Hexagonal_High))
    {
      const usize index = neighborFeature * 4;
      // Get the orientation matrix (which is passive) and then transpose it to make it active transform
      // transpose the g matrix so when c-axis is multiplied by it,
      // it will give the sample direction that the c-axis is along
      ebsdlib::Matrix3X3F g2t = ebsdlib::Quaternion<float32>(m_AvgQuats.getValue(index + 0), m_AvgQuats.getValue(index + 1), m_AvgQuats.getValue(index + 2), m_AvgQuats.getValue(index + 3))
                                    .toOrientationMatrix()
                                    .toGMatrix()
                                    .transpose();
      ebsdlib::Matrix3X1F c2 = (g2t * cAxis).normalize();

      float32 w;
      if(m_InputValues->UseRunningAverage)
      {
        w = m_AvgCAxes.cosTheta(c2);
      }
      else
      {
        w = c1.cosTheta(c2);
      }
      w = std::acos(std::clamp(w, -1.0f, 1.0f));

      // Convert user defined tolerance to radians.
      float32 cAxisToleranceRad = m_InputValues->CAxisTolerance * nx::core::Constants::k_PiF / 180.0f;
      if(w <= cAxisToleranceRad || (nx::core::Constants::k_PiD - w) <= cAxisToleranceRad)
      {
        m_FeatureParentIds.setValue(neighborFeature, newFid);
        if(m_InputValues->UseRunningAverage)
        {
          c2 = c2 * m_Volumes.getValue(neighborFeature);
          m_AvgCAxes = m_AvgCAxes + c2;
        }
        return true;
      }
    }
  }
  return false;
}
