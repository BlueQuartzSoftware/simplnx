#include "FillBadDataBFS.hpp"

#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <fmt/format.h>

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
// FillBadDataUpdateTuples
// -----------------------------------------------------------------------------
// Copies cell data array values from a good neighbor voxel to each bad data
// voxel. The `neighbors` vector maps each voxel index to the index of its best
// source neighbor (determined by majority vote in the iterative fill loop).
//
// Only voxels satisfying ALL of the following conditions are updated:
//   - featureId < 0  (marked as small bad-data region needing fill)
//   - neighbor != -1 (a valid source neighbor was found)
//   - neighbor != tupleIndex (not self-referencing; default sentinel)
//   - featureIds[neighbor] > 0 (the source is a real feature, not bad data)
//
// All components of the tuple are copied (e.g., 3-component RGB, 6-component
// tensor, etc.), preserving multi-component array semantics.
// -----------------------------------------------------------------------------
template <typename T>
void FillBadDataUpdateTuples(const Int32AbstractDataStore& featureIds, AbstractDataStore<T>& outputDataStore, const std::vector<int32>& neighbors)
{
  usize start = 0;
  usize stop = outputDataStore.getNumberOfTuples();
  const usize numComponents = outputDataStore.getNumberOfComponents();
  for(usize tupleIndex = start; tupleIndex < stop; tupleIndex++)
  {
    const int32 featureName = featureIds[tupleIndex];
    const int32 neighbor = neighbors[tupleIndex];
    if(neighbor == tupleIndex)
    {
      continue;
    }

    if(featureName < 0 && neighbor != -1 && featureIds[static_cast<usize>(neighbor)] > 0)
    {
      for(usize i = 0; i < numComponents; i++)
      {
        auto value = outputDataStore[neighbor * numComponents + i];
        outputDataStore[tupleIndex * numComponents + i] = value;
      }
    }
  }
}

struct FillBadDataUpdateTuplesFunctor
{
  template <typename T>
  void operator()(const Int32AbstractDataStore& featureIds, IDataArray* outputIDataArray, const std::vector<int32>& neighbors)
  {
    auto& outputStore = outputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    FillBadDataUpdateTuples(featureIds, outputStore, neighbors);
  }
};
} // namespace

// -----------------------------------------------------------------------------
FillBadDataBFS::FillBadDataBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadDataBFS::~FillBadDataBFS() noexcept = default;

