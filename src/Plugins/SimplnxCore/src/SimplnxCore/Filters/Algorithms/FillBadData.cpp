#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <unordered_map>
#include <unordered_set>

using namespace nx::core;

// =============================================================================
// FillBadData Algorithm Overview
// =============================================================================
//
// This file implements an optimized algorithm for filling bad data (voxels with
// FeatureId == 0) in image geometries. The algorithm handles out-of-core datasets
// efficiently by streaming the data one Z-slab at a time via bulk I/O
// (copyIntoBuffer/copyFromBuffer) and uses a four-phase approach:
//
// Phase 1: Slab-Sequential Connected Component Labeling (CCL)
//   - Read one Z-slab at a time, assigning provisional labels to bad data regions
//   - Use Union-Find to track equivalences between labels across slab boundaries
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
// ChunkAwareUnionFind Implementation
// =============================================================================
//
// A Union-Find (Disjoint Set) data structure optimized for tracking connected
// component equivalences during slab-sequential processing, where labels are
// produced one Z-slab at a time and equivalences can span slab boundaries. Uses
// union-by-rank for efficient merging and defers path compression to a single
// flatten() pass to avoid redundant updates during construction.
//
// Key features:
// - Lazily creates entries as labels are encountered
// - Tracks rank for balanced union operations
// - Accumulates sizes at each label (not root) during construction
// - Single-pass path compression and size accumulation in flatten()
// =============================================================================

// -----------------------------------------------------------------------------
// Find the root representative of a label's equivalence class
// -----------------------------------------------------------------------------
// This performs a simple root lookup without path compression. Path compression
// is deferred to the flatten() method to avoid wasting cycles updating paths
// that will be modified again during later merges.
//
// @param x The label to find the root for
// @return The root label of the equivalence class
int64 ChunkAwareUnionFind::find(int64 x)
{
  // Create a parent entry if it doesn't exist (lazy initialization)
  if(!m_Parent.contains(x))
  {
    m_Parent[x] = x;
    m_Rank[x] = 0;
    m_Size[x] = 0;
  }

  // Find root iteratively without using the path compression algorithm
  // Path compression is deferred to flatten() to avoid wasting cycles
  // during frequent merges where paths would be updated repeatedly
  int64 root = x;
  while(m_Parent[root] != root)
  {
    root = m_Parent[root];
  }

  return root;
}

// -----------------------------------------------------------------------------
// Unite two labels into the same equivalence class
// -----------------------------------------------------------------------------
// Merges the sets containing labels a and b using union-by-rank heuristic.
// This keeps the tree balanced for better performance.
//
// @param a First label
// @param b Second label
void ChunkAwareUnionFind::unite(int64 a, int64 b)
{
  int64 rootA = find(a);
  int64 rootB = find(b);

  // Already in the same set
  if(rootA == rootB)
  {
    return;
  }

  // Union by rank: attach the smaller tree object under the root of the larger tree
  // This keeps the tree height logarithmic for better find() performance
  if(m_Rank[rootA] < m_Rank[rootB])
  {
    m_Parent[rootA] = rootB;
  }
  else if(m_Rank[rootA] > m_Rank[rootB])
  {
    m_Parent[rootB] = rootA;
  }
  else
  {
    // Equal rank: arbitrarily choose rootA as the parent and increment its rank
    m_Parent[rootB] = rootA;
    m_Rank[rootA]++;
  }
}

// -----------------------------------------------------------------------------
// Add voxel count to a label's size
// -----------------------------------------------------------------------------
// During construction, sizes are accumulated at each label (not root).
// This allows concurrent size updates without needing to find roots.
// All sizes will be accumulated to roots during flatten().
//
// @param label The label to add size to
// @param count Number of voxels to add
void ChunkAwareUnionFind::addSize(int64 label, uint64 count)
{
  // Add size to the label itself, not the root
  // Sizes will be accumulated to roots during flatten()
  m_Size[label] += count;
}

// -----------------------------------------------------------------------------
// Get the total size of a label's equivalence class
// -----------------------------------------------------------------------------
// Returns the accumulated size for a label's root. Should only be called
// after flatten() has been executed to get accurate totals.
//
// @param label The label to query
// @return Total number of voxels in the equivalence class
uint64 ChunkAwareUnionFind::getSize(int64 label)
{
  int64 root = find(label);
  auto it = m_Size.find(root);
  if(it == m_Size.end())
  {
    return 0;
  }
  return it->second;
}

