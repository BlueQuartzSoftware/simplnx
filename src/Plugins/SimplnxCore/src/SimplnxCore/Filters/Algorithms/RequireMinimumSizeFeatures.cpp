#include "RequireMinimumSizeFeatures.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/TimeUtilities.hpp"

#include <nonstd/span.hpp>

#include <memory>

using namespace nx::core;

namespace
{
// Chunk size for bulk-I/O scans of FeatureIds in removeSmallFeatures(). 65536 tuples balances
// HDF5 chunk-op amortization against transient RAM (64K * sizeof(int32) = 256 KB per task).
constexpr usize k_ChunkTuples = 65536;

// Memory budget for one per-array transfer slab. At 64 MB and typical CT volumes a single slab
// covers several Z-slices at once, so a batch handles thousands of bad voxels in one HDF5 read +
// write pair instead of one per voxel. With ParallelTaskAlgorithm running one task per cell-level
// array, peak transient memory is (num_parallel_tasks * k_TransferSlabBudgetBytes), typically in
// the low hundreds of MB.
constexpr usize k_TransferSlabBudgetBytes = 64 * 1024 * 1024;

// -----------------------------------------------------------------------------
// ChunkedTransferWorker
//
// Type-dispatched worker that applies the "copy neighbor's tuple to bad voxel" action for ALL
// bad voxels of a single cell-level array, using chunked Z-slab bulk I/O (copyIntoBuffer +
// copyFromBuffer) instead of per-voxel copyTuple().
//
// Safety w.r.t. parallelism across arrays: Each task operates on a distinct IDataArray, so the
// outer ParallelTaskAlgorithm is safe even though each DataStore is not internally thread-safe
// — different threads never touch the same DataStore.
//
// Correctness w.r.t. neighbor reads: The scan phase invariant guarantees that every recorded
// neighbor voxel has a non-negative feature ID (i.e., it's not a bad voxel). Since bad voxels
// are the only ones we mutate, no pair's neighbor index points at another bad voxel — therefore
// there is no read/write ordering dependency between pairs within a batch, and we can do in-slab
// in-place updates safely.
//
// Read range vs. write range: Neighbor offsets are ±1, ±Dx, ±(Dx*Dy), so a neighbor's Z can be
// at most 1 slice away from its voxel's Z. We read the slab with a ±1 Z margin so every
// neighbor-tuple read is guaranteed to land inside the slab; we only write back the interior
// (excluding the margin) so the margin slices are never mutated.
// -----------------------------------------------------------------------------
template <typename T>
class ChunkedTransferWorker
{
public:
  ChunkedTransferWorker(IDataArray& array, const std::vector<usize>& changedVoxels, const std::vector<int64>& neighborVoxelIdxs, std::array<int64, 3> dims, const std::atomic_bool& shouldCancel)
  : m_Store(array.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_ChangedVoxels(changedVoxels)
  , m_NeighborVoxelIdxs(neighborVoxelIdxs)
  , m_Dims(dims)
  , m_ShouldCancel(shouldCancel)
  {
  }
  ~ChunkedTransferWorker() = default;

  ChunkedTransferWorker(const ChunkedTransferWorker&) = default;
  ChunkedTransferWorker(ChunkedTransferWorker&&) noexcept = default;
  ChunkedTransferWorker& operator=(const ChunkedTransferWorker&) = delete;
  ChunkedTransferWorker& operator=(ChunkedTransferWorker&&) noexcept = delete;

  void operator()() const
  {
    if(m_ChangedVoxels.empty())
    {
      return;
    }
    const usize numComps = m_Store.getNumberOfComponents();
    const usize sliceSize = static_cast<usize>(m_Dims[0]) * static_cast<usize>(m_Dims[1]);
    const usize tupleBytes = sizeof(T) * numComps;
    const usize sliceBytes = sliceSize * tupleBytes;

    // Pick the largest Z-batch that fits inside the per-slab memory budget. The read slab is
    // (zBatch + 2) slices wide (margin for neighbors), so we subtract 2 when sizing from the
    // budget. Floor at 1 slice so we always make forward progress even on very large tuples.
    const int64 budgetSlices = std::max<int64>(1, static_cast<int64>(k_TransferSlabBudgetBytes / std::max<usize>(sliceBytes, 1)) - 2);
    const int64 zBatch = std::min<int64>(budgetSlices, m_Dims[2]);

    // Read slab: (zBatch + 2) slices to cover the ±1 Z-neighbor margin.
    const usize slabTupleCapacity = static_cast<usize>(zBatch + 2) * sliceSize;
    auto slab = std::make_unique<T[]>(slabTupleCapacity * numComps);

    // changedVoxels is produced by the Z-major scan so it is already sorted by tuple index —
    // we can walk it with a single monotone cursor rather than searching per batch.
    usize cursor = 0;
    for(int64 zStart = 0; zStart < m_Dims[2]; zStart += zBatch)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const int64 zEnd = std::min<int64>(zStart + zBatch, m_Dims[2]);
      const usize batchUpperLimit = static_cast<usize>(zEnd) * sliceSize;

      const usize batchLo = cursor;
      while(cursor < m_ChangedVoxels.size() && m_ChangedVoxels[cursor] < batchUpperLimit)
      {
        cursor++;
      }
      const usize batchHi = cursor;
      if(batchLo == batchHi)
      {
        continue; // no bad voxels fall in this Z-batch
      }

      // Extend read range by ±1 slice (clamped to volume) to cover the 6-face neighbor window.
      const int64 readZStart = std::max<int64>(0, zStart - 1);
      const int64 readZEnd = std::min<int64>(m_Dims[2], zEnd + 1);
      const usize readTuples = static_cast<usize>(readZEnd - readZStart) * sliceSize;
      const usize readElements = readTuples * numComps;
      m_Store.copyIntoBuffer(static_cast<usize>(readZStart) * sliceSize * numComps, nonstd::span<T>(slab.get(), readElements));

      // Apply (voxel <- neighbor) tuple copies in-memory on the slab. The scan invariant
      // guarantees every neighborGlobal lands inside [readZStart, readZEnd) so the local
      // offsets below are always in range of the allocated slab.
      const int64 slabBaseTuple = readZStart * static_cast<int64>(sliceSize);
      for(usize k = batchLo; k < batchHi; k++)
      {
        const int64 voxelGlobal = static_cast<int64>(m_ChangedVoxels[k]);
        const int64 neighborGlobal = m_NeighborVoxelIdxs[k];
        if(neighborGlobal < 0)
        {
          continue;
        }
        const usize voxelLocal = static_cast<usize>(voxelGlobal - slabBaseTuple);
        const usize neighborLocal = static_cast<usize>(neighborGlobal - slabBaseTuple);
        T* const voxelDst = slab.get() + voxelLocal * numComps;
        const T* const neighborSrc = slab.get() + neighborLocal * numComps;
        for(usize c = 0; c < numComps; c++)
        {
          voxelDst[c] = neighborSrc[c];
        }
      }

      // Write back only the [zStart, zEnd) interior — the ±1 margin is read-only scratch.
      const usize writeStartTupleInSlab = static_cast<usize>(zStart - readZStart) * sliceSize;
      const usize writeTupleCount = static_cast<usize>(zEnd - zStart) * sliceSize;
      m_Store.copyFromBuffer(static_cast<usize>(zStart) * sliceSize * numComps, nonstd::span<const T>(slab.get() + writeStartTupleInSlab * numComps, writeTupleCount * numComps));
    }
  }

private:
  AbstractDataStore<T>& m_Store;
  const std::vector<usize>& m_ChangedVoxels;
  const std::vector<int64>& m_NeighborVoxelIdxs;
  std::array<int64, 3> m_Dims;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
RequireMinimumSizeFeatures::RequireMinimumSizeFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       RequireMinimumSizeFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RequireMinimumSizeFeatures::~RequireMinimumSizeFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RequireMinimumSizeFeatures::operator()()
{

  // Input Cell Level Data
  auto& featureIdsStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();

  // Input Feature Level Data
  auto& featureNumCellsStoreRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureNumCellsPath).getDataStoreRef();

