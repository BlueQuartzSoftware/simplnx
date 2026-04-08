#include "FillBadDataCCL.hpp"

#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <cstdio>
#include <memory>

using namespace nx::core;

// -----------------------------------------------------------------------------
// FillBadData Algorithm Overview
// -----------------------------------------------------------------------------
//
// This file implements an optimized algorithm for filling bad data (voxels with
// FeatureId == 0) in image geometries. The algorithm handles out-of-core datasets
// efficiently by processing data in Z-slice buffers and uses a four-phase approach:
//
// Phase 1: Z-Slice Sequential Connected Component Labeling (CCL)
//   - Process Z-slices sequentially, assigning provisional labels to bad data regions
//   - Use Union-Find to track equivalences between labels across slice boundaries
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
// -----------------------------------------------------------------------------

namespace
{
// -----------------------------------------------------------------------------
// Helper: Copy all components of a single tuple from src to dest in a data store.
// -----------------------------------------------------------------------------
template <typename T>
void copyTuple(AbstractDataStore<T>& store, int64 dest, int64 src)
{
  const usize numComp = store.getNumberOfComponents();
  auto buffer = std::make_unique<T[]>(numComp);
  store.copyIntoBuffer(static_cast<usize>(src) * numComp, nonstd::span<T>(buffer.get(), numComp));
  store.copyFromBuffer(static_cast<usize>(dest) * numComp, nonstd::span<const T>(buffer.get(), numComp));
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

// Functor for type-dispatched slice-buffered copy of all pairs for one array.
// Uses a 3-slice rolling window to apply fills with bulk I/O.
struct SliceBufferedCopyFunctor
{
  template <typename T>
  void operator()(IDataArray* dataArray, const std::vector<std::array<int64, 2>>& allPairs, usize pairsWritten, usize sliceTuples, int64 sliceStride, int64 dimZ)
  {
    if constexpr(std::is_same_v<T, bool>)
    {
      // std::vector<bool> doesn't support .data() — fall back to per-tuple copy.
      // Bool cell arrays are extremely rare in practice.
      auto& store = dataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
      for(usize pi = 0; pi < pairsWritten; pi++)
      {
        copyTuple(store, allPairs[pi][0], allPairs[pi][1]);
      }
    }
    else
    {
      auto& store = dataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
      const usize numComp = store.getNumberOfComponents();
      const usize sliceValues = sliceTuples * numComp;

      std::vector<T> buf(3 * sliceValues);
      int64 curZ = -2;

      for(usize pi = 0; pi < pairsWritten; pi++)
      {
        const int64 dest = allPairs[pi][0];
        const int64 src = allPairs[pi][1];
        const int64 dz = dest / sliceStride;

        if(dz != curZ)
        {
          // Write back previous current slice
          if(curZ >= 0)
          {
            store.copyFromBuffer(static_cast<usize>(curZ) * sliceValues, nonstd::span<const T>(buf.data() + sliceValues, sliceValues));
          }
          curZ = dz;
          if(curZ > 0)
          {
            store.copyIntoBuffer(static_cast<usize>(curZ - 1) * sliceValues, nonstd::span<T>(buf.data(), sliceValues));
          }
          store.copyIntoBuffer(static_cast<usize>(curZ) * sliceValues, nonstd::span<T>(buf.data() + sliceValues, sliceValues));
          if(curZ + 1 < dimZ)
          {
            store.copyIntoBuffer(static_cast<usize>(curZ + 1) * sliceValues, nonstd::span<T>(buf.data() + 2 * sliceValues, sliceValues));
          }
        }

        const usize destInSlice = static_cast<usize>(dest - dz * sliceStride);
        const int64 sz = src / sliceStride;
        const usize srcInSlice = static_cast<usize>(src - sz * sliceStride);
        const usize srcSlot = (sz == curZ - 1) ? 0 : (sz == curZ) ? 1 : 2;

        // Copy all components from src to dest
        for(usize c = 0; c < numComp; c++)
        {
          buf[sliceValues + destInSlice * numComp + c] = buf[srcSlot * sliceValues + srcInSlice * numComp + c];
        }
      }
      // Write back final slice
      if(curZ >= 0)
      {
        store.copyFromBuffer(static_cast<usize>(curZ) * sliceValues, nonstd::span<const T>(buf.data() + sliceValues, sliceValues));
      }
    }
  }
};

// RAII wrapper for std::FILE* that guarantees cleanup of the temporary file
// on destruction. This ensures the temp file is closed (and thus deleted by
// the OS, since std::tmpfile creates an anonymous file) even if Phase 4
// returns early due to cancellation or error. Copy/assignment are deleted
// to enforce single-ownership semantics.
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

// -----------------------------------------------------------------------------
// FillBadData Implementation
// -----------------------------------------------------------------------------

FillBadDataCCL::FillBadDataCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues)
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

// -----------------------------------------------------------------------------
// PHASE 1: Z-Slice Sequential Connected Component Labeling (CCL)
// -----------------------------------------------------------------------------
//
// Performs connected component labeling on bad data voxels (FeatureId == 0)
// using a Z-slice sequential scanline algorithm. Uses positive labels and an
// in-memory provisional labels buffer to avoid cross-slice OOC reads.
//
// @param featureIdsStore The feature IDs data store (maybe out-of-core)
// @param unionFind Union-Find structure for tracking label equivalences
// @param nextLabel Next label to assign (incremented as new labels are created)
// @param dims Image dimensions [X, Y, Z]
// -----------------------------------------------------------------------------
void FillBadDataCCL::phaseOneCCL(Int32AbstractDataStore& featureIdsStore, UnionFind& unionFind, int32& nextLabel, const std::array<int64, 3>& dims)
{
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Rolling 2-slice buffer for backward neighbor label reads.
  // The scanline CCL algorithm only needs to look at three backward neighbors:
  // x-1 (same slice), y-1 (same slice), and z-1 (previous slice). So we only
  // need the current and immediately previous Z-slice labels in memory. The
  // buffer alternates between even/odd Z indices via (z % 2) indexing.
  // This gives O(dimX * dimY) memory instead of O(volume).
  std::vector<int32> labelBuffer(2 * sliceSize, 0);

  // Temporary buffer for reading/writing featureIds one Z-slice at a time
  std::vector<int32> featureIdsSlice(sliceSize);

  // Process each Z-slice sequentially
  for(usize z = 0; z < static_cast<usize>(dims[2]); z++)
  {
    featureIdsStore.copyIntoBuffer(z * sliceSize, nonstd::span<int32>(featureIdsSlice.data(), sliceSize));

    // Clear current slice in rolling label buffer for this z
    const usize curOff = (z % 2) * sliceSize;
    std::fill(labelBuffer.begin() + curOff, labelBuffer.begin() + curOff + sliceSize, 0);
    const usize prevOff = ((z + 1) % 2) * sliceSize;

    for(usize y = 0; y < static_cast<usize>(dims[1]); y++)
    {
      for(usize x = 0; x < static_cast<usize>(dims[0]); x++)
      {
        const usize inSlice = y * static_cast<usize>(dims[0]) + x;

        // Only process bad data voxels (FeatureId == 0)
        if(featureIdsSlice[inSlice] != 0)
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

        // Write the provisional label to both the rolling buffer (for
        // backward neighbor reads by subsequent voxels) and the featureIds
        // slice buffer (persisted for Phases 2-3 to read back).
        labelBuffer[curOff + inSlice] = assignedLabel;
        featureIdsSlice[inSlice] = assignedLabel;

        // Accumulate region size: each voxel contributes 1 to its label.
        // After Phase 2 flattening, sizes are aggregated to root labels
        // so we can classify regions by total voxel count.
        unionFind.addSize(assignedLabel, 1);
      }
    }

    featureIdsStore.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(featureIdsSlice.data(), sliceSize));
  }
}