// -----------------------------------------------------------------------------
// Flatten the Union-Find structure with path compression
// -----------------------------------------------------------------------------
// Performs a single-pass path compression and size accumulation after all
// merges are complete. This is more efficient than doing path compression
// during every find() operation when there are frequent merges.
//
// After flatten():
// - Every label points directly to its root (fully compressed paths)
// - All sizes are accumulated at root labels
// - Subsequent find() and getSize() operations are O(1)
void ChunkAwareUnionFind::flatten()
{
  // First pass: flatten all parents with path compression
  // Make every label point directly to its root for O(1) lookups
  // This is done in a single pass after all merges to avoid wasting
  // cycles updating paths repeatedly during construction
  std::unordered_map<int64, int64> finalRoots;
  for(auto& [label, parent] : m_Parent)
  {
    int64 root = find(label);
    finalRoots[label] = root;
  }

  // Second pass: accumulate sizes to roots
  // Sum up all the sizes from individual labels to their root representatives
  std::unordered_map<int64, uint64> rootSizes;
  for(const auto& [label, root] : finalRoots)
  {
    rootSizes[root] += m_Size[label];
  }

  // Replace maps with flattened versions for O(1) access
  m_Parent = finalRoots;
  m_Size = rootSizes;
}

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
// PHASE 1: Slab-Sequential Connected Component Labeling (CCL)
// =============================================================================
//
// Performs connected component labeling on bad data voxels (FeatureId == 0)
// using a slab-sequential scanline algorithm. This approach is optimized for
// out-of-core datasets: it streams one Z-slab at a time into a buffer via
// copyIntoBuffer instead of touching the store element-by-element.
//
// Algorithm:
// 1. Read each Z-slab sequentially into a buffer (keeping the previous slab
//    available for -Z neighbor lookups)
// 2. For each bad data voxel, check already-processed neighbors (-X, -Y, -Z)
// 3. If neighbors exist, reuse their label; otherwise assign new label
// 4. Track label equivalences in Union-Find structure
// 5. Track size of each connected component
//
// The scanline order ensures we only need to check 3 neighbors (previous in
// X, Y, and Z directions) instead of all 6 face neighbors, because later
// neighbors haven't been processed yet.
//
// @param featureIdsStore The feature IDs data store (maybe out-of-core)
// @param unionFind Union-Find structure for tracking label equivalences
// @param provisionalLabels Map from voxel index to assigned provisional label
// @param dims Image dimensions [X, Y, Z]
// @return Result indicating success or a slab-read failure
// =============================================================================
Result<> FillBadData::phaseOneCCL(Int32AbstractDataStore& featureIdsStore, ChunkAwareUnionFind& unionFind, std::unordered_map<usize, int64>& provisionalLabels, const std::array<int64, 3>& dims)
{
  int64 nextLabel = -1;
  const usize slabSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Two slab buffers: current Z-slab and previous Z-slab for -Z neighbor checks
  std::vector<int32> curSlab(slabSize);
  std::vector<int32> prevSlab(slabSize);

  for(int64 z = 0; z < dims[2]; z++)
  {
    const usize slabStart = static_cast<usize>(z) * slabSize;
    auto readResult = featureIdsStore.copyIntoBuffer(slabStart, nonstd::span<int32>(curSlab.data(), slabSize));
    if(readResult.invalid())
    {
      return MergeResults(readResult,
                          MakeErrorResult(-71500, fmt::format("FillBadData phase 1 (connected component labeling): failed to read Z-slab {} (start index {}, size {}) from feature IDs store.", z,
                                                              slabStart, slabSize)));
    }

    for(int64 y = 0; y < dims[1]; y++)
    {
      for(int64 x = 0; x < dims[0]; x++)
      {
        const usize localIdx = static_cast<usize>(y) * static_cast<usize>(dims[0]) + static_cast<usize>(x);
        if(curSlab[localIdx] != 0)
        {
          continue;
        }

        const usize globalIdx = slabStart + localIdx;
        std::vector<int64> neighborLabels;

        // Check -X neighbor (same slab)
        if(x > 0 && curSlab[localIdx - 1] == 0)
        {
          const usize nIdx = globalIdx - 1;
          if(provisionalLabels.contains(nIdx))
          {
            neighborLabels.push_back(provisionalLabels[nIdx]);
          }
        }
        // Check -Y neighbor (same slab)
        if(y > 0 && curSlab[localIdx - dims[0]] == 0)
        {
          const usize nIdx = globalIdx - dims[0];
          if(provisionalLabels.contains(nIdx))
          {
            neighborLabels.push_back(provisionalLabels[nIdx]);
          }
        }
        // Check -Z neighbor (previous slab)
        if(z > 0 && prevSlab[localIdx] == 0)
        {
          const usize nIdx = globalIdx - slabSize;
          if(provisionalLabels.contains(nIdx))
          {
            neighborLabels.push_back(provisionalLabels[nIdx]);
          }
        }

        int64 assignedLabel = 0;
        if(neighborLabels.empty())
        {
          assignedLabel = nextLabel--;
          unionFind.find(assignedLabel);
        }
        else
        {
          assignedLabel = neighborLabels[0];
          for(usize i = 1; i < neighborLabels.size(); i++)
          {
            if(neighborLabels[i] != assignedLabel)
            {
              unionFind.unite(assignedLabel, neighborLabels[i]);
            }
          }
        }

        provisionalLabels[globalIdx] = assignedLabel;
        unionFind.addSize(assignedLabel, 1);
      }
    }

    std::swap(prevSlab, curSlab);
  }
  return {};
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
// @param smallRegions Unused in current implementation (kept for interface compatibility)
// =============================================================================
void FillBadData::phaseTwoGlobalResolution(ChunkAwareUnionFind& unionFind, std::unordered_set<int64>& smallRegions)
{
  // Flatten the union-find structure to:
  // 1. Compress all paths (make every label point directly to root)
  // 2. Accumulate all sizes to root labels
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
// This phase streams the feature IDs one Z-slab at a time (reading and writing
// each slab back via bulk I/O) to relabel voxels based on their region
// classification. Large regions may optionally be assigned to a new phase (if
// storeAsNewPhase is true).
//
// @param featureIdsStore The feature IDs data store
// @param cellPhasesPtr Cell phases array (maybe null)
// @param provisionalLabels Map from voxel index to provisional label (from Phase 1)
// @param smallRegions Unused in current implementation (kept for interface compatibility)
// @param unionFind Union-Find structure with resolved equivalences (from Phase 2)
// @param maxPhase Maximum existing phase value (for new phase assignment)
// @return Result indicating success or a slab read/write failure
// =============================================================================
Result<> FillBadData::phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, const std::unordered_map<usize, int64>& provisionalLabels,
                                           const std::unordered_set<int64>& /*smallRegions*/, ChunkAwareUnionFind& unionFind, usize maxPhase) const
{
  // Collect the total size of each unique root label's connected component
  std::unordered_map<int64, uint64> rootSizes;
  for(const auto& [index, label] : provisionalLabels)
  {
    int64 root = unionFind.find(label);
    if(!rootSizes.contains(root))
    {
      rootSizes[root] = unionFind.getSize(root);
    }
  }

  // Classify regions as small (need filling) based on the size threshold
  std::unordered_set<int64> localSmallRegions;
  for(const auto& [root, size] : rootSizes)
  {
    if(static_cast<int32>(size) < m_InputValues->minAllowedDefectSizeValue)
    {
      localSmallRegions.insert(root);
    }
  }

  // Process slab-by-slab, reading and writing back via bulk I/O
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();
  const usize slabSize = udims[0] * udims[1];
  std::vector<int32> slab(slabSize);

  for(usize z = 0; z < udims[2]; z++)
  {
    const usize slabStart = z * slabSize;
    auto readResult = featureIdsStore.copyIntoBuffer(slabStart, nonstd::span<int32>(slab.data(), slabSize));
    if(readResult.invalid())
    {
      return MergeResults(readResult, MakeErrorResult(-71501, fmt::format("FillBadData phase 3 (region classification): failed to read Z-slab {} (start index {}, size {}) from feature IDs store.", z,
                                                                          slabStart, slabSize)));
    }

    bool slabModified = false;
    for(usize localIdx = 0; localIdx < slabSize; localIdx++)
    {
      const usize globalIdx = slabStart + localIdx;
      auto labelIter = provisionalLabels.find(globalIdx);
      if(labelIter == provisionalLabels.end())
      {
        continue;
      }

      int64 root = unionFind.find(labelIter->second);
      if(localSmallRegions.contains(root))
      {
        slab[localIdx] = -1;
      }
      else
      {
        slab[localIdx] = 0;
        if(m_InputValues->storeAsNewPhase && cellPhasesPtr != nullptr)
        {
          (*cellPhasesPtr)[globalIdx] = static_cast<int32>(maxPhase) + 1;
        }
      }
      slabModified = true;
    }

    if(slabModified)
    {
      auto writeResult = featureIdsStore.copyFromBuffer(slabStart, nonstd::span<const int32>(slab.data(), slabSize));
      if(writeResult.invalid())
      {
        return MergeResults(writeResult,
                            MakeErrorResult(-71502, fmt::format("FillBadData phase 3 (region classification): failed to write Z-slab {} (start index {}, size {}) back to feature IDs store.", z,
                                                                slabStart, slabSize)));
      }
    }
  }
  return {};
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

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

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
        std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
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
// 1. Slab-Sequential CCL: Label connected components of bad data
// 2. Global Resolution: Resolve equivalences and accumulate sizes
// 3. Region Classification: Classify regions as small or large
// 4. Iterative Fill: Fill small regions using morphological dilation
//
// @return Result indicating success or failure
// =============================================================================
Result<> FillBadData::operator()() const
{
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->featureIdsArrayPath)->getDataStoreRef();
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  const usize totalPoints = featureIdsStore.getNumberOfTuples();

  Int32Array* cellPhasesPtr = nullptr;
  usize maxPhase = 0;
  if(m_InputValues->storeAsNewPhase)
  {
    cellPhasesPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
    for(usize i = 0; i < totalPoints; i++)
    {
      if((*cellPhasesPtr)[i] > maxPhase)
      {
        maxPhase = (*cellPhasesPtr)[i];
      }
    }
  }

  usize numFeatures = 0;
  {
    const usize bufSize = 65536;
    std::vector<int32> buf(bufSize);
    for(usize offset = 0; offset < totalPoints; offset += bufSize)
    {
      const usize count = std::min(bufSize, totalPoints - offset);
      auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(buf.data(), count));
      if(readResult.invalid())
      {
        return MergeResults(readResult, MakeErrorResult(-71503, fmt::format("FillBadData: failed to scan feature IDs store for maximum feature id (buffer range [{}, {}) of {}).", offset,
                                                                            offset + count, totalPoints)));
      }
      for(usize i = 0; i < count; i++)
      {
        if(buf[i] > static_cast<int32>(numFeatures))
        {
          numFeatures = buf[i];
        }
      }
    }
  }

  ChunkAwareUnionFind unionFind;
  std::unordered_map<usize, int64> provisionalLabels;
  std::unordered_set<int64> smallRegions;

  m_MessageHandler({IFilter::Message::Type::Info, "Phase 1/4: Labeling connected components..."});
  auto phaseOneResult = phaseOneCCL(featureIdsStore, unionFind, provisionalLabels, dims);
  if(phaseOneResult.invalid())
  {
    return phaseOneResult;
  }

  m_MessageHandler({IFilter::Message::Type::Info, "Phase 2/4: Resolving region equivalences..."});
  phaseTwoGlobalResolution(unionFind, smallRegions);

  m_MessageHandler({IFilter::Message::Type::Info, "Phase 3/4: Classifying region sizes..."});
  auto phaseThreeResult = phaseThreeRelabeling(featureIdsStore, cellPhasesPtr, provisionalLabels, smallRegions, unionFind, maxPhase);
  if(phaseThreeResult.invalid())
  {
    return phaseThreeResult;
  }

  m_MessageHandler({IFilter::Message::Type::Info, "Phase 4/4: Filling small defects..."});
  phaseFourIterativeFill(featureIdsStore, dims, numFeatures);

  return {};
}
