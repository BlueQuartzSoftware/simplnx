#include "MergeTwins.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <random>

using namespace nx::core;

// -----------------------------------------------------------------------------
MergeTwins::MergeTwins(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeTwinsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
  m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
}

// -----------------------------------------------------------------------------
MergeTwins::~MergeTwins() noexcept = default;

// -----------------------------------------------------------------------------
int MergeTwins::getSeed(int32 newFid)
{
  auto& phases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  auto& featureParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();
  auto& cellFeaturesAttMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->NewCellFeatureAttributeMatrixPath);

  auto numFeatures = static_cast<int32>(phases.getNumberOfTuples());

  int32 seed = -1;

  // Precalculate some constants
  int32 totalFMinus1 = numFeatures - 1;

  usize counter = 0;

  auto randFeature = static_cast<int32>(m_Distribution(m_Generator) * static_cast<float32>(totalFMinus1));

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
    ShapeType tDims = {newFid + 1ULL};
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

  if(featureParentIds[neighborFeature] == -1 && phases[referenceFeature] > 0 && phases[neighborFeature] > 0)
  {
    uint32 laueClass = crystalStructures[phases[referenceFeature]];

    ebsdlib::QuatD q1(avgQuats[referenceFeature * 4], avgQuats[referenceFeature * 4 + 1], avgQuats[referenceFeature * 4 + 2], avgQuats[referenceFeature * 4 + 3]);
    ebsdlib::QuatD q2(avgQuats[neighborFeature * 4], avgQuats[neighborFeature * 4 + 1], avgQuats[neighborFeature * 4 + 2], avgQuats[neighborFeature * 4 + 3]);

    uint32 phase2 = crystalStructures[phases[neighborFeature]];
    if(laueClass == phase2 && (laueClass == ebsdlib::CrystalStructure::Cubic_High))
    {
      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
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
void MergeTwins::groupFeaturesExecute()
{ // This code used to be in GroupFeatures Superclass
  auto& conNeighborList = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->ContiguousNeighborListArrayPath);
  std::vector<int32_t> groupList;

  int32_t parentCount = 0;
  int32_t featureSeed = 0;
  int32_t list1size = 0, list2size = 0, listsize = 0;
  int32_t neigh = 0;

  while(featureSeed >= 0)
  {
    if(m_ShouldCancel)
    {
      return;
    }

    bool m_PatchGrouping = false;
    parentCount++;
    featureSeed = getSeed(parentCount);
    if(featureSeed >= 0)
    {
      groupList.push_back(featureSeed);
      for(std::vector<int32_t>::size_type j = 0; j < groupList.size(); j++)
      {
        int32_t firstFeature = groupList[j];
        list1size = int32_t(conNeighborList[firstFeature].size());

        for(int32_t k = 0; k < 2; k++)
        {
          if(k == 0)
          {
            listsize = list1size;
          }
          else if(k == 1)
          {
            listsize = list2size;
          }
          for(int32_t l = 0; l < listsize; l++)
          {
            if(k == 0)
            {
              neigh = conNeighborList[firstFeature][l];
            }
            else if(k == 1)
            {
            }
            if(neigh != firstFeature)
            {
              if(determineGrouping(firstFeature, neigh, parentCount))
              {
                if(!m_PatchGrouping)
                {
                  groupList.push_back(neigh);
                }
              }
            }
          }
        }
      }
    }
    groupList.clear();
  }
}

// -----------------------------------------------------------------------------
/**
 * @brief Merges twin-related features by clustering grains that share a 60-degree
 * <111> misorientation (sigma-3 twin relationship). The algorithm first identifies
 * twin pairs using the inherited groupFeatures framework, then assigns parent IDs
 * to every voxel based on the feature-to-parent mapping.
 *
 * OOC strategy: The cellParentIds array is initialized via chunked copyFromBuffer
 * (rather than a single fill() call) to avoid per-element OOC overhead. The
 * feature-to-parent mapping is cached in a local vector, then the voxel-level
 * parent assignment loop reads featureIds in 64K-tuple chunks via copyIntoBuffer,
 * looks up parents from the local cache, and bulk-writes cellParentIds via
 * copyFromBuffer.
 */
Result<> MergeTwins::operator()()
{
  Result result = {};

  m_Generator = std::mt19937_64(std::mt19937::default_seed);
  m_Distribution = std::uniform_real_distribution<float32>(0.0f, 1.0f);

  /* Sanity check that each phase is Cubic High (m3m) Laue class. If not then warn the user.
   * There is code later on to ensure that only m3m Laue class is used.
   */
  auto& laueClasses = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& cellParentIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellParentIdsArrayPath)->getDataStoreRef();
  auto& featureParentIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureParentIdsArrayPath)->getDataStoreRef();

  usize totalPoints = cellParentIdsStore.getNumberOfTuples();

  // Initialize cellParentIds to -1 using chunked bulk writes. For OOC stores,
  // a single fill() call would trigger per-element virtual dispatch; chunked
  // copyFromBuffer amortizes the overhead over 64K-tuple writes.
  {
    constexpr usize k_FillChunk = 65536;
    std::vector<int32> fillBuf(k_FillChunk, -1);
    for(usize offset = 0; offset < totalPoints; offset += k_FillChunk)
    {
      usize count = std::min(k_FillChunk, totalPoints - offset);
      cellParentIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(fillBuf.data(), count));
    }
  }

  featureParentIds.fill(-1);

  for(usize i = 1; i < laueClasses.getSize(); i++)
  {
    if(laueClasses[i] != ebsdlib::CrystalStructure::Cubic_High)
    {
      std::string msg = fmt::format("Phase '{}' is NOT m3m crystal symmetry. Data from this phase will not be used in this filter.", i);
      result = MakeWarningVoidResult(-23500, msg);
    }
  }

  featureParentIds[0] = 0; // set feature 0 to be parent 0

  // This kicks off the main clustering algorithm. This was taken from SIMPL::GroupFeatures
  // with sections of the function removed that would _never_ get hit.
  groupFeaturesExecute();

  // Now that the newly created Feature Attribute Matrix is sized correctly, fill
  // the `Active` array with True values
  auto& active = m_DataStructure.getDataAs<BoolArray>(m_InputValues->ActiveArrayPath)->getDataStoreRef();
  active.fill(true);

  // Check the number of Parents that were created....
  usize totalFeatures = active.getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return MergeResults(
        result, ConvertResult(MakeErrorResult<OutputActions>(-23501, "The number of grouped Features was 0 or 1 which means no grouped Features were detected. A grouping value may be set too high")));
  }

  // Cache feature-level featureParentIds into a local vector. The voxel loop
  // below looks up the parent for each voxel's feature (random access by
  // featureId), which would thrash OOC chunks if done via DataStore operator[].
  const usize numFeatures = featureParentIds.getNumberOfTuples();
  std::vector<int32> featureParentIdsCache(numFeatures);
  featureParentIds.copyIntoBuffer(0, nonstd::span<int32>(featureParentIdsCache.data(), numFeatures));

  // Assign parent IDs to every voxel using chunked bulk I/O: read a chunk of
  // featureIds, look up each feature's parent from the local cache, then
  // bulk-write the parent IDs back to the cellParentIds DataStore.
  int32 numParents = 0;
  {
    constexpr usize k_ChunkSize = 65536;
    std::vector<int32> featureIdsBuf(k_ChunkSize);
    std::vector<int32> cellParentIdsBuf(k_ChunkSize);

    for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      usize count = std::min(k_ChunkSize, totalPoints - offset);
      featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureIdsBuf.data(), count));

      for(usize i = 0; i < count; i++)
      {
        int32 featureName = featureIdsBuf[i];
        cellParentIdsBuf[i] = featureParentIdsCache[featureName];
        if(featureParentIdsCache[featureName] > numParents)
        {
          numParents = featureParentIdsCache[featureName];
        }
      }

      cellParentIdsStore.copyFromBuffer(offset, nonstd::span<const int32>(cellParentIdsBuf.data(), count));
    }
  }
  numParents += 1;

  // Randomize the feature Ids for purely visual clarify. Having random Feature Ids
  // allows users visualizing the data to better discern each grain otherwise the coloring
  // would look like a smooth gradient. This is a user input parameter
  if(m_InputValues->RandomizeParentIds)
  { // Randomize Parent IDs
    m_MessageHandler({IFilter::Message::Type::Info, "Randomizing Parent Ids...."});
    ClusterUtilities::RandomizeFeatureIds(featureParentIds, numParents);
  }

  return result;
}