  // Optionally allow applying to a single phase
  auto* featurePhases = m_InputValues->ApplySinglePhase ? m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesPath)->getDataStore() : nullptr;
  if(m_InputValues->ApplySinglePhase && featurePhases != nullptr)
  {
    usize numFeatures = featurePhases->getNumberOfTuples();
    bool unavailablePhase = true;
    for(usize i = 0; i < numFeatures; i++)
    {
      if(featurePhases->getValue(i) == m_InputValues->PhaseNumber)
      {
        unavailablePhase = false;
        break;
      }
    }

    if(unavailablePhase)
    {
      std::string ss = fmt::format("The phase number {} is not available in the supplied Feature phases array with path {}", m_InputValues->PhaseNumber, m_InputValues->FeaturePhasesPath.toString());
      return MakeErrorResult(-5555, ss);
    }
  }

  Error errorReturn = {0, ""};
  std::vector<bool> activeObjects =
      removeSmallFeatures(featureIdsStoreRef, featureNumCellsStoreRef, featurePhases, m_InputValues->PhaseNumber, m_InputValues->ApplySinglePhase, m_InputValues->MinAllowedFeaturesSize, errorReturn);

  if(errorReturn.code < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{errorReturn})};
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  assignBadVoxels(imageGeom.getDimensions(), featureNumCellsStoreRef);

  if(m_ShouldCancel)
  {
    return {};
  }
  DataPath cellFeatureGroupPath = m_InputValues->FeatureNumCellsPath.getParent();
  usize currentFeatureCount = featureNumCellsStoreRef.getNumberOfTuples();

  int32 count = 0;
  for(const auto& value : activeObjects)
  {
    if(value)
    {
      count++;
    }
  }
  std::string message = fmt::format("Feature Count Changed: Previous: {} New: {}", currentFeatureCount, count);
  m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});

  nx::core::RemoveInactiveObjects(m_DataStructure, cellFeatureGroupPath, activeObjects, featureIdsStoreRef, currentFeatureCount, m_MessageHandler, m_ShouldCancel);

  return {};
}

