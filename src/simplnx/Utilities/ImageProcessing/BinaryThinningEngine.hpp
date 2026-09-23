#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/task_arena.h>
#endif

#include <thread>

namespace nx::core::ImageProcessing
{
namespace detail
{
inline constexpr usize k_BinaryThinning2DTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_BinaryThinning2DMetadataHeadroomBytes = 64ULL * 1024ULL;

/**
 * @brief Returns the smallest target that can hold one external tile column and one typed conversion value.
 * @tparam T Input and output value type.
 * @return Minimum external 2D working-memory target in bytes.
 */
template <class T>
constexpr usize BinaryThinningMinimumExternal2DTargetBytes()
{
  constexpr usize k_MinimumTileBytes = 10;
  return k_BinaryThinning2DMetadataHeadroomBytes + std::max(k_MinimumTileBytes, sizeof(T) + sizeof(uint8));
}

struct BinaryThinningFixed2DPlan
{
  bool useFixedCapacity = false;
  usize workCapacityValues = 0;
  usize markerCapacityWords = 0;
  usize transferValues = 0;
  usize minimumFixedBytes = 0;
  usize residentBytes = 0;
};

struct BinaryThinningExternal2DPlan
{
  bool useTiles = false;
  usize coreRows = 0;
  usize coreColumns = 0;
  usize residentBytes = 0;
};

/**
 * @brief Bounded memory plan for parallel thinning of independent Z slices.
 */
struct BinaryThinningSliceBatchPlan
{
  usize sliceValues = 0;
  usize markerWordsPerSlice = 0;
  usize bytesPerWorker = 0;
  usize workerCount = 0;
  usize residentBytes = 0;
};

inline bool BinaryThinningCheckedAdd(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline bool BinaryThinningCheckedMultiply(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

/**
 * @brief Creates a checked plan for parallel Z-slice thinning within @p targetBytes.
 *
 * Each worker owns one typed slice that is reused for output, one uint8 work slice, and one packed deletion bitset.
 * @tparam T Input and output value type.
 * @param dims Image dimensions in X, Y, Z order.
 * @param targetBytes Available working-memory bytes.
 * @param maximumWorkers Maximum compute workers available to this call.
 * @return A valid bounded plan or a contextual size or capacity error.
 */
template <class T>
Result<BinaryThinningSliceBatchPlan> CreateBinaryThinningSliceBatchPlan(const SizeVec3& dims, usize targetBytes, usize maximumWorkers)
{
  BinaryThinningSliceBatchPlan plan;
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0 || targetBytes == 0 || maximumWorkers == 0)
  {
    return MakeErrorResult<BinaryThinningSliceBatchPlan>(
        -8716, fmt::format("Binary thinning requires nonzero 3D dimensions, target bytes, and workers. Dimensions: {} x {} x {}; target: {}; maximum workers: {}.", dims[0], dims[1], dims[2],
                           targetBytes, maximumWorkers));
  }

  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dims[0] > k_Int64Max || dims[1] > k_Int64Max)
  {
    return MakeErrorResult<BinaryThinningSliceBatchPlan>(-8716,
                                                         fmt::format("Binary thinning slice dimensions must fit signed 64-bit indexes. Dimensions: {} x {} x {}; target: {}; maximum workers: {}.",
                                                                     dims[0], dims[1], dims[2], targetBytes, maximumWorkers));
  }

  usize volumeValues = 0;
  if(!BinaryThinningCheckedMultiply(dims[0], dims[1], plan.sliceValues) || !BinaryThinningCheckedMultiply(plan.sliceValues, dims[2], volumeValues) || volumeValues == 0)
  {
    return MakeErrorResult<BinaryThinningSliceBatchPlan>(
        -8716, fmt::format("Binary thinning dimensions {} x {} x {} overflow the addressable value count. Target: {}; maximum workers: {}.", dims[0], dims[1], dims[2], targetBytes, maximumWorkers));
  }
  plan.markerWordsPerSlice = plan.sliceValues / 64 + (plan.sliceValues % 64 != 0 ? 1 : 0);

  usize typedBytes = 0;
  usize markerBytes = 0;
  if(!BinaryThinningCheckedMultiply(plan.sliceValues, sizeof(T), typedBytes) || !BinaryThinningCheckedMultiply(plan.markerWordsPerSlice, sizeof(uint64), markerBytes) ||
     !BinaryThinningCheckedAdd(typedBytes, plan.sliceValues, plan.bytesPerWorker) || !BinaryThinningCheckedAdd(plan.bytesPerWorker, markerBytes, plan.bytesPerWorker))
  {
    return MakeErrorResult<BinaryThinningSliceBatchPlan>(-8716, fmt::format("Binary thinning cannot size one {}-byte-value worker for dimensions {} x {} x {}. Target: {}; maximum workers: {}.",
                                                                            sizeof(T), dims[0], dims[1], dims[2], targetBytes, maximumWorkers));
  }

  const usize availableWorkers = std::min(dims[2], maximumWorkers);
  plan.workerCount = std::min(availableWorkers, targetBytes / plan.bytesPerWorker);
  if(plan.workerCount == 0 || !BinaryThinningCheckedMultiply(plan.workerCount, plan.bytesPerWorker, plan.residentBytes))
  {
    return MakeErrorResult<BinaryThinningSliceBatchPlan>(-8716,
                                                         fmt::format("Binary thinning cannot fit one {}-byte Z-slice worker for dimensions {} x {} x {} within a {}-byte target. Maximum workers: {}.",
                                                                     plan.bytesPerWorker, dims[0], dims[1], dims[2], targetBytes, maximumWorkers));
  }
  return {plan};
}

template <class T>
Result<BinaryThinningFixed2DPlan> CreateBinaryThinningFixed2DPlan(usize volumeValues, usize targetBytes = k_BinaryThinning2DTargetBytes)
{
  BinaryThinningFixed2DPlan plan;
  if(volumeValues == 0 || targetBytes < BinaryThinningMinimumExternal2DTargetBytes<T>())
  {
    return MakeErrorResult<BinaryThinningFixed2DPlan>(-8711,
                                                      fmt::format("Binary thinning cannot plan true-2-D storage for {} values of {} bytes each within a {}-byte target. Minimum external target: {}.",
                                                                  volumeValues, sizeof(T), targetBytes, BinaryThinningMinimumExternal2DTargetBytes<T>()));
  }

  const usize markerCapacityWords = volumeValues / 64 + (volumeValues % 64 != 0 ? 1 : 0);
  usize markerBytes = 0;
  usize fixedStateBytes = 0;
  if(!BinaryThinningCheckedMultiply(markerCapacityWords, sizeof(uint64), markerBytes) || !BinaryThinningCheckedAdd(k_BinaryThinning2DMetadataHeadroomBytes, volumeValues, fixedStateBytes) ||
     !BinaryThinningCheckedAdd(fixedStateBytes, markerBytes, fixedStateBytes) || !BinaryThinningCheckedAdd(fixedStateBytes, sizeof(T), plan.minimumFixedBytes))
  {
    return MakeErrorResult<BinaryThinningFixed2DPlan>(
        -8711, fmt::format("Binary thinning cannot plan fixed true-2-D storage for {} values of {} bytes each within a {}-byte target.", volumeValues, sizeof(T), targetBytes));
  }

  if(plan.minimumFixedBytes > targetBytes)
  {
    return {plan};
  }

  const usize transferValues = std::min(volumeValues, (targetBytes - fixedStateBytes) / sizeof(T));
  usize transferBytes = 0;
  if(transferValues == 0 || !BinaryThinningCheckedMultiply(transferValues, sizeof(T), transferBytes) || !BinaryThinningCheckedAdd(fixedStateBytes, transferBytes, plan.residentBytes))
  {
    return MakeErrorResult<BinaryThinningFixed2DPlan>(
        -8711, fmt::format("Binary thinning fixed true-2-D storage leaves no typed transfer capacity for {} values of {} bytes each within a {}-byte target.", volumeValues, sizeof(T), targetBytes));
  }

  plan.useFixedCapacity = true;
  plan.workCapacityValues = volumeValues;
  plan.markerCapacityWords = markerCapacityWords;
  plan.transferValues = transferValues;
  return {plan};
}

inline Result<BinaryThinningExternal2DPlan> CreateBinaryThinningExternal2DPlan(usize dimX, usize dimY, usize targetBytes = k_BinaryThinning2DTargetBytes)
{
  usize volumeValues = 0;
  if(dimX == 0 || dimY == 0 || targetBytes <= k_BinaryThinning2DMetadataHeadroomBytes || !BinaryThinningCheckedMultiply(dimX, dimY, volumeValues))
  {
    return MakeErrorResult<BinaryThinningExternal2DPlan>(
        -8712, fmt::format("Binary thinning requires nonzero true-2-D dimensions whose value count fits usize. Dimensions: {} x {}; target: {} bytes.", dimX, dimY, targetBytes));
  }

  BinaryThinningExternal2DPlan plan;
  const usize availableBytes = targetBytes - k_BinaryThinning2DMetadataHeadroomBytes;
  usize minimumFullWidthBytes = 0;
  if(BinaryThinningCheckedMultiply(dimX, usize{4}, minimumFullWidthBytes) && minimumFullWidthBytes <= availableBytes)
  {
    const usize rowUnits = availableBytes / dimX;
    plan.coreRows = std::min(dimY, (rowUnits - 2) / 2);
    plan.coreColumns = dimX;
    usize residentRows = 0;
    usize residentValues = 0;
    if(plan.coreRows == 0 || !BinaryThinningCheckedMultiply(plan.coreRows, usize{2}, residentRows) || !BinaryThinningCheckedAdd(residentRows, usize{2}, residentRows) ||
       !BinaryThinningCheckedMultiply(residentRows, dimX, residentValues) || !BinaryThinningCheckedAdd(k_BinaryThinning2DMetadataHeadroomBytes, residentValues, plan.residentBytes))
    {
      return MakeErrorResult<BinaryThinningExternal2DPlan>(
          -8712, fmt::format("Binary thinning cannot fit one full-width external row block for dimensions {} x {} within a {}-byte target.", dimX, dimY, targetBytes));
    }
    return {plan};
  }

  constexpr usize k_TileHaloBytes = 6;
  constexpr usize k_BytesPerCoreColumn = 4;
  if(availableBytes < k_TileHaloBytes + k_BytesPerCoreColumn)
  {
    return MakeErrorResult<BinaryThinningExternal2DPlan>(
        -8712, fmt::format("Binary thinning cannot fit one external X-tile column with halos for dimensions {} x {} within a {}-byte target.", dimX, dimY, targetBytes));
  }

  plan.useTiles = true;
  plan.coreRows = 1;
  plan.coreColumns = std::min(dimX, (availableBytes - k_TileHaloBytes) / k_BytesPerCoreColumn);
  usize tileBytes = 0;
  if(plan.coreColumns == 0 || !BinaryThinningCheckedMultiply(plan.coreColumns, k_BytesPerCoreColumn, tileBytes) || !BinaryThinningCheckedAdd(tileBytes, k_TileHaloBytes, tileBytes) ||
     !BinaryThinningCheckedAdd(k_BinaryThinning2DMetadataHeadroomBytes, tileBytes, plan.residentBytes))
  {
    return MakeErrorResult<BinaryThinningExternal2DPlan>(-8712,
                                                         fmt::format("Binary thinning external X-tile planning overflowed for dimensions {} x {} and a {}-byte target.", dimX, dimY, targetBytes));
  }
  return {plan};
}

class BinaryThinningWorkStore
{
public:
  explicit BinaryThinningWorkStore(AbstractDataStore<uint8>& store)
  : m_DataStore(&store)
  {
  }

