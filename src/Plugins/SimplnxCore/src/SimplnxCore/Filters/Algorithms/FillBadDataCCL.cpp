#include "FillBadDataCCL.hpp"

#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <cstdio>

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
// Phase 4: Iterative Morphological Fill (On-Disk Deferred)
//   - Uses a temporary file to defer fills: Pass 1 writes (dest, src) pairs,
//     Pass 2 reads them back and applies fills.
//   - No O(N) memory allocations — uses O(features) vote counters + temp file I/O.
//
// =============================================================================

namespace
{
// -----------------------------------------------------------------------------
// Helper: Copy all components of a single tuple from src to dest in a data store.
// -----------------------------------------------------------------------------
template <typename T>
void copyTuple(AbstractDataStore<T>& store, int64 dest, int64 src)
{
  const usize numComp = store.getNumberOfComponents();
  for(usize c = 0; c < numComp; c++)
  {
    store[dest * numComp + c] = store[src * numComp + c];
  }
}

// Functor for type-dispatched single-tuple copy
struct CopyTupleFunctor
{
  template <typename T>
  void operator()(IDataArray* dataArray, int64 dest, int64 src)
  {
    auto& store = dataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    copyTuple(store, dest, src);
  }
};

// RAII wrapper for std::FILE* that auto-closes on destruction
struct TempFileGuard
{
  std::FILE* file = nullptr;

  TempFileGuard() = default;
  ~TempFileGuard()
  {
    if(file != nullptr)
    {
      std::fclose(file);
    }
  }

  TempFileGuard(const TempFileGuard&) = delete;
  TempFileGuard& operator=(const TempFileGuard&) = delete;
};
} // namespace

// =============================================================================
// FillBadData Implementation
// =============================================================================

FillBadDataCCL::FillBadDataCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadDataCCL::~FillBadDataCCL() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FillBadDataCCL::getCancel() const
{
  return m_ShouldCancel;
}