/**
 * @brief Iteratively fills voxels belonging to removed features (featureId < 0)
 * by majority-voting among their 6 face-neighbors.
 *
 * OOC optimization: The voting scan uses a rolling 3-slice buffer for FeatureIds.
 * For each Z-slice, the current slice and its Z-neighbors (prev, next) are in
 * memory, so all 6 face-neighbor reads come from local buffers rather than
 * per-element OOC DataStore access. The buffer slides forward one Z-slice per
 * iteration. Only changed voxels are tracked and have their data arrays updated,
 * rather than iterating over all voxels for each data array.
 */
void RequireMinimumSizeFeatures::assignBadVoxels(SizeVec3 dimensions, const Int32AbstractDataStore& featureNumCellsStoreRef)
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage(fmt::format("Assigning voxels...."));

  Int32AbstractDataStore& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();

  const std::array<int64, 3> dims = {
      static_cast<int64>(dimensions[0]),
      static_cast<int64>(dimensions[1]),
      static_cast<int64>(dimensions[2]),
  };

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // voteCounter is indexed by FEATURE ID, not by cell — its size tracks feature count (~thousands)
  // and is independent of volume size. Safe for OOC.
  std::vector<uint8> voteCounter(featureNumCellsStoreRef.getNumberOfTuples(), 0);

  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Rolling 3-slice buffer: holds the previous, current, and next Z-slices so all 6 face-neighbor
  // reads come from local memory. Allocated once and reused across convergence iterations rather
  // than reallocated per iteration. Size is O(X*Y), bounded by slice area (not O(n_cells)).
  std::vector<int32> slabBuf(3 * sliceSize, 0);

  // Sparse record of (bad-voxel index, chosen-neighbor index) pairs produced by each scan pass.
  // Allocated empty; grows only to the number of bad voxels that find a valid neighbor, and is
  // cleared between iterations. Replaces the old O(n_cells) dense neighborsVoxelIndex vector
  // which allocated 8 bytes per voxel across the entire volume.
  std::vector<usize> changedVoxels;
  std::vector<int64> neighborVoxelIdxs;

  usize counter = 1;
  while(counter != 0)
  {
    counter = 0;

    std::fill(slabBuf.begin(), slabBuf.end(), 0);
    int32* prevSlice = slabBuf.data();
    int32* curSlice = slabBuf.data() + sliceSize;
    int32* nextSlice = slabBuf.data() + 2 * sliceSize;

    featureIds.copyIntoBuffer(0, nonstd::span<int32>(curSlice, sliceSize));
    if(dims[2] > 1)
    {
      featureIds.copyIntoBuffer(sliceSize, nonstd::span<int32>(nextSlice, sliceSize));
    }

    changedVoxels.clear();
    neighborVoxelIdxs.clear();

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const int64 kstride = dims[0] * dims[1] * zIdx;
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        const int64 jstride = dims[0] * yIdx;
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 globalIdx = kstride + jstride + xIdx;
          const int64 localIdx = yIdx * dims[0] + xIdx;
          const int32 currentFeatureId = curSlice[localIdx];
          if(currentFeatureId < 0)
          {
            counter++;
            uint8 maxVoteCount = 0;
            int64 chosenNeighborIdx = -1;
            const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              const int64 neighborOffset = neighborVoxelIndexOffsets[faceIndex];
              int32 neighborFeatureId = 0;
              if(neighborOffset == -dims[0] * dims[1])
              {
                neighborFeatureId = prevSlice[localIdx];
              }
              else if(neighborOffset == dims[0] * dims[1])
              {
                neighborFeatureId = nextSlice[localIdx];
              }
              else
              {
                neighborFeatureId = curSlice[localIdx + neighborOffset];
              }

              if(neighborFeatureId >= 0)
              {
                voteCounter[neighborFeatureId]++;
                const uint8 currentVoteCount = voteCounter[neighborFeatureId];
                if(currentVoteCount > maxVoteCount)
                {
                  maxVoteCount = currentVoteCount;
                  chosenNeighborIdx = globalIdx + neighborOffset;
                }
              }
            }

            if(chosenNeighborIdx >= 0)
            {
              changedVoxels.push_back(static_cast<usize>(globalIdx));
              neighborVoxelIdxs.push_back(chosenNeighborIdx);
            }

            std::fill(voteCounter.begin(), voteCounter.end(), 0);
          }
        }
      }

      // Slide the slab window forward: what was nextSlice becomes curSlice, etc.
      std::swap(prevSlice, curSlice);
      std::swap(curSlice, nextSlice);
      if(zIdx + 2 < dims[2])
      {
        featureIds.copyIntoBuffer(static_cast<usize>(zIdx + 2) * sliceSize, nonstd::span<int32>(nextSlice, sliceSize));
      }
    }

    // Skip transfer entirely if no voxels were assigned this iteration.
    if(changedVoxels.empty())
    {
      break;
    }

    messageHelper.sendMessage(fmt::format("Remaining voxels: {} - Updating {} changed voxels... ", counter, changedVoxels.size()));

    // Transfer phase: dispatch one ChunkedTransferWorker per cell-level array across threads.
    // Each worker does Z-batched bulk I/O (copyIntoBuffer + in-memory edits + copyFromBuffer)
    // — orders of magnitude fewer HDF5 chunk ops than the old per-voxel copyTuple loop.
    const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsPath, {});

    ParallelTaskAlgorithm taskRunner;
    for(const auto& voxelArray : voxelArrays)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      ExecuteParallelFunction<ChunkedTransferWorker>(voxelArray->getDataType(), taskRunner, *voxelArray, changedVoxels, neighborVoxelIdxs, dims, m_ShouldCancel);
    }
    taskRunner.wait();
  }
}