  explicit BinaryThinningWorkStore(SweepTemporaryStore<uint8>& store)
  : m_FixedRecordStore(&store)
  {
  }

  usize getSize() const
  {
    return m_DataStore != nullptr ? m_DataStore->getSize() : m_FixedRecordStore->getSize();
  }

  std::optional<ShapeType> getChunkShape() const
  {
    return m_DataStore != nullptr ? m_DataStore->getChunkShape() : m_FixedRecordStore->getChunkShape();
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<uint8> values) const
  {
    return m_DataStore != nullptr ? m_DataStore->copyIntoBuffer(offset, values) : m_FixedRecordStore->copyIntoBuffer(offset, values);
  }

  Result<> copyFromBuffer(usize offset, nonstd::span<const uint8> values)
  {
    return m_DataStore != nullptr ? m_DataStore->copyFromBuffer(offset, values) : m_FixedRecordStore->copyFromBuffer(offset, values);
  }

private:
  AbstractDataStore<uint8>* m_DataStore = nullptr;
  SweepTemporaryStore<uint8>* m_FixedRecordStore = nullptr;
};
} // namespace detail

namespace detail
{
// ITK-free port of BinaryThinningImageFilter: per-z-slice 2D sequential thinning (Gonzalez-Woods), transcribed from
// itkBinaryThinningImageFilter.hxx. PrepareData: foreground (input != 0) -> 1, else 0. ComputeThinImage: repeat
// {for step 1..4: collect all center pixels (center==1) satisfying testA (1<nOn<7) & testB (transitions==1) & the
// step's testC/D; then set them to 0} until a full 4-step pass deletes nothing. ITK's 8 neighbor offsets are 2D
// (z-offset 0), so a 3D image thins each z-slice independently -> we thin one slice in a RAM buffer to convergence and
// write it (byte-identical to ITK's global loop; slices are decoupled). ZeroFluxNeumann (edge-clamp) 8-neighbor reads.
// Output values are 0/1 in T. `messageHandler` currently unused (progress deferred); kept for signature parity.
template <class T>
Result<> ApplyBinaryThinningResident(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, const std::atomic_bool& shouldCancel,
                                     const IFilter::MessageHandler& messageHandler)
{
  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  const usize slice = static_cast<usize>(nX) * static_cast<usize>(nY);

  auto clampi = [](int64 v, int64 hi) { return v < 0 ? int64{0} : (v > hi ? hi : v); };

  std::vector<T> inBuf(slice);
  std::vector<uint8> work(slice); // 0/1 working slice
  std::vector<T> outBuf(slice);
  std::vector<usize> toDelete;

  for(int64 z = 0; z < nZ; ++z)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> r = inStore.copyIntoBuffer(static_cast<usize>(z) * slice, nonstd::span<T>(inBuf.data(), slice)); r.invalid())
    {
      return r;
    }
    for(usize i = 0; i < slice; ++i) // PrepareData: foreground -> 1
    {
      work[i] = (inBuf[i] != T{}) ? uint8{1} : uint8{0};
    }