// =============================================================================
// PHASE 1: Chunk-Sequential Connected Component Labeling (CCL)
// =============================================================================
//
// Performs connected component labeling on bad data voxels (FeatureId == 0)
// using a chunk-sequential scanline algorithm. Uses positive labels and an
// in-memory provisional labels buffer to avoid cross-chunk OOC reads.
//
// @param featureIdsStore The feature IDs data store (maybe out-of-core)
// @param unionFind Union-Find structure for tracking label equivalences
// @param nextLabel Next label to assign (incremented as new labels are created)
// @param dims Image dimensions [X, Y, Z]
// =============================================================================
void FillBadDataCCL::phaseOneCCL(Int32AbstractDataStore& featureIdsStore, UnionFind& unionFind, int32& nextLabel, const std::array<int64, 3>& dims)
{
  const uint64 numChunks = featureIdsStore.getNumberOfChunks();
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Rolling 2-slice buffer for backward neighbor label reads.
  // Only current + previous Z-slice are needed. O(slice) memory.
  std::vector<int32> labelBuffer(2 * sliceSize, 0);

  // Track last cleared Z-slice to avoid re-clearing when a Z-slice spans multiple chunks
  int64 lastClearedZ = -1;

  // Process each chunk sequentially
  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    featureIdsStore.loadChunk(chunkIdx);

    const auto chunkLowerBounds = featureIdsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = featureIdsStore.getChunkUpperBounds(chunkIdx);

    for(usize z = chunkLowerBounds[0]; z <= chunkUpperBounds[0]; z++)
    {
      // Clear current slice in rolling buffer only when entering a NEW z value.
      // A single Z-slice may span multiple chunks, so we must not re-clear
      // data written by a previous chunk for the same z.
      const usize curOff = (z % 2) * sliceSize;
      if(static_cast<int64>(z) != lastClearedZ)
      {
        std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
        lastClearedZ = static_cast<int64>(z);
      }
      const usize prevOff = ((z + 1) % 2) * sliceSize;

      for(usize y = chunkLowerBounds[1]; y <= chunkUpperBounds[1]; y++)
      {
        for(usize x = chunkLowerBounds[2]; x <= chunkUpperBounds[2]; x++)
        {
          const usize index = z * sliceSize + y * static_cast<usize>(dims[0]) + x;
          const usize inSlice = y * static_cast<usize>(dims[0]) + x;

          // Only process bad data voxels (FeatureId == 0)
          if(featureIdsStore[index] != 0)
          {
            continue;
          }

          // Check backward neighbors using rolling buffer
          int32 assignedLabel = 0;

          if(x > 0)
          {
            int32 neighLabel = labelBuffer[curOff + inSlice - 1];
            if(neighLabel > 0)
            {
              assignedLabel = neighLabel;
            }
          }

          if(y > 0)
          {
            int32 neighLabel = labelBuffer[curOff + inSlice - static_cast<usize>(dims[0])];
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

          if(z > 0)
          {
            int32 neighLabel = labelBuffer[prevOff + inSlice];
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

          if(assignedLabel == 0)
          {
            assignedLabel = nextLabel++;
            unionFind.find(assignedLabel);
          }

          // Write to rolling buffer AND featureIds store
          labelBuffer[curOff + inSlice] = assignedLabel;
          featureIdsStore[index] = assignedLabel;

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
void FillBadDataCCL::phaseTwoGlobalResolution(UnionFind& unionFind)
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
// =============================================================================
void FillBadDataCCL::phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, int32 startLabel, int32 nextLabel, UnionFind& unionFind, usize maxPhase) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();
  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  // Build a vector-based classification: isSmallRoot[label] = 1 if small, 0 if large
  // Only provisional labels [startLabel, nextLabel) are CCL labels; others are original feature IDs.
  std::vector<int8> isSmallRoot(static_cast<usize>(nextLabel), 0);
  for(int32 label = startLabel; label < nextLabel; label++)
  {
    int64 root = unionFind.find(label);
    if(root == label)
    {
      uint64 regionSize = unionFind.getSize(root);
      if(static_cast<int32>(regionSize) < m_InputValues->minAllowedDefectSizeValue)
      {
        isSmallRoot[root] = 1;
      }
    }
  }

  // Read provisional labels from featureIds store (written during Phase 1)
  // and relabel based on region classification.
  // Only voxels with label >= startLabel are provisional CCL labels (bad data).
  // Voxels with label in [1, startLabel) are original good feature IDs — leave them alone.
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

          int32 label = featureIdsStore[index];
          if(label >= startLabel)
          {
            int64 root = unionFind.find(label);

            if(isSmallRoot[root] != 0)
            {
              featureIdsStore[index] = -1;
            }
            else
            {
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
// PHASE 4: Iterative Morphological Fill (On-Disk Deferred)
// =============================================================================
//
// Uses a temporary file to avoid O(N) memory allocations. Each iteration:
//   Pass 1 (Vote): Scan voxels chunk-sequentially. For each -1 voxel, find the
//     best positive-featureId neighbor via majority vote. Write (dest, src) pairs
//     to a temp file. featureIds is read-only during this pass.
//   Pass 2 (Apply): Read pairs back from the temp file. Copy all cell data array
//     components from src to dest. Update featureIds last.
// =============================================================================
void FillBadDataCCL::phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64, 3>& dims, usize numFeatures) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // Feature vote counter: O(features) not O(voxels)
  std::vector<int32> featureNumber(numFeatures + 1, 0);

  // Get cell arrays that need updating during filling
  std::optional<std::vector<DataPath>> allChildArrays = GetAllChildDataPaths(m_DataStructure, selectedImageGeom.getCellDataPath(), DataObject::Type::DataArray, m_InputValues->ignoredDataArrayPaths);
  std::vector<DataPath> voxelArrayNames;
  if(allChildArrays.has_value())
  {
    voxelArrayNames = allChildArrays.value();
  }

  // Open temp file for deferred fill pairs
  TempFileGuard tmpGuard;
  tmpGuard.file = std::tmpfile();
  if(tmpGuard.file == nullptr)
  {
    m_MessageHandler({IFilter::Message::Type::Error, "Phase 4/4: Failed to create temporary file for deferred fill"});
    return;
  }

  MessageHelper messageHelper(m_MessageHandler, std::chrono::milliseconds(1000));
  auto throttledMessenger = messageHelper.createThrottledMessenger(std::chrono::milliseconds(1000));

  usize count = 1;
  usize iteration = 0;
  const uint64 numChunks = featureIdsStore.getNumberOfChunks();

  while(count != 0)
  {
    iteration++;
    count = 0;

    // Truncate temp file for this iteration
    std::rewind(tmpGuard.file);

    // Pass 1 (Vote): Chunk-sequential scan writing (dest, src) pairs to temp file.
    // featureIds is read-only during this pass — two-pass semantics are automatic.
    for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
    {
      featureIdsStore.loadChunk(chunkIdx);
      const auto lower = featureIdsStore.getChunkLowerBounds(chunkIdx);
      const auto upper = featureIdsStore.getChunkUpperBounds(chunkIdx);

      for(usize z = lower[0]; z <= upper[0]; z++)
      {
        for(usize y = lower[1]; y <= upper[1]; y++)
        {
          for(usize x = lower[2]; x <= upper[2]; x++)
          {
            const int64 voxelIndex = static_cast<int64>(z) * dims[0] * dims[1] + static_cast<int64>(y) * dims[0] + static_cast<int64>(x);
            int32 featureName = featureIdsStore[voxelIndex];

            if(featureName < 0)
            {
              count++;
              int32 most = 0;
              int64 bestNeighbor = -1;

              std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(static_cast<int64>(x), static_cast<int64>(y), static_cast<int64>(z), dims);
              for(const auto& faceIndex : faceNeighborInternalIdx)
              {
                if(!isValidFaceNeighbor[faceIndex])
                {
                  continue;
                }

                auto neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
                int32 feature = featureIdsStore[neighborPoint];

                if(feature > 0)
                {
                  featureNumber[feature]++;
                  int32 current = featureNumber[feature];
                  if(current > most)
                  {
                    most = current;
                    bestNeighbor = neighborPoint;
                  }
                }
              }

              // Reset vote counters
              for(const auto& faceIndex : faceNeighborInternalIdx)
              {
                if(!isValidFaceNeighbor[faceIndex])
                {
                  continue;
                }
                auto neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
                int32 feature = featureIdsStore[neighborPoint];
                if(feature > 0)
                {
                  featureNumber[feature] = 0;
                }
              }

              // Write (dest, src) pair to temp file if a valid neighbor was found
              if(bestNeighbor >= 0)
              {
                std::array<int64, 2> pair = {voxelIndex, bestNeighbor};
                std::fwrite(pair.data(), sizeof(int64), 2, tmpGuard.file);
              }
            }
          }
        }
      }
    }

    if(count == 0)
    {
      break;
    }

    // Pass 2 (Apply): Read (dest, src) pairs from temp file and apply fills.
    // Update all cell arrays except featureIds first, then featureIds last.
    std::rewind(tmpGuard.file);
    std::array<int64, 2> pair;

    // First pass over pairs: update all non-featureIds cell arrays
    while(std::fread(pair.data(), sizeof(int64), 2, tmpGuard.file) == 2)
    {
      int64 dest = pair[0];
      int64 src = pair[1];

      for(const auto& cellArrayPath : voxelArrayNames)
      {
        if(cellArrayPath == m_InputValues->featureIdsArrayPath)
        {
          continue;
        }
        auto* cellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);
        ExecuteDataFunction(CopyTupleFunctor{}, cellArray->getDataType(), cellArray, dest, src);
      }
    }

    // Second pass over pairs: update featureIds last
    std::rewind(tmpGuard.file);
    while(std::fread(pair.data(), sizeof(int64), 2, tmpGuard.file) == 2)
    {
      int64 dest = pair[0];
      int64 src = pair[1];
      featureIdsStore[dest] = featureIdsStore[src];
    }

    featureIdsStore.flush();

    throttledMessenger.sendThrottledMessage([iteration, count]() { return fmt::format("  Iteration {}: {} voxels remaining to fill", iteration, count); });
  }

  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("  Completed in {} iteration{}", iteration, iteration == 1 ? "" : "s")});
}

// =============================================================================
// Main Algorithm Entry Point
// =============================================================================
Result<> FillBadDataCCL::operator()() const
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

  // Get cell phases array if we need to assign large regions to a new phase
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

  // Initialize data structures for connected component labeling.
  // Start provisional labels AFTER the max existing feature ID to avoid collisions.
  // Existing feature IDs are in [1, numFeatures], so provisional labels start at numFeatures+1.
  UnionFind unionFind;
  const int32 startLabel = static_cast<int32>(numFeatures) + 1;
  int32 nextLabel = startLabel;

  // Phase 1: Chunk-Sequential Connected Component Labeling
  // Uses a 2-slice rolling buffer (O(slice) memory) for backward neighbor reads.
  // Writes provisional labels to featureIds store for Phases 2-3.
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 1/4: Labeling connected components..."});
  phaseOneCCL(featureIdsStore, unionFind, nextLabel, dims);

  // Phase 2: Global Resolution of equivalences
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 2/4: Resolving region equivalences..."});
  phaseTwoGlobalResolution(unionFind);

  // Phase 3: Relabeling based on region size classification
  // Reads provisional labels from featureIds store (written during Phase 1)
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 3/4: Classifying region sizes..."});
  phaseThreeRelabeling(featureIdsStore, cellPhasesPtr, startLabel, nextLabel, unionFind, maxPhase);

  // Phase 4: Iterative morphological fill
  m_MessageHandler({IFilter::Message::Type::Info, "Phase 4/4: Filling small defects..."});
  phaseFourIterativeFill(featureIdsStore, dims, numFeatures);

  return {};
}
