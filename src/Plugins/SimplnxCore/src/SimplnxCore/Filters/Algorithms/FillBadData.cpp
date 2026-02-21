#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

// =============================================================================
// FillBadData Algorithm Overview
// =============================================================================
//
// This file implements an optimized algorithm for filling bad data (voxels with
// FeatureId == 0) in image geometries. The algorithm handles out-of-core datasets
// efficiently by processing data in chunks and uses a four-phase approach:
//
// Phase 1: Chunk-Sequential Connected Component Labeling (CCL)
//   - Process chunks sequentially, assigning provisional labels to bad data regions
//   - Use Union-Find to track equivalences between labels across chunk boundaries
//   - Track size of each connected component
//
// Phase 2: Global Resolution
//   - Flatten Union-Find structure to resolve all equivalences
//   - Accumulate region sizes to root labels
//
// Phase 3: Region Classification and Relabeling
//   - Classify regions as "small" (below threshold) or "large" (above threshold)
//   - Small regions: mark with -1 for filling in Phase 4
//   - Large regions: keep as 0 or assign to new phase (if requested)
//
// Phase 4: Iterative Morphological Fill
//   - Iteratively fill -1 voxels by assigning them to the most common neighbor
//   - Update all cell data arrays to match the filled voxels
//
// =============================================================================

namespace
{
// -----------------------------------------------------------------------------
// Helper function: Update data array tuples based on neighbor assignments
// -----------------------------------------------------------------------------
// Copies data from neighbor voxels to fill bad data voxels (-1 values)
// This is used to propagate cell data attributes during the filling process
//
// @param featureIds The feature IDs array indicating which voxels are bad data
// @param outputDataStore The data array to update
// @param neighbors The neighbor assignments (index of the neighbor to copy from)
template <typename T>
void FillBadDataUpdateTuples(const Int32AbstractDataStore& featureIds, AbstractDataStore<T>& outputDataStore, const std::vector<int32>& neighbors)
{
  usize start = 0;
  usize stop = outputDataStore.getNumberOfTuples();
  const usize numComponents = outputDataStore.getNumberOfComponents();

  // Loop through all tuples in the data array
  for(usize tupleIndex = start; tupleIndex < stop; tupleIndex++)
  {
    const int32 featureName = featureIds[tupleIndex];
    const int32 neighbor = neighbors[tupleIndex];

    // Skip if no neighbor assignment
    if(neighbor == tupleIndex)
    {
      continue;
    }

    // Copy data from the valid neighbor to bad data voxel
    // Only copy if the current voxel is bad data (-1) and the neighbor is valid (>0)
    if(featureName < 0 && neighbor != -1 && featureIds[static_cast<usize>(neighbor)] > 0)
    {
      // Copy all components from neighbor tuple to current tuple
      for(usize i = 0; i < numComponents; i++)
      {
        auto value = outputDataStore[neighbor * numComponents + i];
        outputDataStore[tupleIndex * numComponents + i] = value;
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Functor for type-dispatched tuple updates
// -----------------------------------------------------------------------------
// Allows the FillBadDataUpdateTuples function to be called with runtime type dispatch
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

// =============================================================================
// FillBadData Implementation
// =============================================================================

FillBadData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadData::~FillBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FillBadData::getCancel() const
{
  return m_ShouldCancel;
}

// =============================================================================
// PHASE 1: Chunk-Sequential Connected Component Labeling (CCL)
// =============================================================================
//
// Performs connected component labeling on bad data voxels (FeatureId == 0)
// using a chunk-sequential scanline algorithm optimized for out-of-core data.
//
// Key optimization: backward neighbor lookups read from the in-memory
// provisionalLabels buffer instead of featureIdsStore, avoiding cross-chunk
// reads that would cause segfaults or severe chunk thrashing with OOC storage.
//
// @param featureIdsStore The feature IDs data store (maybe out-of-core)
// @param unionFind Union-Find structure for tracking label equivalences
// @param provisionalLabels Dense buffer (0 = not bad data, >0 = provisional label)
// @param nextLabel Output: next available label after Phase 1
// @param dims Image dimensions [X, Y, Z]
// =============================================================================
void FillBadData::phaseOneCCL(Int32AbstractDataStore& featureIdsStore, UnionFind& unionFind, std::vector<int32>& provisionalLabels, int32& nextLabel, const std::array<int64, 3>& dims)
{
  nextLabel = 1;

  const int64 sliceStride = dims[0] * dims[1];
  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  // Process each chunk sequentially
  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    featureIdsStore.loadChunk(chunkIdx);

    // Chunk bounds are INCLUSIVE and in [Z, Y, X] order
    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          const usize index = z * sliceStride + y * dims[0] + x;

          // Only process bad data voxels (FeatureId == 0)
          if(featureIdsStore[index] != 0)
          {
            continue;
          }

          // Check backward neighbors using in-memory buffer (no OOC reads)
          int32 assignedLabel = 0;

          // Check -X neighbor
          if(x > 0)
          {
            int32 neighLabel = provisionalLabels[index - 1];
            if(neighLabel > 0)
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                unionFind.unite(assignedLabel, neighLabel);
              }
            }
          }

          // Check -Y neighbor
          if(y > 0)
          {
            int32 neighLabel = provisionalLabels[index - dims[0]];
            if(neighLabel > 0)
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                unionFind.unite(assignedLabel, neighLabel);
              }
            }
          }

          // Check -Z neighbor
          if(z > 0)
          {
            int32 neighLabel = provisionalLabels[index - sliceStride];
            if(neighLabel > 0)
            {
              if(assignedLabel == 0)
              {
                assignedLabel = neighLabel;
              }
              else if(assignedLabel != neighLabel)
              {
                unionFind.unite(assignedLabel, neighLabel);
              }
            }
          }

          // If no matching backward neighbor, assign new label
          if(assignedLabel == 0)
          {
            assignedLabel = nextLabel++;
            unionFind.find(assignedLabel); // Initialize in union-find
          }

          provisionalLabels[index] = assignedLabel;
          unionFind.addSize(assignedLabel, 1);
        }
      }
    }
  }

  featureIdsStore.flush();
}