// -----------------------------------------------------------------------------
std::vector<bool> RequireMinimumSizeFeatures::removeSmallFeatures(Int32AbstractDataStore& featureIdsStoreRef, const Int32AbstractDataStore& featureNumCellsStoreRef,
                                                                  const Int32AbstractDataStore* featurePhases, int32 phaseNumber, bool applyToSinglePhase, int64 minAllowedFeatureSize,
                                                                  Error& errorReturn)
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage(fmt::format("Removing small features...."));

  usize totalPoints = featureIdsStoreRef.getNumberOfTuples();

  bool good = false;

  usize totalFeatures = featureNumCellsStoreRef.getNumberOfTuples();

  std::vector<bool> activeObjects(totalFeatures, true);

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(!applyToSinglePhase)
    {
      if(featureNumCellsStoreRef.getValue(i) >= minAllowedFeatureSize)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
    else
    {
      if(featureNumCellsStoreRef.getValue(i) >= minAllowedFeatureSize || featurePhases->getValue(i) != phaseNumber)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
  }
  if(!good)
  {
    errorReturn = Error{-1, "The minimum size is larger than the largest Feature.  All Features would be removed"};
    return activeObjects;
  }

  // Mark removed features' voxels with featureId = -1 using chunked bulk I/O.
  // The original per-element setValue(-1) caused a chunk operation per voxel.
  // Reading/writing in 64K chunks reduces chunk operations by ~64000x.
  // Only modified chunks are written back (tracked via the `modified` flag).
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, totalPoints - offset);
    featureIdsStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));

    bool modified = false;
    for(usize i = 0; i < count; i++)
    {
      if(!activeObjects[featureIdBuf[i]])
      {
        featureIdBuf[i] = -1;
        modified = true;
      }
    }
    if(modified)
    {
      featureIdsStoreRef.copyFromBuffer(offset, nonstd::span<const int32>(featureIdBuf.get(), count));
    }
  }
  return activeObjects;
}