    // ZeroFluxNeumann-clamped neighbor read (default NeighborhoodIterator boundary).
    auto at = [&](int64 xx, int64 yy) -> int { return static_cast<int>(work[static_cast<usize>(clampi(yy, nY - 1) * nX + clampi(xx, nX - 1))]); };

    bool noChange = false;
    while(!noChange)
    {
      noChange = true;
      for(int step = 1; step <= 4; ++step)
      {
        if(shouldCancel)
        {
          return {};
        }
        toDelete.clear();
        for(int64 y = 0; y < nY; ++y)
        {
          for(int64 x = 0; x < nX; ++x)
          {
            if(work[static_cast<usize>(y * nX + x)] == 0)
            {
              continue; // center off
            }
            // Gonzalez-Woods 8-neighborhood (clockwise from north): p2=(0,-1) .. p9=(-1,-1).
            const int p2 = at(x, y - 1);
            const int p3 = at(x + 1, y - 1);
            const int p4 = at(x + 1, y);
            const int p5 = at(x + 1, y + 1);
            const int p6 = at(x, y + 1);
            const int p7 = at(x - 1, y + 1);
            const int p8 = at(x - 1, y);
            const int p9 = at(x - 1, y - 1);

            const int nOn = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
            const bool testA = (nOn > 1 && nOn < 7);

            const int transitions = (std::abs(p3 - p2) + std::abs(p4 - p3) + std::abs(p5 - p4) + std::abs(p6 - p5) + std::abs(p7 - p6) + std::abs(p8 - p7) + std::abs(p9 - p8) + std::abs(p2 - p9)) / 2;
            const bool testB = (transitions == 1);

            bool testCD = false;
            if(step == 1)
            {
              testCD = (p4 == 0 || p6 == 0);
            }
            else if(step == 2)
            {
              testCD = (p2 == 0 && p8 == 0);
            }
            else if(step == 3)
            {
              testCD = (p2 == 0 || p8 == 0);
            }
            else // step == 4
            {
              testCD = (p4 == 0 && p6 == 0);
            }

            if(testA && testB && testCD)
            {
              toDelete.push_back(static_cast<usize>(y * nX + x));
              noChange = false;
            }
          }
        }
        for(usize idx : toDelete) // apply deletions after the full step scan
        {
          work[idx] = 0;
        }
      }
    }