// =============================================================================
// PHASE 2: Global Resolution of Equivalences
// =============================================================================
//
// Resolves all label equivalences from Phase 1 and accumulates region sizes.
// After this phase:
// - All labels point directly to their root representatives
// - All sizes are accumulated at root labels
// - Region sizes can be queried in O(1) time
//
// @param unionFind Union-Find structure containing label equivalences
// =============================================================================
void FillBadData::phaseTwoGlobalResolution(UnionFind& unionFind)
{
  unionFind.flatten();
}

// =============================================================================
// PHASE 3: Region Classification and Relabeling
// =============================================================================
//
// Classifies bad data regions as "small" or "large" based on size threshold:
// - Small regions (< minAllowedDefectSize): marked with -1 for filling in Phase 4
// - Large regions (>= minAllowedDefectSize): kept as 0 (or assigned new phase)
//
// Uses direct vector lookups (indexed by provisional label) instead of hash maps
// for O(1) classification of each voxel.
//
// @param featureIdsStore The feature IDs data store
// @param cellPhasesPtr Cell phases array (maybe null)
// @param provisionalLabels Dense buffer from Phase 1 (0 = not bad data, >0 = label)
// @param nextLabel Number of provisional labels assigned in Phase 1
// @param unionFind Union-Find structure with resolved equivalences (from Phase 2)
// @param maxPhase Maximum existing phase value (for new phase assignment)
// =============================================================================
void FillBadData::phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, const std::vector<int32>& provisionalLabels, int32 nextLabel, UnionFind& unionFind,
                                       usize maxPhase) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();
  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  // Build classification vector: isSmallRoot[label] indicates if root is small
  // 0 = unclassified, 1 = small (fill), -1 = large (keep)
  std::vector<int8> isSmallRoot(static_cast<usize>(nextLabel), 0);
  for(int32 label = 1; label < nextLabel; label++)
  {
    int32 root = static_cast<int32>(unionFind.find(label));
    if(isSmallRoot[root] == 0)
    {
      uint64 regionSize = unionFind.getSize(root);
      isSmallRoot[root] = (static_cast<int32>(regionSize) < m_InputValues->minAllowedDefectSizeValue) ? 1 : -1;
    }
    // Propagate classification to non-root labels for O(1) lookup
    isSmallRoot[label] = isSmallRoot[root];
  }

  // Process each chunk to relabel voxels based on region classification
  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    featureIdsStore.loadChunk(chunkIdx);

    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          const usize index = z * udims[0] * udims[1] + y * udims[0] + x;

          int32 provLabel = provisionalLabels[index];
          if(provLabel > 0)
          {
            if(isSmallRoot[provLabel] == 1)
            {
              // Small region - mark with -1 for filling in Phase 4
              featureIdsStore[index] = -1;
            }
            else
            {
              // Large region - keep as bad data (0) or assign to a new phase
              featureIdsStore[index] = 0;

              if(m_InputValues->storeAsNewPhase && cellPhasesPtr != nullptr)
              {
                (*cellPhasesPtr)[index] = static_cast<int32>(maxPhase) + 1;
              }
            }
          }
        }
      }
    }
  }

  featureIdsStore.flush();
}