// -----------------------------------------------------------------------------
// FillBadDataBFS::operator()
// -----------------------------------------------------------------------------
// BFS-based flood-fill algorithm for replacing bad data voxels with values
// from neighboring good features. The algorithm has three main steps:
//
// Step 1: Find the maximum feature ID (and optionally maximum phase).
//
// Step 2: BFS flood-fill to discover connected regions of bad data
//   (featureId == 0). Each region is classified by size:
//   - Large regions (>= minAllowedDefectSize): kept as voids (featureId
//     stays 0, optionally assigned a new phase).
//   - Small regions (< threshold): marked with featureId = -1 for filling.
//
// Step 3: Iterative morphological dilation. Each iteration scans all -1
//   voxels, finds the neighboring good feature with the most face-adjacent
//   votes (majority vote), and records the best neighbor. Then copies all
//   cell data components from that neighbor to the -1 voxel. Repeats until
//   no -1 voxels remain. FeatureIds are updated LAST to avoid changing the
//   vote source mid-iteration.
//
// NOTE: This algorithm uses O(N) memory (neighbors + alreadyChecked +
// featureNumber vectors), making it unsuitable for very large OOC datasets.
// Use FillBadDataCCL for out-of-core compatible processing.
// -----------------------------------------------------------------------------
Result<> FillBadDataBFS::operator()()
{
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->featureIdsArrayPath)->getDataStoreRef();
  const usize totalPoints = featureIdsStore.getNumberOfTuples();

  // O(N) allocations: one int32 per voxel for neighbor mapping, one bit per
  // voxel for BFS visited tracking
  std::vector<int32> neighbors(totalPoints, -1);
  std::vector<bool> alreadyChecked(totalPoints, false);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();

  Int32Array* cellPhasesPtr = nullptr;

  if(m_InputValues->storeAsNewPhase)
  {
    cellPhasesPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  }

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  usize count = 1;
  usize numFeatures = 0;
  usize maxPhase = 0;

  // --- Step 1: Find the maximum feature ID across all voxels ----------------
  // This value is used to size the featureNumber vote counter in Step 3.
  for(usize i = 0; i < totalPoints; i++)
  {
    int32 featureName = featureIdsStore[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  // Optionally find the maximum phase so large void regions can be assigned
  // to (maxPhase + 1), creating a distinct phase for visualization.
  if(m_InputValues->storeAsNewPhase)
  {
    for(usize i = 0; i < totalPoints; i++)
    {
      if((*cellPhasesPtr)[i] > maxPhase)
      {
        maxPhase = (*cellPhasesPtr)[i];
      }
    }
  }

  // Face-neighbor offsets in flat index space: -Z, -Y, -X, +X, +Y, +Z
  std::array<int64, 6> neighborPoints = {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};
  std::vector<int64> currentVisitedList;

  MessageHelper messageHelper(m_MessageHandler);

  // --- Step 2: BFS flood-fill to classify bad data regions ------------------
  // Mark all non-zero voxels as already checked (they are good features).
  // Then BFS from each unchecked voxel with featureId == 0 to discover
  // contiguous bad data regions.
  for(usize iter = 0; iter < totalPoints; iter++)
  {
    alreadyChecked[iter] = false;
    if(featureIdsStore[iter] != 0)
    {
      alreadyChecked[iter] = true;
    }
  }

  messageHelper.sendMessage("Identifying bad data regions via BFS...");

  for(usize i = 0; i < totalPoints; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(!alreadyChecked[i] && featureIdsStore[i] == 0)
    {
      // Start a new BFS from this seed voxel to discover all connected
      // bad-data voxels in this region
      currentVisitedList.push_back(static_cast<int64>(i));
      count = 0;
      while(count < currentVisitedList.size())
      {
        int64 index = currentVisitedList[count];
        int64 column = index % dims[0];
        int64 row = (index / dims[0]) % dims[1];
        int64 plane = index / (dims[0] * dims[1]);
        // Check all 6 face-adjacent neighbors, with boundary guard checks
        for(int32 j = 0; j < 6; j++)
        {
          int64 neighbor = index + neighborPoints[j];
          if(j == 0 && plane == 0)
          {
            continue;
          }
          if(j == 5 && plane == (dims[2] - 1))
          {
            continue;
          }
          if(j == 1 && row == 0)
          {
            continue;
          }
          if(j == 4 && row == (dims[1] - 1))
          {
            continue;
          }
          if(j == 2 && column == 0)
          {
            continue;
          }
          if(j == 3 && column == (dims[0] - 1))
          {
            continue;
          }
          if(featureIdsStore[neighbor] == 0 && !alreadyChecked[neighbor])
          {
            currentVisitedList.push_back(neighbor);
            alreadyChecked[neighbor] = true;
          }
        }
        count++;
      }
      // Classify this region by size:
      // Large regions (>= threshold): keep as voids (featureId = 0),
      // optionally assign to a new phase for visualization.
      if((int32)currentVisitedList.size() >= m_InputValues->minAllowedDefectSizeValue)
      {
        for(const auto& currentIndex : currentVisitedList)
        {
          featureIdsStore[currentIndex] = 0;
          if(m_InputValues->storeAsNewPhase)
          {
            (*cellPhasesPtr)[currentIndex] = static_cast<int32>(maxPhase) + 1;
          }
        }
      }
      // Small regions (< threshold): mark with -1 to indicate they should
      // be filled in Step 3 by copying data from neighboring good features.
      if((int32)currentVisitedList.size() < m_InputValues->minAllowedDefectSizeValue)
      {
        for(const auto& currentIndex : currentVisitedList)
        {
          featureIdsStore[currentIndex] = -1;
        }
      }
      currentVisitedList.clear();
    }
  }

  // --- Step 3: Iterative morphological dilation -----------------------------
  // Vote counter indexed by feature ID. O(numFeatures) memory.
  std::vector<int32> featureNumber(numFeatures + 1, 0);

  // Collect all cell data arrays that need updating when a voxel is filled
  // (excludes user-specified ignored arrays)
  std::optional<std::vector<DataPath>> allChildArrays = GetAllChildDataPaths(m_DataStructure, selectedImageGeom.getCellDataPath(), DataObject::Type::DataArray, m_InputValues->ignoredDataArrayPaths);
  std::vector<DataPath> voxelArrayNames;
  if(allChildArrays.has_value())
  {
    voxelArrayNames = allChildArrays.value();
  }

  // Iterate until no -1 voxels remain. Each iteration grows the good-data
  // boundary inward by one voxel layer (morphological dilation).
  int32 iteration = 0;
  while(count != 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    iteration++;
    count = 0;
    for(usize i = 0; i < totalPoints; i++)
    {
      int32 featureName = featureIdsStore[i];
      if(featureName < 0)
      {
        count++;
        int32 most = 0;
        int64 xIndex = static_cast<int64>(i % dims[0]);
        int64 yIndex = static_cast<int64>((i / dims[0]) % dims[1]);
        int64 zIndex = static_cast<int64>(i / (dims[0] * dims[1]));

        // First neighbor loop: tally votes from face-adjacent good features.
        // Each good neighbor increments featureNumber[its featureId]. The
        // feature with the highest vote count wins (majority vote), and
        // neighbors[i] records the winning neighbor's voxel index.
        for(int32 j = 0; j < 6; j++)
        {
          auto neighborPoint = static_cast<int64>(i + neighborPoints[j]);
          if(j == 0 && zIndex == 0)
          {
            continue;
          }
          if(j == 5 && zIndex == (dims[2] - 1))
          {
            continue;
          }
          if(j == 1 && yIndex == 0)
          {
            continue;
          }
          if(j == 4 && yIndex == (dims[1] - 1))
          {
            continue;
          }
          if(j == 2 && xIndex == 0)
          {
            continue;
          }
          if(j == 3 && xIndex == (dims[0] - 1))
          {
            continue;
          }

          int32 feature = featureIdsStore[neighborPoint];
          if(feature > 0)
          {
            featureNumber[feature]++;
            int32 current = featureNumber[feature];
            if(current > most)
            {
              most = current;
              neighbors[i] = static_cast<int32>(neighborPoint);
            }
          }
        }
        // Second neighbor loop: reset the vote counters for only the features
        // that were incremented above. This avoids zeroing the entire
        // featureNumber vector (which would be O(numFeatures) per voxel).
        for(int32 j = 0; j < 6; j++)
        {
          int64 neighborPoint = static_cast<int64>(i) + neighborPoints[j];
          if(j == 0 && zIndex == 0)
          {
            continue;
          }
          if(j == 5 && zIndex == (dims[2] - 1))
          {
            continue;
          }
          if(j == 1 && yIndex == 0)
          {
            continue;
          }
          if(j == 4 && yIndex == (dims[1] - 1))
          {
            continue;
          }
          if(j == 2 && xIndex == 0)
          {
            continue;
          }
          if(j == 3 && xIndex == (dims[0] - 1))
          {
            continue;
          }

          int32 feature = featureIdsStore[neighborPoint];
          if(feature > 0)
          {
            featureNumber[feature] = 0;
          }
        }
      }
    }

    // Apply fills: update all non-featureIds cell arrays first by copying
    // all components from the winning neighbor to the bad voxel.
    for(const auto& cellArrayPath : voxelArrayNames)
    {
      if(cellArrayPath == m_InputValues->featureIdsArrayPath)
      {
        continue;
      }
      auto* oldCellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);

      ExecuteDataFunction(FillBadDataUpdateTuplesFunctor{}, oldCellArray->getDataType(), featureIdsStore, oldCellArray, neighbors);
    }

    // Update FeatureIds LAST: the FillBadDataUpdateTuples calls above rely
    // on featureIds to check that the source neighbor is still a valid good
    // feature (featureId > 0). If featureIds were updated first, a freshly
    // filled voxel could become a vote source before its other arrays were
    // copied, leading to inconsistent data.
    FillBadDataUpdateTuples<int32>(featureIdsStore, featureIdsStore, neighbors);
  }
  return {};
}