// -----------------------------------------------------------------------------
// PHASE 2: Global Resolution of Equivalences
// -----------------------------------------------------------------------------
void FillBadDataCCL::phaseTwoGlobalResolution(UnionFind& unionFind)
{
  unionFind.flatten();
}

// -----------------------------------------------------------------------------
// PHASE 3: Region Classification and Relabeling
// -----------------------------------------------------------------------------
//
// Classifies bad data regions as "small" or "large" based on size threshold:
// - Small regions (< minAllowedDefectSize): marked with -1 for filling in Phase 4
// - Large regions (>= minAllowedDefectSize): kept as 0 (or assigned new phase)
// -----------------------------------------------------------------------------
void FillBadDataCCL::phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, int32 startLabel, int32 nextLabel, UnionFind& unionFind, usize maxPhase) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);
  const SizeVec3 udims = selectedImageGeom.getDimensions();

  // Build a vector-based classification: isSmallRoot[label] = 1 if small, 0 if large.
  //
  // The startLabel boundary is critical: provisional CCL labels were assigned
  // starting at (maxExistingFeatureId + 1) during Phase 1, so labels in the
  // range [1, startLabel) are original good feature IDs that must NOT be
  // touched. Only labels in [startLabel, nextLabel) are CCL-assigned bad-data
  // region labels that need classification and relabeling.
  std::vector<int8> isSmallRoot(static_cast<usize>(nextLabel), 0);
  for(int32 label = startLabel; label < nextLabel; label++)
  {
    int64 root = unionFind.find(label);
    if(root == label)
    {
      uint64 regionSize = unionFind.getSize(root);
      if(regionSize < static_cast<uint64>(m_InputValues->minAllowedDefectSizeValue))
      {
        isSmallRoot[root] = 1;
      }
    }
  }

  // Temporary buffer for reading/writing featureIds one Z-slice at a time
  std::vector<int32> sliceData(static_cast<usize>(udims[0]) * static_cast<usize>(udims[1]));
  const usize sliceSize = sliceData.size();

  // Optional cellPhases buffer for bulk read/write (avoids per-element OOC access)
  const bool needPhasesBuffer = m_InputValues->storeAsNewPhase && cellPhasesPtr != nullptr;
  std::vector<int32> phasesSlice;
  Int32AbstractDataStore* cellPhasesStorePtr = nullptr;
  if(needPhasesBuffer)
  {
    phasesSlice.resize(sliceSize);
    cellPhasesStorePtr = &cellPhasesPtr->getDataStoreRef();
  }

  // Read provisional labels from featureIds store (written during Phase 1)
  // and relabel based on region classification.
  // Only voxels with label >= startLabel are provisional CCL labels (bad data).
  // Voxels with label in [1, startLabel) are original good feature IDs — leave them alone.
  for(usize z = 0; z < udims[2]; z++)
  {
    featureIdsStore.copyIntoBuffer(z * sliceSize, nonstd::span<int32>(sliceData.data(), sliceSize));

    bool phasesModified = false;
    if(needPhasesBuffer)
    {
      cellPhasesStorePtr->copyIntoBuffer(z * sliceSize, nonstd::span<int32>(phasesSlice.data(), sliceSize));
    }

    for(usize y = 0; y < udims[1]; y++)
    {
      for(usize x = 0; x < udims[0]; x++)
      {
        const usize inSlice = y * udims[0] + x;

        int32 label = sliceData[inSlice];
        if(label >= startLabel)
        {
          int64 root = unionFind.find(label);

          if(isSmallRoot[root] != 0)
          {
            sliceData[inSlice] = -1;
          }
          else
          {
            sliceData[inSlice] = 0;

            if(needPhasesBuffer)
            {
              phasesSlice[inSlice] = static_cast<int32>(maxPhase) + 1;
              phasesModified = true;
            }
          }
        }
      }
    }

    featureIdsStore.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(sliceData.data(), sliceSize));
    if(phasesModified)
    {
      cellPhasesStorePtr->copyFromBuffer(z * sliceSize, nonstd::span<const int32>(phasesSlice.data(), sliceSize));
    }
  }
}