// =============================================================================
// PHASE 4: Iterative Morphological Fill
// =============================================================================
//
// Fills small bad data regions (marked with -1 in Phase 3) using iterative
// morphological dilation. Each iteration:
// 1. For each -1 voxel, find the most common positive feature among its neighbors
// 2. Assign that voxel to the most common neighbor's feature
// 3. Update all cell data arrays to match the filled voxels
//
// This process repeats until all -1 voxels have been filled. The algorithm
// gradually fills small defects from the edges inward, ensuring smooth boundaries.
//
// @param featureIdsStore The feature IDs data store
// @param dims Image dimensions [X, Y, Z]
// @param numFeatures Number of features in the dataset
// =============================================================================
void FillBadData::phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64, 3>& dims, usize numFeatures) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const usize totalPoints = featureIdsStore.getNumberOfTuples();

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // Neighbor assignment array: neighbors[i] = index of the neighbor to copy from
  std::vector<int32> neighbors(totalPoints, -1);

  // Feature vote counter: tracks how many times each feature appears as the neighbor
  std::vector<int32> featureNumber(numFeatures + 1, 0);

  // Get a list of all cell arrays that need to be updated during filling
  // Exclude arrays specified in ignoredDataArrayPaths
  std::optional<std::vector<DataPath>> allChildArrays = GetAllChildDataPaths(m_DataStructure, selectedImageGeom.getCellDataPath(), DataObject::Type::DataArray, m_InputValues->ignoredDataArrayPaths);
  std::vector<DataPath> voxelArrayNames;
  if(allChildArrays.has_value())
  {
    voxelArrayNames = allChildArrays.value();
  }

  // Create a message helper for throttled progress updates (1 update per second)
  MessageHelper messageHelper(m_MessageHandler, std::chrono::milliseconds(1000));
  auto throttledMessenger = messageHelper.createThrottledMessenger(std::chrono::milliseconds(1000));

  usize count = 1;     // Number of voxels with -1 value that remain
  usize iteration = 0; // Current iteration number

  // Iteratively fill until no voxels with -1 value remain
  while(count != 0)
  {
    iteration++;
    count = 0; // Reset count of voxels with a -1 value for this iteration

    // Pass 1: Determine neighbor assignments for all -1 voxels
    // For each -1 voxel, find the most common positive feature among neighbors
    for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      int32 featureName = featureIdsStore[voxelIndex];

      // Only process voxels marked for filling (-1)
      if(featureName < 0)
      {
        count++;        // Count this voxel as needing filling
        int32 most = 0; // Highest vote count seen so far

        // Compute 3D position from the linear index
        int64 xIdx = voxelIndex % dims[0];
        int64 yIdx = (voxelIndex / dims[0]) % dims[1];
        int64 zIdx = voxelIndex / (dims[0] * dims[1]);

        // Vote for the most common positive neighbor feature
        // Loop over the 6 face neighbors of the voxel
        std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndex : faceNeighborInternalIdx)
        {
          // Skip neighbors outside image bounds
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }

          auto neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
          int32 feature = featureIdsStore[neighborPoint];

          // Only vote for positive features (valid data)
          if(feature > 0)
          {
            // Increment vote count for this feature
            featureNumber[feature]++;
            int32 current = featureNumber[feature];

            // Track the feature with the most votes
            if(current > most)
            {
              most = current;
              neighbors[voxelIndex] = static_cast<int32>(neighborPoint); // Store neighbor to copy from
            }
          }
        }

        // Reset vote counters for next voxel
        // Only reset features that were actually counted to save time
        // Loop over the 6 face neighbors of the voxel
        isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndex : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }

          int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
          int32 feature = featureIdsStore[neighborPoint];

          if(feature > 0)
          {
            featureNumber[feature] = 0;
          }
        }
      }
    }

    // Pass 2: Update all cell data arrays based on neighbor assignments
    // This propagates all cell data attributes (not just feature IDs) to filled voxels
    for(const auto& cellArrayPath : voxelArrayNames)
    {
      // Skip the feature IDs array (will be updated separately below)
      if(cellArrayPath == m_InputValues->featureIdsArrayPath)
      {
        continue;
      }

      auto* oldCellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);

      // Use the type-dispatched update function to handle all data types
      ExecuteDataFunction(FillBadDataUpdateTuplesFunctor{}, oldCellArray->getDataType(), featureIdsStore, oldCellArray, neighbors);
    }

    // Update FeatureIds array last to finalize the iteration
    FillBadDataUpdateTuples<int32>(featureIdsStore, featureIdsStore, neighbors);

    // Send throttled progress update (max 1 per second)
    throttledMessenger.sendThrottledMessage([iteration, count]() { return fmt::format("  Iteration {}: {} voxels remaining to fill", iteration, count); });
  }

  // Send final completion summary
  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("  Completed in {} iteration{}", iteration, iteration == 1 ? "" : "s")});
}