    for(usize i = 0; i < slice; ++i)
    {
      outBuf[i] = static_cast<T>(work[i]);
    }
    if(Result<> r = outStore.copyFromBuffer(static_cast<usize>(z) * slice, nonstd::span<const T>(outBuf.data(), slice)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

inline bool BinaryThinningShouldDelete(int step, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9)
{
  const int nOn = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
  const bool testA = nOn > 1 && nOn < 7;
  const int transitions = (std::abs(p3 - p2) + std::abs(p4 - p3) + std::abs(p5 - p4) + std::abs(p6 - p5) + std::abs(p7 - p6) + std::abs(p8 - p7) + std::abs(p9 - p8) + std::abs(p2 - p9)) / 2;
  const bool testB = transitions == 1;

  bool testCD = false;
  if(step == 1)
  {
    testCD = p4 == 0 || p6 == 0;
  }
  else if(step == 2)
  {
    testCD = p2 == 0 && p8 == 0;
  }
  else if(step == 3)
  {
    testCD = p2 == 0 || p8 == 0;
  }
  else
  {
    testCD = p4 == 0 && p6 == 0;
  }
  return testA && testB && testCD;
}

/**
 * @brief Thins one resident slice with exact ITK substep order and packed deletion markers.
 * @tparam T Input and output value type.
 * @param typedValues Typed input values. The function replaces them with typed binary output values.
 * @param workValues Private uint8 work values for the slice.
 * @param deletionMarkers Private packed deletion markers for the slice.
 * @param dimX Slice width.
 * @param dimY Slice height.
 * @param shouldCancel Shared cancellation flag.
 */
template <class T>
void ThinBinarySliceInPlace(T* typedValues, uint8* workValues, uint64* deletionMarkers, usize dimX, usize dimY, const std::atomic_bool& shouldCancel)
{
  const usize sliceValues = dimX * dimY;
  const usize markerWords = sliceValues / 64 + (sliceValues % 64 != 0 ? 1 : 0);
  for(usize index = 0; index < sliceValues; ++index)
  {
    workValues[index] = typedValues[index] != T{} ? uint8{1} : uint8{0};
  }

  const int64 signedDimX = static_cast<int64>(dimX);
  const int64 signedDimY = static_cast<int64>(dimY);
  const auto clamp = [](int64 value, int64 upper) { return value < 0 ? int64{0} : (value > upper ? upper : value); };
  const auto at = [&](int64 x, int64 y) {
    const int64 flatIndex = clamp(y, signedDimY - 1) * signedDimX + clamp(x, signedDimX - 1);
    return static_cast<int>(workValues[static_cast<usize>(flatIndex)]);
  };

  bool changed = true;
  while(changed)
  {
    changed = false;
    for(int step = 1; step <= 4; ++step)
    {
      if(shouldCancel)
      {
        return;
      }
      std::fill_n(deletionMarkers, markerWords, uint64{0});
      bool stepChanged = false;
      for(int64 y = 0; y < signedDimY; ++y)
      {
        if(shouldCancel)
        {
          return;
        }
        for(int64 x = 0; x < signedDimX; ++x)
        {
          const usize flatIndex = static_cast<usize>(y * signedDimX + x);
          if(workValues[flatIndex] == 0)
          {
            continue;
          }
          const int p2 = at(x, y - 1);
          const int p3 = at(x + 1, y - 1);
          const int p4 = at(x + 1, y);
          const int p5 = at(x + 1, y + 1);
          const int p6 = at(x, y + 1);
          const int p7 = at(x - 1, y + 1);
          const int p8 = at(x - 1, y);
          const int p9 = at(x - 1, y - 1);
          if(BinaryThinningShouldDelete(step, p2, p3, p4, p5, p6, p7, p8, p9))
          {
            deletionMarkers[flatIndex / 64] |= uint64{1} << (flatIndex % 64);
            stepChanged = true;
          }
        }
      }

      for(usize wordIndex = 0; wordIndex < markerWords; ++wordIndex)
      {
        uint64 markedBits = deletionMarkers[wordIndex];
        while(markedBits != 0)
        {
          const usize bitOffset = static_cast<usize>(std::countr_zero(markedBits));
          const usize flatIndex = wordIndex * 64 + bitOffset;
          if(flatIndex < sliceValues)
          {
            workValues[flatIndex] = 0;
          }
          markedBits &= markedBits - 1;
        }
      }
      changed = changed || stepChanged;
    }
  }

  if(shouldCancel)
  {
    return;
  }
  for(usize index = 0; index < sliceValues; ++index)
  {
    typedValues[index] = static_cast<T>(workValues[index]);
  }
}

/**
 * @brief Reads, thins, and writes independent Z slices in bounded parallel batches.
 *
 * Store transfers stay on the caller thread. Each batch performs one contiguous input read and one contiguous output
 * write. Compute workers use disjoint resident buffers.
 * @tparam T Input and output value type.
 * @param inStore Input scalar store.
 * @param outStore Output scalar store.
 * @param dims Image dimensions in X, Y, Z order.
 * @param shouldCancel Shared cancellation flag.
 * @param plan Checked batch plan.
 * @return A valid result or a contextual plan, allocation, or store-transfer error.
 */
template <class T>
Result<> ApplyBinaryThinningSliceBatches(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, const std::atomic_bool& shouldCancel,
                                         const BinaryThinningSliceBatchPlan& plan)
{
  usize volumeValues = 0;
  auto expectedPlanResult = CreateBinaryThinningSliceBatchPlan<T>(dims, plan.residentBytes, plan.workerCount);
  const bool planMatches = expectedPlanResult.valid() && expectedPlanResult.value().sliceValues == plan.sliceValues && expectedPlanResult.value().markerWordsPerSlice == plan.markerWordsPerSlice &&
                           expectedPlanResult.value().bytesPerWorker == plan.bytesPerWorker && expectedPlanResult.value().workerCount == plan.workerCount &&
                           expectedPlanResult.value().residentBytes == plan.residentBytes;
  if(!planMatches || !BinaryThinningCheckedMultiply(plan.sliceValues, dims[2], volumeValues) || inStore.getSize() != volumeValues || outStore.getSize() != volumeValues)
  {
    return MakeErrorResult(
        -8717, fmt::format("Binary thinning received an invalid 3D slice-batch plan for dimensions {} x {} x {}. Input values: {}; output values: {}; slice values: {}; workers: {}; "
                           "marker words: {}; bytes per worker: {}; resident bytes: {}.",
                           dims[0], dims[1], dims[2], inStore.getSize(), outStore.getSize(), plan.sliceValues, plan.workerCount, plan.markerWordsPerSlice, plan.bytesPerWorker, plan.residentBytes));
  }

  usize batchCapacityValues = 0;
  usize batchMarkerWords = 0;
  if(!BinaryThinningCheckedMultiply(plan.sliceValues, plan.workerCount, batchCapacityValues) || !BinaryThinningCheckedMultiply(plan.markerWordsPerSlice, plan.workerCount, batchMarkerWords))
  {
    return MakeErrorResult(
        -8717, fmt::format("Binary thinning batch allocation overflows for {} slice values, {} marker words, and {} workers.", plan.sliceValues, plan.markerWordsPerSlice, plan.workerCount));
  }

  std::unique_ptr<T[]> typedValues;
  std::unique_ptr<uint8[]> workValues;
  std::unique_ptr<uint64[]> deletionMarkers;
  try
  {
    typedValues = std::make_unique_for_overwrite<T[]>(batchCapacityValues);
    workValues = std::make_unique_for_overwrite<uint8[]>(batchCapacityValues);
    deletionMarkers = std::make_unique_for_overwrite<uint64[]>(batchMarkerWords);
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-8717, fmt::format("Binary thinning could not allocate {} bytes for {} Z-slice workers. Slice values: {}; marker words per slice: {}.", plan.residentBytes, plan.workerCount,
                                              plan.sliceValues, plan.markerWordsPerSlice));
  }

#ifdef SIMPLNX_ENABLE_MULTICORE
  const usize boundedWorkers = std::min(plan.workerCount, static_cast<usize>(std::numeric_limits<int>::max()));
  tbb::task_arena arena(static_cast<int>(boundedWorkers));
#endif
  for(usize zBegin = 0; zBegin < dims[2]; zBegin += plan.workerCount)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize batchSlices = std::min(plan.workerCount, dims[2] - zBegin);
    const usize batchValues = batchSlices * plan.sliceValues;
    if(Result<> result = inStore.copyIntoBuffer(zBegin * plan.sliceValues, nonstd::span<T>(typedValues.get(), batchValues)); result.invalid())
    {
      return result;
    }
    if(shouldCancel)
    {
      return {};
    }

    const auto thinRange = [&](const Range& range) {
      for(usize slot = range.min(); slot < range.max(); ++slot)
      {
        if(shouldCancel)
        {
          return;
        }
        ThinBinarySliceInPlace(typedValues.get() + slot * plan.sliceValues, workValues.get() + slot * plan.sliceValues, deletionMarkers.get() + slot * plan.markerWordsPerSlice, dims[0], dims[1],
                               shouldCancel);
      }
    };
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, batchSlices);
#ifdef SIMPLNX_ENABLE_MULTICORE
    arena.execute([&] { parallelAlgorithm.execute(thinRange); });
#else
    parallelAlgorithm.execute(thinRange);
#endif
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = outStore.copyFromBuffer(zBegin * plan.sliceValues, nonstd::span<const T>(typedValues.get(), batchValues)); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class T>
Result<> ApplyBinaryThinningFixed2D(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, const std::atomic_bool& shouldCancel,
                                    const BinaryThinningFixed2DPlan& plan)
{
  if(shouldCancel)
  {
    return {};
  }
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }

  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  usize volumeValues = 0;
  if(dims[2] != 1 || dims[0] > k_Int64Max || dims[1] > k_Int64Max || !BinaryThinningCheckedMultiply(dims[0], dims[1], volumeValues) || volumeValues > k_Int64Max)
  {
    return MakeErrorResult(-8710, fmt::format("Binary thinning fixed true-2-D execution requires one Z slice and signed-index-safe dimensions. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(inStore.getSize() != volumeValues || outStore.getSize() != volumeValues)
  {
    return MakeErrorResult(-8710, fmt::format("Binary thinning fixed true-2-D stores do not match the {}-value image. Input values: {}; output values: {}; dimensions: {} x {} x {}.", volumeValues,
                                              inStore.getSize(), outStore.getSize(), dims[0], dims[1], dims[2]));
  }

  const usize logicalMarkerWords = volumeValues / 64 + (volumeValues % 64 != 0 ? 1 : 0);
  if(!plan.useFixedCapacity || plan.workCapacityValues < volumeValues || plan.markerCapacityWords < logicalMarkerWords || plan.transferValues == 0)
  {
    return MakeErrorResult(-8710, fmt::format("Binary thinning fixed true-2-D capacity is insufficient for {} values. Work capacity: {}; marker words: {} ({} required); transfer values: {}.",
                                              volumeValues, plan.workCapacityValues, plan.markerCapacityWords, logicalMarkerWords, plan.transferValues));
  }

  auto work = std::make_unique<uint8[]>(plan.workCapacityValues);
  auto deletionMarkers = std::make_unique<uint64[]>(plan.markerCapacityWords);
  auto transfer = std::make_unique<T[]>(plan.transferValues);

  for(usize start = 0; start < volumeValues;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(plan.transferValues, volumeValues - start);
    if(Result<> result = inStore.copyIntoBuffer(start, nonstd::span<T>(transfer.get(), count)); result.invalid())
    {
      return result;
    }
    for(usize index = 0; index < count; ++index)
    {
      work[start + index] = transfer[index] != T{} ? uint8{1} : uint8{0};
    }
    start += count;
  }

  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const auto clamp = [](int64 value, int64 upper) { return value < 0 ? int64{0} : (value > upper ? upper : value); };
  const auto at = [&](int64 x, int64 y) {
    const int64 flatIndex = clamp(y, nY - 1) * nX + clamp(x, nX - 1);
    return static_cast<int>(work[static_cast<usize>(flatIndex)]);
  };

  bool changed = true;
  while(changed)
  {
    changed = false;
    for(int step = 1; step <= 4; ++step)
    {
      if(shouldCancel)
      {
        return {};
      }
      std::fill_n(deletionMarkers.get(), logicalMarkerWords, uint64{0});
      bool stepChanged = false;
      for(int64 y = 0; y < nY; ++y)
      {
        if(shouldCancel)
        {
          return {};
        }
        for(int64 x = 0; x < nX; ++x)
        {
          const usize flatIndex = static_cast<usize>(y * nX + x);
          if(work[flatIndex] == 0)
          {
            continue;
          }
          const int p2 = at(x, y - 1);
          const int p3 = at(x + 1, y - 1);
          const int p4 = at(x + 1, y);
          const int p5 = at(x + 1, y + 1);
          const int p6 = at(x, y + 1);
          const int p7 = at(x - 1, y + 1);
          const int p8 = at(x - 1, y);
          const int p9 = at(x - 1, y - 1);
          if(BinaryThinningShouldDelete(step, p2, p3, p4, p5, p6, p7, p8, p9))
          {
            deletionMarkers[flatIndex / 64] |= uint64{1} << (flatIndex % 64);
            stepChanged = true;
          }
        }
      }

      if(shouldCancel)
      {
        return {};
      }
      for(usize wordIndex = 0; wordIndex < logicalMarkerWords; ++wordIndex)
      {
        uint64 markedBits = deletionMarkers[wordIndex];
        while(markedBits != 0)
        {
          const usize bitOffset = static_cast<usize>(std::countr_zero(markedBits));
          const usize flatIndex = wordIndex * 64 + bitOffset;
          if(flatIndex < volumeValues)
          {
            work[flatIndex] = 0;
          }
          markedBits &= markedBits - 1;
        }
      }
      changed = changed || stepChanged;
    }
  }

  for(usize start = 0; start < volumeValues;)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(plan.transferValues, volumeValues - start);
    for(usize index = 0; index < count; ++index)
    {
      transfer[index] = static_cast<T>(work[start + index]);
    }
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = outStore.copyFromBuffer(start, nonstd::span<const T>(transfer.get(), count)); result.invalid())
    {
      return result;
    }
    start += count;
  }
  return {};
}

inline Result<bool> ApplyBinaryThinningExternalSubstepBlocks(BinaryThinningWorkStore& source, BinaryThinningWorkStore& destination, usize dimX, usize dimY, int step,
                                                             const std::atomic_bool& shouldCancel, const BinaryThinningExternal2DPlan& plan)
{
  usize volumeValues = 0;
  usize maximumInputValues = 0;
  usize maximumOutputValues = 0;
  if(dimX == 0 || dimY == 0 || step < 1 || step > 4 || plan.useTiles || plan.coreRows == 0 || plan.coreColumns != dimX || !BinaryThinningCheckedMultiply(dimX, dimY, volumeValues) ||
     !BinaryThinningCheckedAdd(plan.coreRows, usize{2}, maximumInputValues) || !BinaryThinningCheckedMultiply(maximumInputValues, dimX, maximumInputValues) ||
     !BinaryThinningCheckedMultiply(plan.coreRows, dimX, maximumOutputValues))
  {
    return MakeErrorResult<bool>(-8714, fmt::format("Binary thinning external block substep {} has an invalid plan for dimensions {} x {}. Core rows: {}; core columns: {}; tiles: {}.", step, dimX,
                                                    dimY, plan.coreRows, plan.coreColumns, plan.useTiles));
  }
  if(source.getSize() != volumeValues || destination.getSize() != volumeValues)
  {
    return MakeErrorResult<bool>(-8714, fmt::format("Binary thinning external block stores do not match the {}-value image. Source values: {}; destination values: {}; dimensions: {} x {}.",
                                                    volumeValues, source.getSize(), destination.getSize(), dimX, dimY));
  }

  auto inputBlock = std::make_unique<uint8[]>(maximumInputValues);
  auto outputBlock = std::make_unique<uint8[]>(maximumOutputValues);
  bool changed = false;
  for(usize rowBegin = 0; rowBegin < dimY;)
  {
    if(shouldCancel)
    {
      return Result<bool>{false};
    }
    const usize rowCount = std::min(plan.coreRows, dimY - rowBegin);
    const usize coreEnd = rowBegin + rowCount;
    const usize readBegin = rowBegin == 0 ? 0 : rowBegin - 1;
    const usize readEnd = coreEnd < dimY ? coreEnd + 1 : dimY;
    const usize readRows = readEnd - readBegin;
    const usize readValues = readRows * dimX;
    if(Result<> result = source.copyIntoBuffer(readBegin * dimX, nonstd::span<uint8>(inputBlock.get(), readValues)); result.invalid())
    {
      return ConvertResultTo<bool>(std::move(result), false);
    }

    const auto at = [&](usize x, int64 y) {
      const int64 clampedY = std::clamp<int64>(y, 0, static_cast<int64>(dimY) - 1);
      return static_cast<int>(inputBlock[(static_cast<usize>(clampedY) - readBegin) * dimX + x]);
    };
    for(usize localRow = 0; localRow < rowCount; ++localRow)
    {
      const usize y = rowBegin + localRow;
      for(usize x = 0; x < dimX; ++x)
      {
        const usize outputIndex = localRow * dimX + x;
        const uint8 center = static_cast<uint8>(at(x, static_cast<int64>(y)));
        if(center == 0)
        {
          outputBlock[outputIndex] = 0;
          continue;
        }
        const usize leftX = x == 0 ? 0 : x - 1;
        const usize rightX = x + 1 < dimX ? x + 1 : dimX - 1;
        const int64 signedY = static_cast<int64>(y);
        const int p2 = at(x, signedY - 1);
        const int p3 = at(rightX, signedY - 1);
        const int p4 = at(rightX, signedY);
        const int p5 = at(rightX, signedY + 1);
        const int p6 = at(x, signedY + 1);
        const int p7 = at(leftX, signedY + 1);
        const int p8 = at(leftX, signedY);
        const int p9 = at(leftX, signedY - 1);
        const bool deletePixel = BinaryThinningShouldDelete(step, p2, p3, p4, p5, p6, p7, p8, p9);
        outputBlock[outputIndex] = deletePixel ? uint8{0} : uint8{1};
        changed = changed || deletePixel;
      }
    }

    if(shouldCancel)
    {
      return Result<bool>{false};
    }
    const usize outputValues = rowCount * dimX;
    if(Result<> result = destination.copyFromBuffer(rowBegin * dimX, nonstd::span<const uint8>(outputBlock.get(), outputValues)); result.invalid())
    {
      return ConvertResultTo<bool>(std::move(result), false);
    }
    rowBegin += rowCount;
  }
  return {changed};
}

inline Result<bool> ApplyBinaryThinningExternalSubstepTiles(BinaryThinningWorkStore& source, BinaryThinningWorkStore& destination, usize dimX, usize dimY, int step,
                                                            const std::atomic_bool& shouldCancel, const BinaryThinningExternal2DPlan& plan)
{
  usize volumeValues = 0;
  usize tileStride = 0;
  usize maximumInputValues = 0;
  if(dimX == 0 || dimY == 0 || step < 1 || step > 4 || !plan.useTiles || plan.coreRows != 1 || plan.coreColumns == 0 || plan.coreColumns > dimX ||
     !BinaryThinningCheckedMultiply(dimX, dimY, volumeValues) || !BinaryThinningCheckedAdd(plan.coreColumns, usize{2}, tileStride) ||
     !BinaryThinningCheckedMultiply(tileStride, usize{3}, maximumInputValues))
  {
    return MakeErrorResult<bool>(-8714, fmt::format("Binary thinning external tile substep {} has an invalid plan for dimensions {} x {}. Core rows: {}; core columns: {}; tiles: {}.", step, dimX,
                                                    dimY, plan.coreRows, plan.coreColumns, plan.useTiles));
  }
  if(source.getSize() != volumeValues || destination.getSize() != volumeValues)
  {
    return MakeErrorResult<bool>(-8714, fmt::format("Binary thinning external tile stores do not match the {}-value image. Source values: {}; destination values: {}; dimensions: {} x {}.",
                                                    volumeValues, source.getSize(), destination.getSize(), dimX, dimY));
  }

  auto inputTile = std::make_unique<uint8[]>(maximumInputValues);
  auto outputTile = std::make_unique<uint8[]>(plan.coreColumns);
  bool changed = false;
  for(usize y = 0; y < dimY; ++y)
  {
    for(usize xBegin = 0; xBegin < dimX;)
    {
      if(shouldCancel)
      {
        return Result<bool>{false};
      }
      const usize columnCount = std::min(plan.coreColumns, dimX - xBegin);
      const usize currentStride = columnCount + 2;
      const usize coreEnd = xBegin + columnCount;
      const usize readBegin = xBegin == 0 ? 0 : xBegin - 1;
      const usize readEnd = coreEnd < dimX ? coreEnd + 1 : dimX;
      const usize readCount = readEnd - readBegin;
      const usize destinationOffset = readBegin + 1 - xBegin;

      for(int rowOffset = -1; rowOffset <= 1; ++rowOffset)
      {
        const int64 sourceY = std::clamp<int64>(static_cast<int64>(y) + rowOffset, 0, static_cast<int64>(dimY) - 1);
        uint8* rowValues = inputTile.get() + static_cast<usize>(rowOffset + 1) * currentStride;
        if(Result<> result = source.copyIntoBuffer(static_cast<usize>(sourceY) * dimX + readBegin, nonstd::span<uint8>(rowValues + destinationOffset, readCount)); result.invalid())
        {
          return ConvertResultTo<bool>(std::move(result), false);
        }
        if(xBegin == 0)
        {
          rowValues[0] = rowValues[1];
        }
        if(coreEnd == dimX)
        {
          rowValues[columnCount + 1] = rowValues[columnCount];
        }
      }

      for(usize localX = 0; localX < columnCount; ++localX)
      {
        const usize centerX = localX + 1;
        const uint8 center = inputTile[currentStride + centerX];
        if(center == 0)
        {
          outputTile[localX] = 0;
          continue;
        }
        const int p2 = inputTile[centerX];
        const int p3 = inputTile[centerX + 1];
        const int p4 = inputTile[currentStride + centerX + 1];
        const int p5 = inputTile[2 * currentStride + centerX + 1];
        const int p6 = inputTile[2 * currentStride + centerX];
        const int p7 = inputTile[2 * currentStride + centerX - 1];
        const int p8 = inputTile[currentStride + centerX - 1];
        const int p9 = inputTile[centerX - 1];
        const bool deletePixel = BinaryThinningShouldDelete(step, p2, p3, p4, p5, p6, p7, p8, p9);
        outputTile[localX] = deletePixel ? uint8{0} : uint8{1};
        changed = changed || deletePixel;
      }

      if(shouldCancel)
      {
        return Result<bool>{false};
      }
      if(Result<> result = destination.copyFromBuffer(y * dimX + xBegin, nonstd::span<const uint8>(outputTile.get(), columnCount)); result.invalid())
      {
        return ConvertResultTo<bool>(std::move(result), false);
      }
      xBegin += columnCount;
    }
  }
  return {changed};
}

template <class T>
Result<> ApplyBinaryThinningExternal2D(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, const std::atomic_bool& shouldCancel, usize target2DBytes,
                                       usize inputValueOffset = 0, usize outputValueOffset = 0)
{
  if(shouldCancel)
  {
    return {};
  }
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }

  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  usize volumeValues = 0;
  if(dims[2] != 1 || dims[0] > k_Int64Max || dims[1] > k_Int64Max || !BinaryThinningCheckedMultiply(dims[0], dims[1], volumeValues) || volumeValues > k_Int64Max)
  {
    return MakeErrorResult(-8710,
                           fmt::format("Binary thinning external true-2-D execution requires one Z slice and signed-index-safe dimensions. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  usize inputEnd = 0;
  usize outputEnd = 0;
  if(!BinaryThinningCheckedAdd(inputValueOffset, volumeValues, inputEnd) || !BinaryThinningCheckedAdd(outputValueOffset, volumeValues, outputEnd) || inputEnd > inStore.getSize() ||
     outputEnd > outStore.getSize())
  {
    return MakeErrorResult(
        -8710, fmt::format("Binary thinning external true-2-D ranges do not fit their stores. Slice values: {}; input range: [{}, {}); input size: {}; output range: [{}, {}); output size: {}; "
                           "dimensions: {} x {} x {}.",
                           volumeValues, inputValueOffset, inputEnd, inStore.getSize(), outputValueOffset, outputEnd, outStore.getSize(), dims[0], dims[1], dims[2]));
  }

  auto planResult = CreateBinaryThinningExternal2DPlan(dims[0], dims[1], target2DBytes);
  if(planResult.invalid())
  {
    return ConvertResult(std::move(planResult));
  }
  const BinaryThinningExternal2DPlan plan = planResult.value();

  std::shared_ptr<AbstractDataStore<uint8>> firstDataStore;
  std::shared_ptr<AbstractDataStore<uint8>> secondDataStore;
  std::unique_ptr<SweepTemporaryStore<uint8>> firstFixedRecords;
  std::unique_ptr<SweepTemporaryStore<uint8>> secondFixedRecords;
  std::unique_ptr<BinaryThinningWorkStore> firstWork;
  std::unique_ptr<BinaryThinningWorkStore> secondWork;
  if(DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
  {
    auto firstResult = CreateSweepTemporaryStore<uint8>(volumeValues, std::max<usize>(1, target2DBytes), shouldCancel, "Binary-thinning first external work image");
    if(firstResult.invalid())
    {
      return ConvertResult(std::move(firstResult));
    }
    firstFixedRecords = std::move(firstResult.value());
    auto secondResult = CreateSweepTemporaryStore<uint8>(volumeValues, std::max<usize>(1, target2DBytes), shouldCancel, "Binary-thinning second external work image");
    if(secondResult.invalid())
    {
      return ConvertResult(std::move(secondResult));
    }
    secondFixedRecords = std::move(secondResult.value());
    firstWork = std::make_unique<BinaryThinningWorkStore>(*firstFixedRecords);
    secondWork = std::make_unique<BinaryThinningWorkStore>(*secondFixedRecords);
  }
  else
  {
    const std::string dataFormat = inStore.getStoreType() == IDataStore::StoreType::OutOfCore ? inStore.getDataFormat() : outStore.getDataFormat();
    firstDataStore = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(dataFormat, std::vector<usize>{1, dims[1], dims[0]}, std::vector<usize>{1});
    secondDataStore = DataStoreUtilities::CreateDataStoreWithFormat<uint8>(dataFormat, std::vector<usize>{1, dims[1], dims[0]}, std::vector<usize>{1});
    if(firstDataStore == nullptr || secondDataStore == nullptr || firstDataStore->getSize() != volumeValues || secondDataStore->getSize() != volumeValues)
    {
      return MakeErrorResult(-8713, fmt::format("Binary thinning could not create two {}-value uint8 external work stores in format '{}'.", volumeValues, dataFormat));
    }
    firstWork = std::make_unique<BinaryThinningWorkStore>(*firstDataStore);
    secondWork = std::make_unique<BinaryThinningWorkStore>(*secondDataStore);
  }

  if(target2DBytes <= k_BinaryThinning2DMetadataHeadroomBytes)
  {
    return MakeErrorResult(
        -8715, fmt::format("Binary thinning external typed transfer target ({} bytes) does not exceed its {}-byte metadata reserve.", target2DBytes, k_BinaryThinning2DMetadataHeadroomBytes));
  }
  const usize transferValues = (target2DBytes - k_BinaryThinning2DMetadataHeadroomBytes) / (sizeof(T) + sizeof(uint8));
  if(transferValues == 0)
  {
    return MakeErrorResult(-8715, fmt::format("Binary thinning external typed transfer target ({} bytes) cannot hold one {}-byte input value and one uint8 work value.", target2DBytes, sizeof(T)));
  }

  {
    auto typedValues = std::make_unique<T[]>(transferValues);
    auto binaryValues = std::make_unique<uint8[]>(transferValues);
    for(usize start = 0; start < volumeValues;)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(transferValues, volumeValues - start);
      if(Result<> result = inStore.copyIntoBuffer(inputValueOffset + start, nonstd::span<T>(typedValues.get(), count)); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      for(usize index = 0; index < count; ++index)
      {
        binaryValues[index] = typedValues[index] != T{} ? uint8{1} : uint8{0};
      }
      if(Result<> result = firstWork->copyFromBuffer(start, nonstd::span<const uint8>(binaryValues.get(), count)); result.invalid())
      {
        return result;
      }
      start += count;
    }
  }

  BinaryThinningWorkStore* source = firstWork.get();
  BinaryThinningWorkStore* destination = secondWork.get();
  bool changed = true;
  while(changed)
  {
    changed = false;
    for(int step = 1; step <= 4; ++step)
    {
      if(shouldCancel)
      {
        return {};
      }
      auto stepResult = plan.useTiles ? ApplyBinaryThinningExternalSubstepTiles(*source, *destination, dims[0], dims[1], step, shouldCancel, plan) :
                                        ApplyBinaryThinningExternalSubstepBlocks(*source, *destination, dims[0], dims[1], step, shouldCancel, plan);
      if(stepResult.invalid())
      {
        return ConvertResult(std::move(stepResult));
      }
      if(shouldCancel)
      {
        return {};
      }
      changed = changed || stepResult.value();
      std::swap(source, destination);
    }
  }

  {
    auto typedValues = std::make_unique<T[]>(transferValues);
    auto binaryValues = std::make_unique<uint8[]>(transferValues);
    for(usize start = 0; start < volumeValues;)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(transferValues, volumeValues - start);
      if(Result<> result = source->copyIntoBuffer(start, nonstd::span<uint8>(binaryValues.get(), count)); result.invalid())
      {
        return result;
      }
      if(shouldCancel)
      {
        return {};
      }
      for(usize index = 0; index < count; ++index)
      {
        typedValues[index] = static_cast<T>(binaryValues[index]);
      }
      if(Result<> result = outStore.copyFromBuffer(outputValueOffset + start, nonstd::span<const T>(typedValues.get(), count)); result.invalid())
      {
        return result;
      }
      start += count;
    }
  }
  return {};
}
} // namespace detail

template <class T>
Result<> ApplyBinaryThinning(const AbstractDataStore<T>& inStore, AbstractDataStore<T>& outStore, const SizeVec3& dims, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler, usize target2DBytes = detail::k_BinaryThinning2DTargetBytes)
{
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }

  const bool usesOutOfCoreEndpoint = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(dims[2] > 1)
  {
    usize maximumWorkers = 1;
#ifdef SIMPLNX_ENABLE_MULTICORE
    maximumWorkers = std::max<usize>(1, static_cast<usize>(std::thread::hardware_concurrency()));
#endif
    auto usefulPlanResult = detail::CreateBinaryThinningSliceBatchPlan<T>(dims, std::numeric_limits<usize>::max(), maximumWorkers);
    if(usefulPlanResult.invalid())
    {
      return ConvertResult(std::move(usefulPlanResult));
    }
    const auto& usefulPlan = usefulPlanResult.value();
    auto reservation = ReserveWorkingMemory(usefulPlan.residentBytes, usefulPlan.residentBytes);
    const usize grantedBytes = static_cast<usize>(std::min<uint64>(reservation.sizeBytes(), std::numeric_limits<usize>::max()));
    if(reservation.sizeBytes() >= usefulPlan.bytesPerWorker)
    {
      auto planResult = detail::CreateBinaryThinningSliceBatchPlan<T>(dims, grantedBytes, maximumWorkers);
      if(planResult.invalid())
      {
        return ConvertResult(std::move(planResult));
      }
      const auto& plan = planResult.value();
      reservation.shrinkTo(plan.residentBytes);
      return detail::ApplyBinaryThinningSliceBatches(inStore, outStore, dims, shouldCancel, plan);
    }

    if(usesOutOfCoreEndpoint)
    {
      constexpr usize k_MinimumExternalTargetBytes = detail::BinaryThinningMinimumExternal2DTargetBytes<T>();
      if(grantedBytes < k_MinimumExternalTargetBytes)
      {
        return MakeErrorResult(-8716,
                               fmt::format("Binary thinning cannot run a bounded 3D OOC slice with the current working-memory grant. Dimensions: {} x {} x {}; value bytes: {}; granted bytes: {}; "
                                           "minimum external target: {}; cache budget: {}; maximum working memory: {}.",
                                           dims[0], dims[1], dims[2], sizeof(T), grantedBytes, k_MinimumExternalTargetBytes, CacheMemoryBudgetManager::instance().budgetBytes(),
                                           CacheMemoryBudgetManager::instance().maximumWorkingMemoryBytes()));
      }
      const SizeVec3 sliceDims{dims[0], dims[1], 1};
      for(usize z = 0; z < dims[2]; ++z)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize valueOffset = z * usefulPlan.sliceValues;
        if(Result<> result = detail::ApplyBinaryThinningExternal2D(inStore, outStore, sliceDims, shouldCancel, grantedBytes, valueOffset, valueOffset); result.invalid())
        {
          return result;
        }
      }
      return {};
    }
  }

  if(dims[2] != 1 || !usesOutOfCoreEndpoint)
  {
    return detail::ApplyBinaryThinningResident(inStore, outStore, dims, shouldCancel, messageHandler);
  }

  usize volumeValues = 0;
  if(!detail::BinaryThinningCheckedMultiply(dims[0], dims[1], volumeValues))
  {
    return MakeErrorResult(-8710, fmt::format("Binary thinning true-2-D dimensions overflow the addressable value count. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  auto usefulPlanResult = detail::CreateBinaryThinningFixed2DPlan<T>(volumeValues, std::numeric_limits<usize>::max());
  if(usefulPlanResult.invalid())
  {
    return ConvertResult(std::move(usefulPlanResult));
  }
  const auto& usefulPlan = usefulPlanResult.value();
  const uint64 preferredBytes = std::max<uint64>(target2DBytes, usefulPlan.minimumFixedBytes);
  auto reservation = ReserveWorkingMemory(preferredBytes, usefulPlan.residentBytes);
  const usize grantedBytes = static_cast<usize>(std::min<uint64>(reservation.sizeBytes(), std::numeric_limits<usize>::max()));
  if(grantedBytes < detail::BinaryThinningMinimumExternal2DTargetBytes<T>())
  {
    return MakeErrorResult(-8711,
                           fmt::format("Binary thinning cannot run a bounded true-2-D OOC image with the current working-memory grant. Dimensions: {} x {} x {}; value bytes: {}; granted bytes: {}; "
                                       "minimum external target: {}; cache budget: {}; maximum working memory: {}.",
                                       dims[0], dims[1], dims[2], sizeof(T), grantedBytes, detail::BinaryThinningMinimumExternal2DTargetBytes<T>(), CacheMemoryBudgetManager::instance().budgetBytes(),
                                       CacheMemoryBudgetManager::instance().maximumWorkingMemoryBytes()));
  }
  auto fixedPlanResult = detail::CreateBinaryThinningFixed2DPlan<T>(volumeValues, grantedBytes);
  if(fixedPlanResult.invalid())
  {
    return ConvertResult(std::move(fixedPlanResult));
  }
  const detail::BinaryThinningFixed2DPlan& fixedPlan = fixedPlanResult.value();
  if(fixedPlan.useFixedCapacity)
  {
    reservation.shrinkTo(fixedPlan.residentBytes);
    return detail::ApplyBinaryThinningFixed2D(inStore, outStore, dims, shouldCancel, fixedPlan);
  }
  return detail::ApplyBinaryThinningExternal2D(inStore, outStore, dims, shouldCancel, grantedBytes);
}
} // namespace nx::core::ImageProcessing