// -----------------------------------------------------------------------------
// PHASE 4: Iterative Morphological Fill (On-Disk Deferred)
// -----------------------------------------------------------------------------
//
// Uses a temporary file to avoid O(N) memory allocations. Each iteration:
//   Pass 1 (Vote): Scan voxels using a 3-slice rolling window. For each -1 voxel,
//     find the best positive-featureId neighbor via majority vote. Write (dest, src)
//     pairs to a temp file. featureIds is read-only during this pass.
//   Pass 2 (Apply): Read pairs back from the temp file. Copy all cell data array
//     components from src to dest. Update featureIds last.
// -----------------------------------------------------------------------------
Result<> FillBadDataCCL::phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64, 3>& dims, usize numFeatures) const
{
  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);

  // Feature vote counter: O(features) not O(voxels)
  std::vector<int32> featureNumber(numFeatures + 1, 0);

  // Get cell arrays that need updating during filling
  std::optional<std::vector<DataPath>> allChildArrays = GetAllChildDataPaths(m_DataStructure, selectedImageGeom.getCellDataPath(), DataObject::Type::DataArray, m_InputValues->ignoredDataArrayPaths);
  std::vector<DataPath> voxelArrayNames;
  if(allChildArrays.has_value())
  {
    voxelArrayNames = allChildArrays.value();
  }

  // Open a temporary file for deferred fill pairs. We use a temp file instead
  // of an O(N) in-memory neighbors vector so that Phase 4 stays OOC-friendly.
  // Pass 1 writes (dest, src) index pairs to the file; Pass 2 reads them back
  // and applies the fills. This two-pass approach ensures that featureIds are
  // read-only during the vote scan (Pass 1), so all votes see the pre-iteration
  // state. The TempFileGuard RAII wrapper guarantees the file is closed even
  // if an early return or error occurs, preventing temp file leaks.
  TempFileGuard tmpGuard;
  tmpGuard.file = std::tmpfile();
  if(tmpGuard.file == nullptr)
  {
    return MakeErrorResult(-87010, "Phase 4/4: Failed to create temporary file for deferred fill");
  }

  usize count = 1;
  usize iteration = 0;
  usize pairsWritten = 0;

  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  while(count != 0)
  {
    iteration++;
    count = 0;

    // Rewind for this iteration's writes
    std::rewind(tmpGuard.file);
    pairsWritten = 0;

    // Pass 1 (Vote): Z-slice scan using a 3-slice rolling window, writing
    // (dest, src) pairs to temp file. featureIds is read-only during this
    // pass — two-pass semantics are automatic.
    std::vector<int32> prevSlice(sliceSize, 0);
    std::vector<int32> curSlice(sliceSize);
    std::vector<int32> nextSlice(sliceSize, 0);

    featureIdsStore.copyIntoBuffer(0, nonstd::span<int32>(curSlice.data(), sliceSize));
    if(dims[2] > 1)
    {
      featureIdsStore.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
    }

    for(int64 z = 0; z < dims[2]; z++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      for(int64 y = 0; y < dims[1]; y++)
      {
        for(int64 x = 0; x < dims[0]; x++)
        {
          const usize inSlice = static_cast<usize>(y) * static_cast<usize>(dims[0]) + static_cast<usize>(x);
          int32 featureName = curSlice[inSlice];

          if(featureName < 0)
          {
            count++;
            int32 most = 0;
            int64 bestNeighbor = -1;

            // Check 6 face neighbors using the 3-slice window
            // -X neighbor
            if(x > 0)
            {
              int32 feature = curSlice[inSlice - 1];
              if(feature > 0)
              {
                featureNumber[feature]++;
                if(featureNumber[feature] > most)
                {
                  most = featureNumber[feature];
                  bestNeighbor = z * dims[0] * dims[1] + y * dims[0] + (x - 1);
                }
              }
            }
            // +X neighbor
            if(x < dims[0] - 1)
            {
              int32 feature = curSlice[inSlice + 1];
              if(feature > 0)
              {
                featureNumber[feature]++;
                if(featureNumber[feature] > most)
                {
                  most = featureNumber[feature];
                  bestNeighbor = z * dims[0] * dims[1] + y * dims[0] + (x + 1);
                }
              }
            }
            // -Y neighbor
            if(y > 0)
            {
              int32 feature = curSlice[inSlice - static_cast<usize>(dims[0])];
              if(feature > 0)
              {
                featureNumber[feature]++;
                if(featureNumber[feature] > most)
                {
                  most = featureNumber[feature];
                  bestNeighbor = z * dims[0] * dims[1] + (y - 1) * dims[0] + x;
                }
              }
            }
            // +Y neighbor
            if(y < dims[1] - 1)
            {
              int32 feature = curSlice[inSlice + static_cast<usize>(dims[0])];
              if(feature > 0)
              {
                featureNumber[feature]++;
                if(featureNumber[feature] > most)
                {
                  most = featureNumber[feature];
                  bestNeighbor = z * dims[0] * dims[1] + (y + 1) * dims[0] + x;
                }
              }
            }
            // -Z neighbor
            if(z > 0)
            {
              int32 feature = prevSlice[inSlice];
              if(feature > 0)
              {
                featureNumber[feature]++;
                if(featureNumber[feature] > most)
                {
                  most = featureNumber[feature];
                  bestNeighbor = (z - 1) * dims[0] * dims[1] + y * dims[0] + x;
                }
              }
            }
            // +Z neighbor
            if(z < dims[2] - 1)
            {
              int32 feature = nextSlice[inSlice];
              if(feature > 0)
              {
                featureNumber[feature]++;
                if(featureNumber[feature] > most)
                {
                  most = featureNumber[feature];
                  bestNeighbor = (z + 1) * dims[0] * dims[1] + y * dims[0] + x;
                }
              }
            }

            // Reset vote counters by re-visiting only the neighbors that
            // were actually incremented above. This sets featureNumber[feature]
            // back to 0 for each neighbor's feature, avoiding the need to zero
            // the entire featureNumber vector (which would be O(numFeatures)
            // per voxel). Since at most 6 neighbors are visited, this reset
            // is O(1) per voxel.
            if(x > 0)
            {
              int32 f = curSlice[inSlice - 1];
              if(f > 0)
              {
                featureNumber[f] = 0;
              }
            }
            if(x < dims[0] - 1)
            {
              int32 f = curSlice[inSlice + 1];
              if(f > 0)
              {
                featureNumber[f] = 0;
              }
            }
            if(y > 0)
            {
              int32 f = curSlice[inSlice - static_cast<usize>(dims[0])];
              if(f > 0)
              {
                featureNumber[f] = 0;
              }
            }
            if(y < dims[1] - 1)
            {
              int32 f = curSlice[inSlice + static_cast<usize>(dims[0])];
              if(f > 0)
              {
                featureNumber[f] = 0;
              }
            }
            if(z > 0)
            {
              int32 f = prevSlice[inSlice];
              if(f > 0)
              {
                featureNumber[f] = 0;
              }
            }
            if(z < dims[2] - 1)
            {
              int32 f = nextSlice[inSlice];
              if(f > 0)
              {
                featureNumber[f] = 0;
              }
            }

            // Write (dest, src) pair to temp file if a valid neighbor was found
            if(bestNeighbor >= 0)
            {
              std::array<int64, 2> pair = {z * dims[0] * dims[1] + y * dims[0] + x, bestNeighbor};
              if(std::fwrite(pair.data(), sizeof(int64), 2, tmpGuard.file) != 2)
              {
                return MakeErrorResult(-87012, "Phase 4/4: Failed to write fill pair to temporary file");
              }
              pairsWritten++;
            }
          }
        }
      }

      // Shift 3-slice window forward
      std::swap(prevSlice, curSlice);
      std::swap(curSlice, nextSlice);
      if(z + 2 < dims[2])
      {
        featureIdsStore.copyIntoBuffer(static_cast<usize>(z + 2) * sliceSize, nonstd::span<int32>(nextSlice.data(), sliceSize));
      }
    }

    if(count == 0)
    {
      break;
    }

    // Pass 2 (Apply): Read all pairs from temp file, then apply fills using
    // slice-buffered I/O per array. Pairs are in Z-Y-X order, so each array
    // is processed with a 3-slice rolling window. This converts millions of
    // per-tuple OOC accesses into ~600 bulk slice reads/writes per array.

    // Read all pairs into memory (788K pairs × 16 bytes = ~12 MB — acceptable)
    std::vector<std::array<int64, 2>> allPairs(pairsWritten);
    std::rewind(tmpGuard.file);
    for(usize i = 0; i < pairsWritten; i++)
    {
      std::fread(allPairs[i].data(), sizeof(int64), 2, tmpGuard.file);
    }

    const int64 sliceStride = dims[0] * dims[1];
    const usize sliceTuples = sliceSize;

    // Apply featureIds fills using 3-slice buffer
    {
      std::vector<int32> fidBuf(3 * sliceTuples, 0);
      int64 curZ = -2;

      for(usize pi = 0; pi < pairsWritten; pi++)
      {
        const int64 dest = allPairs[pi][0];
        const int64 src = allPairs[pi][1];
        const int64 dz = dest / sliceStride;

        if(dz != curZ)
        {
          // Write back previous current slice
          if(curZ >= 0)
          {
            featureIdsStore.copyFromBuffer(static_cast<usize>(curZ) * sliceTuples, nonstd::span<const int32>(fidBuf.data() + sliceTuples, sliceTuples));
          }
          curZ = dz;
          // Load 3-slice window: [prev | cur | next]
          if(curZ > 0)
          {
            featureIdsStore.copyIntoBuffer(static_cast<usize>(curZ - 1) * sliceTuples, nonstd::span<int32>(fidBuf.data(), sliceTuples));
          }
          featureIdsStore.copyIntoBuffer(static_cast<usize>(curZ) * sliceTuples, nonstd::span<int32>(fidBuf.data() + sliceTuples, sliceTuples));
          if(curZ + 1 < dims[2])
          {
            featureIdsStore.copyIntoBuffer(static_cast<usize>(curZ + 1) * sliceTuples, nonstd::span<int32>(fidBuf.data() + 2 * sliceTuples, sliceTuples));
          }
        }

        const usize destInSlice = static_cast<usize>(dest - dz * sliceStride);
        const int64 sz = src / sliceStride;
        const usize srcInSlice = static_cast<usize>(src - sz * sliceStride);
        const usize srcSlot = (sz == curZ - 1) ? 0 : (sz == curZ) ? 1 : 2;

        fidBuf[sliceTuples + destInSlice] = fidBuf[srcSlot * sliceTuples + srcInSlice];
      }
      // Write back final slice
      if(curZ >= 0)
      {
        featureIdsStore.copyFromBuffer(static_cast<usize>(curZ) * sliceTuples, nonstd::span<const int32>(fidBuf.data() + sliceTuples, sliceTuples));
      }
    }

    // Apply non-featureIds cell array fills, one array at a time
    for(const auto& cellArrayPath : voxelArrayNames)
    {
      if(cellArrayPath == m_InputValues->featureIdsArrayPath)
      {
        continue;
      }
      auto* cellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);
      // Type-dispatched slice-buffered fill
      ExecuteDataFunction(SliceBufferedCopyFunctor{}, cellArray->getDataType(), cellArray, allPairs, pairsWritten, sliceTuples, sliceStride, dims[2]);
    }

    featureIdsStore.flush();
  }

  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("  Completed in {} iteration{}", iteration, iteration == 1 ? "" : "s")});
  return {};
}