// =============================================================================
// Main Algorithm Entry Point
// =============================================================================
//
// Executes the four-phase bad data filling algorithm:
// 1. Chunk-Sequential CCL: Label connected components of bad data
// 2. Global Resolution: Resolve equivalences and accumulate sizes
// 3. Region Classification: Classify regions as small or large
// 4. Iterative Fill: Fill small regions using morphological dilation
//
// @return Result indicating success or failure
// =============================================================================
Result<> FillBadData::operator()() const
{
  // Get feature IDs array and image geometry
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->featureIdsArrayPath)->getDataStoreRef();
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();

  // Convert dimensions to signed integers for offset calculations
  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  const usize totalPoints = featureIdsStore.getNumberOfTuples();

  // Get cell phases array if we need to assign large regions to a new phase
  Int32Array* cellPhasesPtr = nullptr;
  usize maxPhase = 0;

  if(m_InputValues->storeAsNewPhase)
  {
    cellPhasesPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);

    // Find the maximum existing phase value
    for(usize i = 0; i < totalPoints; i++)
    {
      if((*cellPhasesPtr)[i] > maxPhase)
      {
        maxPhase = (*cellPhasesPtr)[i];
      }
    }
  }

  // Count the number of existing features for array sizing
  usize numFeatures = 0;
  for(usize i = 0; i < totalPoints; i++)
  {
    int32 featureName = featureIdsStore[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  // Initialize data structures for chunk-aware connected component labeling
  UnionFind unionFind;
  std::vector<int32> provisionalLabels(totalPoints, 0); // Dense buffer: 0 = not bad data
  int32 nextLabel = 1;

  // Phase 1: Chunk-Sequential Connected Component Labeling
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 1/4: Labeling connected components..."});
  phaseOneCCL(featureIdsStore, unionFind, provisionalLabels, nextLabel, dims);

  // Phase 2: Global Resolution of equivalences
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 2/4: Resolving region equivalences..."});
  phaseTwoGlobalResolution(unionFind);

  // Phase 3: Relabeling based on region size classification
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 3/4: Classifying region sizes..."});
  phaseThreeRelabeling(featureIdsStore, cellPhasesPtr, provisionalLabels, nextLabel, unionFind, maxPhase);

  // Phase 4: Iterative morphological fill
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 4/4: Filling small defects..."});
  phaseFourIterativeFill(featureIdsStore, dims, numFeatures);

  return {};
}