// -----------------------------------------------------------------------------
// Main Algorithm Entry Point
// -----------------------------------------------------------------------------
Result<> FillBadDataCCL::operator()()
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
  }

  // Chunked scan: find max feature ID and optionally max phase using bulk reads
  usize numFeatures = 0;
  const usize k_ScanBatchSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);
  std::vector<int32> scanBuffer(k_ScanBatchSize);
  for(usize offset = 0; offset < totalPoints; offset += k_ScanBatchSize)
  {
    const usize batchSize = std::min(k_ScanBatchSize, totalPoints - offset);
    featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(scanBuffer.data(), batchSize));
    for(usize i = 0; i < batchSize; i++)
    {
      if(scanBuffer[i] > static_cast<int32>(numFeatures))
      {
        numFeatures = scanBuffer[i];
      }
    }
  }
  // Separate chunked scan for cellPhases if needed
  if(cellPhasesPtr != nullptr)
  {
    auto& cellPhasesStore = cellPhasesPtr->getDataStoreRef();
    for(usize offset = 0; offset < totalPoints; offset += k_ScanBatchSize)
    {
      const usize batchSize = std::min(k_ScanBatchSize, totalPoints - offset);
      cellPhasesStore.copyIntoBuffer(offset, nonstd::span<int32>(scanBuffer.data(), batchSize));
      for(usize i = 0; i < batchSize; i++)
      {
        if(static_cast<usize>(scanBuffer[i]) > maxPhase)
        {
          maxPhase = scanBuffer[i];
        }
      }
    }
  }

  // Initialize data structures for connected component labeling.
  // Start provisional labels AFTER the max existing feature ID to avoid collisions.
  // Existing feature IDs are in [1, numFeatures], so provisional labels start at numFeatures+1.
  UnionFind unionFind;
  const int32 startLabel = static_cast<int32>(numFeatures) + 1;
  int32 nextLabel = startLabel;

  // Phase 1: Z-Slice Sequential Connected Component Labeling
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
  auto result = phaseFourIterativeFill(featureIdsStore, dims, numFeatures);
  return result;
}
