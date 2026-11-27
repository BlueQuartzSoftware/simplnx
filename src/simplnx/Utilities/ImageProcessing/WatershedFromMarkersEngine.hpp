#pragma once

#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/WatershedExternalMemory.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <map>
#include <new> // std::bad_alloc
#include <queue>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
// A radius-1 neighborhood offset (dx,dy,dz), each in {-1,0,1}.
struct WsNeighborOffset
{
  int64 dx;
  int64 dy;
  int64 dz;
};

inline constexpr int32 k_WatershedResidentOutOfMemory = -79040;

/**
 * @brief Build the radius-1 connectivity offset set in ITK's ConstShapedNeighborhoodIterator activation order.
 *
 * ITK's setConnectivity() activates neighbor offsets, and ConstShapedNeighborhoodIterator::ActivateIndex inserts them
 * into m_ActiveIndexList sorted ascending by neighborhood linear index n = sum_d (offset[d]+1) * 3^d (x fastest). So
 * Begin()..End() iterates neighbors in ascending-n order, which is the raster order below (oz outermost, ox innermost),
 * skipping the center and any inactive offset. face (!fullyConnected): the 2*effDim axial offsets (Manhattan dist 1);
 * full: all 3^effDim - 1 offsets. effDim = (nZ>1)?3:2 -- a 2D volume drops the z offsets. This is exact vs ITK's native
 * 3D neighborhood for a single z-plane: the OOB z-neighbors ITK would generate never seed the FAH, never cause a
 * collision, and never relabel (their boundary reads are all neutral), and dropping them does not change the relative
 * order of the in-bounds x/y neighbors (n for the oz==0 slice is monotone in the 2D index).
 */
inline std::vector<WsNeighborOffset> BuildWatershedOffsets(int effDim, bool fullyConnected)
{
  std::vector<WsNeighborOffset> offsets;
  const int64 zLo = (effDim == 3) ? -1 : 0;
  const int64 zHi = (effDim == 3) ? 1 : 0;
  for(int64 oz = zLo; oz <= zHi; ++oz)
  {
    for(int64 oy = -1; oy <= 1; ++oy)
    {
      for(int64 ox = -1; ox <= 1; ++ox)
      {
        if(ox == 0 && oy == 0 && oz == 0)
        {
          continue; // skip the center
        }
        const int64 manhattan = std::abs(ox) + std::abs(oy) + std::abs(oz);
        if(!fullyConnected && manhattan != 1)
        {
          continue; // face connectivity: only axial neighbors
        }
        offsets.push_back(WsNeighborOffset{.dx = ox, .dy = oy, .dz = oz});
      }
    }
  }
  return offsets;
}

template <class TInput>
class WatershedInputRecordStoreView final : public ITemporaryRecordStore
{
public:
  WatershedInputRecordStoreView(const AbstractDataStore<TInput>& store, uint64 maxRecordsPerBatch)
  : m_Store(store)
  , m_MaxRecordsPerBatch(maxRecordsPerBatch)
  {
  }

  uint64 recordSize() const override
  {
    return sizeof(TInput);
  }

  uint64 recordCount() const override
  {
    return m_Store.getSize();
  }

  uint64 maxRecordsPerBatch() const override
  {
    return m_MaxRecordsPerBatch;
  }

  bool isReadOnly() const override
  {
    return true;
  }

  Result<uint64> read(uint64 recordOffset, uint64 requestedRecordCount, nonstd::span<std::byte> records, const std::atomic_bool& shouldCancel) const override
  {
    const uint64 storeSize = m_Store.getSize();
    const bool rangeInvalid = recordOffset > storeSize || requestedRecordCount > storeSize - recordOffset || requestedRecordCount > m_MaxRecordsPerBatch;
    const bool byteCountOverflow = requestedRecordCount > std::numeric_limits<usize>::max() / sizeof(TInput);
    const usize requiredBytes = byteCountOverflow ? 0 : static_cast<usize>(requestedRecordCount) * sizeof(TInput);
    const bool alignmentInvalid = reinterpret_cast<std::uintptr_t>(records.data()) % alignof(TInput) != 0;
    if(shouldCancel || rangeInvalid || byteCountOverflow || records.size() < requiredBytes || alignmentInvalid)
    {
      return MakeErrorResult<uint64>(
          -79071,
          fmt::format("External watershed input-page read is invalid. Offset: {}; records: {}; store records: {}; maximum batch: {}; buffer bytes: {}; required bytes: {}; aligned: {}; cancelled: {}.",
                      recordOffset, requestedRecordCount, storeSize, m_MaxRecordsPerBatch, records.size(), requiredBytes, !alignmentInvalid, shouldCancel.load()));
    }
    if(requestedRecordCount == 0)
    {
      return {0};
    }
    auto values = nonstd::span<TInput>(reinterpret_cast<TInput*>(records.data()), static_cast<usize>(requestedRecordCount));
    auto result = m_Store.copyIntoBuffer(static_cast<usize>(recordOffset), values);
    if(result.invalid())
    {
      return ConvertResultTo<uint64>(std::move(result), uint64{0});
    }
    return {requestedRecordCount};
  }

  Result<> write(uint64 recordOffset, uint64 requestedRecordCount, nonstd::span<const std::byte> /*records*/, const std::atomic_bool& /*shouldCancel*/) override
  {
    return MakeErrorResult(-79071, fmt::format("External watershed input-page view is read-only. Write offset: {}; records: {}.", recordOffset, requestedRecordCount));
  }

  Result<> fill(uint64 recordOffset, uint64 requestedRecordCount, nonstd::span<const std::byte> /*record*/, const std::atomic_bool& /*shouldCancel*/) override
  {
    return MakeErrorResult(-79071, fmt::format("External watershed input-page view is read-only. Fill offset: {}; records: {}.", recordOffset, requestedRecordCount));
  }

  Result<> resize(uint64 recordCount, const std::atomic_bool& /*shouldCancel*/) override
  {
    return MakeErrorResult(-79071, fmt::format("External watershed input-page view is read-only. Requested record count: {}.", recordCount));
  }

private:
  const AbstractDataStore<TInput>& m_Store;
  uint64 m_MaxRecordsPerBatch = 0;
};
} // namespace detail

template <class TInput, bool SplitBucketState, bool PackedCombinedState, bool PackedResidentStatusState>
Result<> ApplyWatershedFromMarkersExternalImpl(const AbstractDataStore<TInput>& inStore, const AbstractDataStore<uint32>& markerStore, AbstractDataStore<uint32>& outStore, const SizeVec3Type& dims,
                                               bool markWatershedLine, bool fullyConnected, uint32 /*borderSentinel*/, const std::atomic_bool& shouldCancel,
                                               std::unique_ptr<ITemporaryRecordStore> voxelRecordStore, std::unique_ptr<ITemporaryRecordStore> queueRecordStore,
                                               const detail::WatershedExternalMemoryPlan<TInput>& plan, const IFilter::MessageHandler& messageHandler = {})
{
  messageHandler.sendInfoMessage("Computing Watershed From Markers");
  ThrottledMessageHandler progressThrottle(messageHandler);
  using VoxelRecordType = detail::WatershedVoxelRecord<TInput>;
  using PhysicalVoxelRecordType = std::conditional_t<SplitBucketState, detail::WatershedBucketVoxelRecord,
                                                     std::conditional_t<PackedResidentStatusState, detail::WatershedPackedResidentStatusVoxelRecordType<TInput>,
                                                                        std::conditional_t<PackedCombinedState, detail::WatershedPackedVoxelRecordType<TInput>, VoxelRecordType>>>;
  using QueueType = detail::WatershedExternalQueue<TInput>;
  static_assert(!SplitBucketState || detail::k_UseWatershedBucketQueue<TInput>);
  static_assert(!PackedCombinedState || (detail::k_UseWatershedBucketQueue<TInput> && !SplitBucketState));
  static_assert(!PackedResidentStatusState || (detail::k_UseWatershedBucketQueue<TInput> && !SplitBucketState && !PackedCombinedState));
  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return {};
  }
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  usize sliceValues = 0;
  usize valueCount = 0;
  if(dims[0] > k_Int64Max || dims[1] > k_Int64Max || dims[2] > k_Int64Max || !detail::TryMultiplyWatershedSize(dims[0], dims[1], sliceValues) ||
     !detail::TryMultiplyWatershedSize(sliceValues, dims[2], valueCount) || valueCount > k_Int64Max)
  {
    return MakeErrorResult(-79055, fmt::format("External watershed dimensions overflow the supported signed flat-index range: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  if(inStore.getSize() != valueCount || markerStore.getSize() != valueCount || outStore.getSize() != valueCount)
  {
    return MakeErrorResult(-79055, fmt::format("External watershed store sizes must equal the {}-value image volume. Input: {}; marker: {}; output: {}.", valueCount, inStore.getSize(),
                                               markerStore.getSize(), outStore.getSize()));
  }
  detail::WatershedTiledRecordLayout tiledLayout;
  usize physicalVoxelRecordCount = valueCount;
  if(plan.use_tiled_voxel_layout)
  {
    auto layoutResult = detail::CreateWatershedTiledRecordLayout(dims, plan.voxel_records_per_page);
    if(layoutResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(layoutResult));
    }
    tiledLayout = layoutResult.value();
    physicalVoxelRecordCount = tiledLayout.record_count;
  }
  if(plan.use_split_bucket_state != SplitBucketState || plan.use_packed_combined_state != PackedCombinedState || plan.use_packed_resident_status_state != PackedResidentStatusState)
  {
    return MakeErrorResult(
        -79056, fmt::format("External watershed state-plan mismatch. Planned split state: {}; selected split state: {}; planned packed combined state: {}; selected packed combined state: {}; "
                            "planned packed resident-status state: {}; selected packed resident-status state: {}.",
                            plan.use_split_bucket_state, SplitBucketState, plan.use_packed_combined_state, PackedCombinedState, plan.use_packed_resident_status_state, PackedResidentStatusState));
  }
  if(voxelRecordStore == nullptr || voxelRecordStore->recordSize() != sizeof(PhysicalVoxelRecordType) || voxelRecordStore->recordCount() != physicalVoxelRecordCount ||
     voxelRecordStore->maxRecordsPerBatch() < std::max(plan.voxel_records_per_page, plan.transfer_records))
  {
    return MakeErrorResult(-79056, "External watershed voxel-record provider returned mismatched storage metadata.");
  }
  const bool bucketStoreMismatch = plan.use_bucket_queue && (queueRecordStore == nullptr || queueRecordStore->recordSize() != sizeof(detail::WatershedQueueBlock) ||
                                                           queueRecordStore->recordCount() != plan.bucket_block_count || queueRecordStore->maxRecordsPerBatch() < plan.bucket_blocks_per_page);
  const bool heapStoreMismatch = !plan.use_bucket_queue && (queueRecordStore == nullptr || queueRecordStore->recordSize() != sizeof(detail::WatershedHeapRecord<TInput>) ||
                                                          queueRecordStore->recordCount() != valueCount || queueRecordStore->maxRecordsPerBatch() < plan.heap_records_per_page);
  if(bucketStoreMismatch || heapStoreMismatch)
  {
    return MakeErrorResult(
        -79056,
        fmt::format("External watershed queue-record provider returned mismatched storage metadata. Queue kind: {}; record bytes: {}; records: {}; provider batch: {}; expected record bytes: {}; "
                    "expected records: {}; expected batch: {}.",
                    plan.use_bucket_queue ? "bucket blocks" : "minimum heap", queueRecordStore == nullptr ? 0 : queueRecordStore->recordSize(),
                    queueRecordStore == nullptr ? 0 : queueRecordStore->recordCount(), queueRecordStore == nullptr ? 0 : queueRecordStore->maxRecordsPerBatch(),
                    plan.use_bucket_queue ? sizeof(detail::WatershedQueueBlock) : sizeof(detail::WatershedHeapRecord<TInput>), plan.use_bucket_queue ? plan.bucket_block_count : valueCount,
                    plan.use_bucket_queue ? plan.bucket_blocks_per_page : plan.heap_records_per_page));
  }
  if(plan.use_resident_status && (!markWatershedLine || plan.resident_status_word_count == 0 || plan.resident_status_bytes != plan.resident_status_word_count * sizeof(uint64)))
  {
    return MakeErrorResult(-79056, fmt::format("External watershed resident-status plan is inconsistent. Watershed line: {}; status words: {}; status bytes: {}.", markWatershedLine,
                                               plan.resident_status_word_count, plan.resident_status_bytes));
  }

  auto describeStoreError = [](const auto& result) { return result.errors().empty() ? std::string{"provider returned an unspecified error"} : result.errors().front().message; };
  std::vector<uint64> residentStatus;
  if(plan.use_resident_status)
  {
    try
    {
      residentStatus.assign(plan.resident_status_word_count, uint64{0});
    } catch(const std::exception& exception)
    {
      return MakeErrorResult(-79056,
                             fmt::format("External watershed could not allocate {} resident status words ({} bytes): {}", plan.resident_status_word_count, plan.resident_status_bytes, exception.what()));
    }
  }

  auto encodePhysicalVoxel = [](const VoxelRecordType& record) {
    if constexpr(SplitBucketState)
    {
      return PhysicalVoxelRecordType{record.output};
    }
    else if constexpr(PackedCombinedState)
    {
      return detail::EncodeWatershedPackedVoxelRecord(record);
    }
    else if constexpr(PackedResidentStatusState)
    {
      return detail::EncodeWatershedPackedResidentStatusVoxelRecord(record);
    }
    else
    {
      return record;
    }
  };
  auto decodePhysicalVoxel = [](const PhysicalVoxelRecordType& record) {
    if constexpr(SplitBucketState)
    {
      return VoxelRecordType{record.output, uint8{0}, TInput{}};
    }
    else if constexpr(PackedCombinedState)
    {
      return detail::DecodeWatershedPackedVoxelRecord<TInput>(record);
    }
    else if constexpr(PackedResidentStatusState)
    {
      return detail::DecodeWatershedPackedResidentStatusVoxelRecord<TInput>(record);
    }
    else
    {
      return record;
    }
  };

  messageHandler.sendInfoMessage("Initializing Watershed Regions");
  progressThrottle.reset(valueCount, "Initializing Watershed Values");
  if(plan.use_tiled_voxel_layout)
  {
    std::vector<TInput> inputBuffer(plan.transfer_records);
    std::vector<uint32> markerBuffer(plan.transfer_records);
    std::vector<PhysicalVoxelRecordType> recordBuffer(plan.transfer_records);
    if(plan.transfer_records < plan.voxel_records_per_page)
    {
      return MakeErrorResult(
          -79057, fmt::format("External watershed tiled initialization requires at least {} transfer records. Planned transfer records: {}.", plan.voxel_records_per_page, plan.transfer_records));
    }
    const usize tileX = tiledLayout.tile_dimensions[0];
    const usize tileY = tiledLayout.tile_dimensions[1];
    const usize tileZ = tiledLayout.tile_dimensions[2];
    for(usize tileZIndex = 0; tileZIndex < tiledLayout.tile_counts[2]; ++tileZIndex)
    {
      const usize zStart = tileZIndex * tileZ;
      const usize validZ = std::min(tileZ, dims[2] - zStart);
      for(usize tileYIndex = 0; tileYIndex < tiledLayout.tile_counts[1]; ++tileYIndex)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize yStart = tileYIndex * tileY;
        const usize validY = std::min(tileY, dims[1] - yStart);
        const usize bandRows = validZ * validY;
        const usize maximumGroupWidth = std::max(tileX, plan.transfer_records / bandRows);
        const usize maximumGroupTiles = std::max<usize>(1, maximumGroupWidth / tileX);
        for(usize firstTileX = 0; firstTileX < tiledLayout.tile_counts[0]; firstTileX += maximumGroupTiles)
        {
          const usize groupTileCount = std::min(maximumGroupTiles, tiledLayout.tile_counts[0] - firstTileX);
          const usize xStart = firstTileX * tileX;
          const usize groupWidth = std::min(groupTileCount * tileX, dims[0] - xStart);
          for(usize localZ = 0; localZ < validZ; ++localZ)
          {
            for(usize localY = 0; localY < validY; ++localY)
            {
              const usize logicalStart = ((zStart + localZ) * dims[1] + yStart + localY) * dims[0] + xStart;
              const usize bandOffset = (localZ * validY + localY) * groupWidth;
              if(Result<> result = inStore.copyIntoBuffer(logicalStart, nonstd::span<TInput>(inputBuffer.data() + bandOffset, groupWidth)); result.invalid())
              {
                return result;
              }
              if(Result<> result = markerStore.copyIntoBuffer(logicalStart, nonstd::span<uint32>(markerBuffer.data() + bandOffset, groupWidth)); result.invalid())
              {
                return result;
              }
            }
          }

          for(usize groupTileX = 0; groupTileX < groupTileCount; ++groupTileX)
          {
            std::fill_n(recordBuffer.begin(), plan.voxel_records_per_page, PhysicalVoxelRecordType{});
            const usize tileStartInGroup = groupTileX * tileX;
            const usize validX = std::min(tileX, groupWidth - tileStartInGroup);
            for(usize localZ = 0; localZ < validZ; ++localZ)
            {
              for(usize localY = 0; localY < validY; ++localY)
              {
                for(usize localX = 0; localX < validX; ++localX)
                {
                  const usize bandOffset = (localZ * validY + localY) * groupWidth + tileStartInGroup + localX;
                  const usize localIndex = (localZ * tileY + localY) * tileX + localX;
                  const uint32 marker = markerBuffer[bandOffset];
                  const bool isMarker = marker != uint32{0};
                  const uint8 status = markWatershedLine && isMarker && !plan.use_resident_status ? uint8{1} : uint8{0};
                  recordBuffer[localIndex] = encodePhysicalVoxel(VoxelRecordType{marker, status, inputBuffer[bandOffset]});
                  if(markWatershedLine && isMarker && plan.use_resident_status)
                  {
                    const usize logicalIndex = ((zStart + localZ) * dims[1] + yStart + localY) * dims[0] + xStart + tileStartInGroup + localX;
                    residentStatus[logicalIndex / 64] |= uint64{1} << (logicalIndex % 64);
                  }
                }
              }
            }
            const usize tileIndex = (tileZIndex * tiledLayout.tile_counts[1] + tileYIndex) * tiledLayout.tile_counts[0] + firstTileX + groupTileX;
            nonstd::span<const PhysicalVoxelRecordType> records(recordBuffer.data(), plan.voxel_records_per_page);
            nonstd::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(records.data()), records.size() * sizeof(PhysicalVoxelRecordType));
            Result<> writeResult = voxelRecordStore->write(tileIndex * plan.voxel_records_per_page, plan.voxel_records_per_page, bytes, shouldCancel);
            if(writeResult.invalid())
            {
              return MakeErrorResult(-79057, fmt::format("External watershed tiled voxel initialization write for tile {} failed: {}", tileIndex, describeStoreError(writeResult)));
            }
          }

          progressThrottle.incrementCount(validZ * validY * groupWidth);
        }
      }
    }
  }
  else
  {
    std::vector<TInput> inputBuffer(plan.transfer_records);
    std::vector<uint32> markerBuffer(plan.transfer_records);
    std::vector<PhysicalVoxelRecordType> recordBuffer(plan.transfer_records);
    for(usize start = 0; start < valueCount; start += plan.transfer_records)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(plan.transfer_records, valueCount - start);
      if(Result<> result = inStore.copyIntoBuffer(start, nonstd::span<TInput>(inputBuffer.data(), count)); result.invalid())
      {
        return result;
      }
      if(Result<> result = markerStore.copyIntoBuffer(start, nonstd::span<uint32>(markerBuffer.data(), count)); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < count; ++index)
      {
        const uint32 marker = markerBuffer[index];
        const bool isMarker = marker != uint32{0};
        const uint8 status = markWatershedLine && isMarker && !plan.use_resident_status ? uint8{1} : uint8{0};
        recordBuffer[index] = encodePhysicalVoxel(VoxelRecordType{marker, status, inputBuffer[index]});
        if(markWatershedLine && isMarker && plan.use_resident_status)
        {
          const usize flatIndex = start + index;
          residentStatus[flatIndex / 64] |= uint64{1} << (flatIndex % 64);
        }
      }
      nonstd::span<const PhysicalVoxelRecordType> records(recordBuffer.data(), count);
      nonstd::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(records.data()), records.size() * sizeof(PhysicalVoxelRecordType));
      Result<> writeResult = voxelRecordStore->write(start, count, bytes, shouldCancel);
      if(writeResult.invalid())
      {
        return MakeErrorResult(-79057, fmt::format("External watershed voxel initialization write at offset {} failed: {}", start, describeStoreError(writeResult)));
      }

      progressThrottle.updateCount(start + count);
    }
  }

  BoundedRecordPageCache<PhysicalVoxelRecordType> voxels(*voxelRecordStore, plan.voxel_records_per_page, plan.voxel_cache_pages);
  detail::WatershedInputRecordStoreView<TInput> inputRecordView(inStore, plan.input_records_per_page);
  BoundedRecordPageCache<TInput> inputValues(inputRecordView, plan.input_records_per_page, plan.input_cache_pages);
  auto queueResult = QueueType::Create(std::move(queueRecordStore), plan, valueCount, shouldCancel);
  if(queueResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(queueResult));
  }
  std::unique_ptr<QueueType> queue = std::move(queueResult.value());

  const int64 dimX = static_cast<int64>(dims[0]);
  const int64 dimY = static_cast<int64>(dims[1]);
  const int64 dimZ = static_cast<int64>(dims[2]);
  const int effectiveDimension = dimZ > 1 ? 3 : 2;
  const std::vector<detail::WsNeighborOffset> offsets = detail::BuildWatershedOffsets(effectiveDimension, fullyConnected);
  struct Neighbor
  {
    int64 dx;
    int64 dy;
    int64 dz;
    int64 flat_delta;
  };
  std::vector<Neighbor> neighbors;
  neighbors.reserve(offsets.size());
  for(const detail::WsNeighborOffset& offset : offsets)
  {
    neighbors.push_back(Neighbor{offset.dx, offset.dy, offset.dz, (offset.dz * dimY + offset.dy) * dimX + offset.dx});
  }

  auto decode = [dimX, dimY](int64 flatIndex, int64& x, int64& y, int64& z) {
    x = flatIndex % dimX;
    const int64 remainder = flatIndex / dimX;
    y = remainder % dimY;
    z = remainder / dimY;
  };
  auto physicalIndex = [&](int64 flatIndex) {
    const usize logicalIndex = static_cast<usize>(flatIndex);
    return plan.use_tiled_voxel_layout ? tiledLayout.physicalIndexUnchecked(logicalIndex) : logicalIndex;
  };
  auto readVoxelAt = [&](int64 flatIndex, usize recordIndex) -> Result<VoxelRecordType> {
    auto result = voxels.read(recordIndex, shouldCancel);
    if(result.invalid())
    {
      return MakeErrorResult<VoxelRecordType>(-79058, fmt::format("External watershed voxel read at flat index {} failed: {}", flatIndex, describeStoreError(result)));
    }
    return {decodePhysicalVoxel(result.value())};
  };
  auto readVoxel = [&](int64 flatIndex) { return readVoxelAt(flatIndex, physicalIndex(flatIndex)); };
  auto writeVoxelAt = [&](int64 flatIndex, usize recordIndex, const VoxelRecordType& record) -> Result<> {
    Result<> result;
    result = voxels.write(recordIndex, encodePhysicalVoxel(record), shouldCancel);
    if(result.invalid())
    {
      return MakeErrorResult(-79059, fmt::format("External watershed voxel write at flat index {} failed: {}", flatIndex, describeStoreError(result)));
    }
    return {};
  };
  auto writeVoxel = [&](int64 flatIndex, const VoxelRecordType& record) { return writeVoxelAt(flatIndex, physicalIndex(flatIndex), record); };
  auto isStatusSet = [&](int64 flatIndex, const VoxelRecordType& record) {
    if(plan.use_resident_status)
    {
      const usize index = static_cast<usize>(flatIndex);
      return (residentStatus[index / 64] & (uint64{1} << (index % 64))) != 0;
    }
    return record.status != 0;
  };
  auto setStatus = [&](int64 flatIndex, VoxelRecordType& record) {
    if(plan.use_resident_status)
    {
      const usize index = static_cast<usize>(flatIndex);
      residentStatus[index / 64] |= uint64{1} << (index % 64);
      return false;
    }
    record.status = uint8{1};
    return true;
  };
  auto readGrayValue = [&](int64 flatIndex, const VoxelRecordType& record) -> Result<TInput> {
    if constexpr(SplitBucketState)
    {
      auto result = inputValues.read(static_cast<uint64>(flatIndex), shouldCancel);
      if(result.invalid())
      {
        return MakeErrorResult<TInput>(-79058, fmt::format("External watershed grayscale read at flat index {} failed: {}", flatIndex, describeStoreError(result)));
      }
      return result;
    }
    else
    {
      return {record.gray_value};
    }
  };

  constexpr uint32 k_BackgroundLabel = 0u;
  constexpr uint32 k_WatershedLabel = 0u;
  messageHandler.sendInfoMessage("Seeding Watershed Regions");
  if(markWatershedLine)
  {
    progressThrottle.reset(valueCount, "Seeding Watershed Values");
    for(int64 flatIndex = 0; flatIndex < static_cast<int64>(valueCount); ++flatIndex)
    {
      if((static_cast<uint64>(flatIndex) & 0xFFFFu) == 0 && shouldCancel)
      {
        return {};
      }
      if(flatIndex != 0 && (static_cast<uint64>(flatIndex) & 0xFFFFu) == 0)
      {
        progressThrottle.updateCount(static_cast<usize>(flatIndex));
      }
      auto recordResult = readVoxel(flatIndex);
      if(recordResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(recordResult));
      }
      VoxelRecordType record = recordResult.value();
      if(record.output == k_BackgroundLabel)
      {
        continue;
      }
      int64 x = 0;
      int64 y = 0;
      int64 z = 0;
      decode(flatIndex, x, y, z);
      for(const Neighbor& offset : neighbors)
      {
        const int64 neighborX = x + offset.dx;
        const int64 neighborY = y + offset.dy;
        const int64 neighborZ = z + offset.dz;
        if(neighborX < 0 || neighborX >= dimX || neighborY < 0 || neighborY >= dimY || neighborZ < 0 || neighborZ >= dimZ)
        {
          continue;
        }
        const int64 neighborFlat = flatIndex + offset.flat_delta;
        auto neighborResult = readVoxel(neighborFlat);
        if(neighborResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(neighborResult));
        }
        VoxelRecordType neighbor = neighborResult.value();
        if(!isStatusSet(neighborFlat, neighbor) && neighbor.output == k_BackgroundLabel)
        {
          auto grayValueResult = readGrayValue(neighborFlat, neighbor);
          if(grayValueResult.invalid())
          {
            return ConvertInvalidResult<void>(std::move(grayValueResult));
          }
          if(Result<> result = queue->push(grayValueResult.value(), neighborFlat); result.invalid())
          {
            return result;
          }
          if(setStatus(neighborFlat, neighbor))
          {
            if(Result<> result = writeVoxel(neighborFlat, neighbor); result.invalid())
            {
              return result;
            }
          }
        }
      }
    }

    progressThrottle.updateCount(valueCount);
    messageHandler.sendInfoMessage("Flooding Watershed Regions");
    progressThrottle.reset(0, "Flooding Watershed Regions");
    uint64 popCount = 0;
    std::array<uint64, 26> cachedNeighborIndices = {};
    std::array<uint64, 26> cachedPhysicalNeighborIndices = {};
    std::array<PhysicalVoxelRecordType, 26> cachedPhysicalNeighborRecords = {};
    std::array<VoxelRecordType, 26> cachedNeighborRecords = {};
    while(!queue->empty())
    {
      if(((++popCount & 0xFFFFu) == 0) && shouldCancel)
      {
        return {};
      }
      if((popCount & 0xFFFFu) == 0)
      {
        progressThrottle.queueMessage("Flooding Watershed Regions: {} queue entries processed", popCount - 1);
      }
      auto queuedVoxelResult = queue->pop();
      if(queuedVoxelResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(queuedVoxelResult));
      }
      const auto queueRecord = queuedVoxelResult.value();
      const int64 flatIndex = queueRecord.flat_index;
      int64 x = 0;
      int64 y = 0;
      int64 z = 0;
      decode(flatIndex, x, y, z);
      const usize currentPhysicalIndex = physicalIndex(flatIndex);

      uint32 marker = k_WatershedLabel;
      bool collision = false;
      usize cachedNeighborCount = 0;
      for(const Neighbor& offset : neighbors)
      {
        const int64 neighborX = x + offset.dx;
        const int64 neighborY = y + offset.dy;
        const int64 neighborZ = z + offset.dz;
        if(neighborX >= 0 && neighborX < dimX && neighborY >= 0 && neighborY < dimY && neighborZ >= 0 && neighborZ < dimZ)
        {
          const int64 neighborFlatIndex = flatIndex + offset.flat_delta;
          cachedNeighborIndices[cachedNeighborCount] = static_cast<uint64>(neighborFlatIndex);
          cachedPhysicalNeighborIndices[cachedNeighborCount] = physicalIndex(neighborFlatIndex);
          ++cachedNeighborCount;
        }
      }
      auto neighborReadResult = voxels.readMany(nonstd::span<const uint64>(cachedPhysicalNeighborIndices.data(), cachedNeighborCount),
                                                nonstd::span<PhysicalVoxelRecordType>(cachedPhysicalNeighborRecords.data(), cachedNeighborCount), shouldCancel);
      if(neighborReadResult.invalid())
      {
        return MakeErrorResult(-79058, fmt::format("External watershed neighbor read for flat index {} failed: {}", flatIndex, describeStoreError(neighborReadResult)));
      }
      for(usize neighborIndex = 0; neighborIndex < cachedNeighborCount; ++neighborIndex)
      {
        cachedNeighborRecords[neighborIndex] = decodePhysicalVoxel(cachedPhysicalNeighborRecords[neighborIndex]);
        const uint32 output = cachedNeighborRecords[neighborIndex].output;
        if(output != k_WatershedLabel)
        {
          if(marker != k_WatershedLabel && output != marker)
          {
            collision = true;
            break;
          }
          marker = output;
        }
      }
      if(collision)
      {
        continue;
      }
      auto currentResult = readVoxelAt(flatIndex, currentPhysicalIndex);
      if(currentResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(currentResult));
      }
      VoxelRecordType current = currentResult.value();
      current.output = marker;
      if(Result<> result = writeVoxelAt(flatIndex, currentPhysicalIndex, current); result.invalid())
      {
        return result;
      }
      for(usize neighborIndex = 0; neighborIndex < cachedNeighborCount; ++neighborIndex)
      {
        const int64 neighborFlat = static_cast<int64>(cachedNeighborIndices[neighborIndex]);
        VoxelRecordType neighbor = cachedNeighborRecords[neighborIndex];
        if(!isStatusSet(neighborFlat, neighbor))
        {
          auto grayValueResult = readGrayValue(neighborFlat, neighbor);
          if(grayValueResult.invalid())
          {
            return ConvertInvalidResult<void>(std::move(grayValueResult));
          }
          if(Result<> result = queue->push(detail::WatershedEffectiveLevel(grayValueResult.value(), queueRecord.level), neighborFlat); result.invalid())
          {
            return result;
          }
          if(setStatus(neighborFlat, neighbor))
          {
            if(Result<> result = writeVoxelAt(neighborFlat, cachedPhysicalNeighborIndices[neighborIndex], neighbor); result.invalid())
            {
              return result;
            }
          }
        }
      }
    }
  }
  else
  {
    progressThrottle.reset(valueCount, "Seeding Watershed Values");
    for(int64 flatIndex = 0; flatIndex < static_cast<int64>(valueCount); ++flatIndex)
    {
      if((static_cast<uint64>(flatIndex) & 0xFFFFu) == 0 && shouldCancel)
      {
        return {};
      }
      if(flatIndex != 0 && (static_cast<uint64>(flatIndex) & 0xFFFFu) == 0)
      {
        progressThrottle.updateCount(static_cast<usize>(flatIndex));
      }
      auto recordResult = readVoxel(flatIndex);
      if(recordResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(recordResult));
      }
      VoxelRecordType record = recordResult.value();
      if(record.output == k_BackgroundLabel)
      {
        continue;
      }
      int64 x = 0;
      int64 y = 0;
      int64 z = 0;
      decode(flatIndex, x, y, z);
      bool hasBackgroundNeighbor = false;
      for(const Neighbor& offset : neighbors)
      {
        const int64 neighborX = x + offset.dx;
        const int64 neighborY = y + offset.dy;
        const int64 neighborZ = z + offset.dz;
        if(neighborX < 0 || neighborX >= dimX || neighborY < 0 || neighborY >= dimY || neighborZ < 0 || neighborZ >= dimZ)
        {
          continue;
        }
        auto neighborResult = readVoxel(flatIndex + offset.flat_delta);
        if(neighborResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(neighborResult));
        }
        if(neighborResult.value().output == k_BackgroundLabel)
        {
          hasBackgroundNeighbor = true;
          break;
        }
      }
      if(hasBackgroundNeighbor)
      {
        auto grayValueResult = readGrayValue(flatIndex, record);
        if(grayValueResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(grayValueResult));
        }
        if(Result<> result = queue->push(grayValueResult.value(), flatIndex); result.invalid())
        {
          return result;
        }
      }
    }

    progressThrottle.updateCount(valueCount);
    messageHandler.sendInfoMessage("Flooding Watershed Regions");
    progressThrottle.reset(0, "Flooding Watershed Regions");
    uint64 popCount = 0;
    while(!queue->empty())
    {
      if(((++popCount & 0xFFFFu) == 0) && shouldCancel)
      {
        return {};
      }
      if((popCount & 0xFFFFu) == 0)
      {
        progressThrottle.queueMessage("Flooding Watershed Regions: {} queue entries processed", popCount - 1);
      }
      auto queuedVoxelResult = queue->pop();
      if(queuedVoxelResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(queuedVoxelResult));
      }
      const auto queueRecord = queuedVoxelResult.value();
      const int64 flatIndex = queueRecord.flat_index;
      auto currentResult = readVoxel(flatIndex);
      if(currentResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(currentResult));
      }
      const uint32 currentMarker = currentResult.value().output;
      int64 x = 0;
      int64 y = 0;
      int64 z = 0;
      decode(flatIndex, x, y, z);
      for(const Neighbor& offset : neighbors)
      {
        const int64 neighborX = x + offset.dx;
        const int64 neighborY = y + offset.dy;
        const int64 neighborZ = z + offset.dz;
        if(neighborX < 0 || neighborX >= dimX || neighborY < 0 || neighborY >= dimY || neighborZ < 0 || neighborZ >= dimZ)
        {
          continue;
        }
        const int64 neighborFlat = flatIndex + offset.flat_delta;
        auto neighborResult = readVoxel(neighborFlat);
        if(neighborResult.invalid())
        {
          return ConvertInvalidResult<void>(std::move(neighborResult));
        }
        VoxelRecordType neighbor = neighborResult.value();
        if(neighbor.output == k_WatershedLabel)
        {
          neighbor.output = currentMarker;
          if(Result<> result = writeVoxel(neighborFlat, neighbor); result.invalid())
          {
            return result;
          }
          auto grayValueResult = readGrayValue(neighborFlat, neighbor);
          if(grayValueResult.invalid())
          {
            return ConvertInvalidResult<void>(std::move(grayValueResult));
          }
          if(Result<> result = queue->push(detail::WatershedEffectiveLevel(grayValueResult.value(), queueRecord.level), neighborFlat); result.invalid())
          {
            return result;
          }
        }
      }
    }
  }

  if(Result<> result = queue->flush(); result.invalid())
  {
    return result;
  }
  messageHandler.sendInfoMessage("Writing Watershed Labels");
  progressThrottle.reset(valueCount, "Writing Watershed Labels");
  if(Result<> result = voxels.flush(shouldCancel); result.invalid())
  {
    return MakeErrorResult(-79060, fmt::format("External watershed voxel-cache flush failed: {}", describeStoreError(result)));
  }

  if(plan.use_tiled_voxel_layout)
  {
    std::vector<PhysicalVoxelRecordType> recordBuffer(plan.transfer_records);
    std::vector<uint32> outputBuffer(plan.transfer_records);
    const usize tileX = tiledLayout.tile_dimensions[0];
    const usize tileY = tiledLayout.tile_dimensions[1];
    const usize tileZ = tiledLayout.tile_dimensions[2];
    for(usize tileZIndex = 0; tileZIndex < tiledLayout.tile_counts[2]; ++tileZIndex)
    {
      const usize zStart = tileZIndex * tileZ;
      const usize validZ = std::min(tileZ, dims[2] - zStart);
      for(usize tileYIndex = 0; tileYIndex < tiledLayout.tile_counts[1]; ++tileYIndex)
      {
        if(shouldCancel)
        {
          return {};
        }
        const usize yStart = tileYIndex * tileY;
        const usize validY = std::min(tileY, dims[1] - yStart);
        const usize bandRows = validZ * validY;
        const usize maximumGroupWidth = std::max(tileX, plan.transfer_records / bandRows);
        const usize maximumGroupTiles = std::max<usize>(1, maximumGroupWidth / tileX);
        for(usize firstTileX = 0; firstTileX < tiledLayout.tile_counts[0]; firstTileX += maximumGroupTiles)
        {
          const usize groupTileCount = std::min(maximumGroupTiles, tiledLayout.tile_counts[0] - firstTileX);
          const usize xStart = firstTileX * tileX;
          const usize groupWidth = std::min(groupTileCount * tileX, dims[0] - xStart);
          const usize bandValueCount = bandRows * groupWidth;
          std::fill_n(outputBuffer.begin(), bandValueCount, uint32{0});
          for(usize groupTileX = 0; groupTileX < groupTileCount; ++groupTileX)
          {
            const usize tileIndex = (tileZIndex * tiledLayout.tile_counts[1] + tileYIndex) * tiledLayout.tile_counts[0] + firstTileX + groupTileX;
            nonstd::span<PhysicalVoxelRecordType> records(recordBuffer.data(), plan.voxel_records_per_page);
            nonstd::span<std::byte> bytes(reinterpret_cast<std::byte*>(records.data()), records.size() * sizeof(PhysicalVoxelRecordType));
            auto readResult = voxelRecordStore->read(tileIndex * plan.voxel_records_per_page, plan.voxel_records_per_page, bytes, shouldCancel);
            if(readResult.invalid())
            {
              return MakeErrorResult(-79061, fmt::format("External watershed final tiled voxel read for tile {} failed: {}", tileIndex, describeStoreError(readResult)));
            }
            if(readResult.value() != plan.voxel_records_per_page)
            {
              return MakeErrorResult(-79061, fmt::format("External watershed final tiled voxel read for tile {} returned {} of {} records.", tileIndex, readResult.value(), plan.voxel_records_per_page));
            }
            const usize tileStartInGroup = groupTileX * tileX;
            const usize validX = std::min(tileX, groupWidth - tileStartInGroup);
            for(usize localZ = 0; localZ < validZ; ++localZ)
            {
              for(usize localY = 0; localY < validY; ++localY)
              {
                for(usize localX = 0; localX < validX; ++localX)
                {
                  const usize localIndex = (localZ * tileY + localY) * tileX + localX;
                  const usize bandOffset = (localZ * validY + localY) * groupWidth + tileStartInGroup + localX;
                  outputBuffer[bandOffset] = decodePhysicalVoxel(recordBuffer[localIndex]).output;
                }
              }
            }
          }
          for(usize localZ = 0; localZ < validZ; ++localZ)
          {
            for(usize localY = 0; localY < validY; ++localY)
            {
              const usize logicalStart = ((zStart + localZ) * dims[1] + yStart + localY) * dims[0] + xStart;
              const usize bandOffset = (localZ * validY + localY) * groupWidth;
              if(Result<> result = outStore.copyFromBuffer(logicalStart, nonstd::span<const uint32>(outputBuffer.data() + bandOffset, groupWidth)); result.invalid())
              {
                return result;
              }
            }
          }

          progressThrottle.incrementCount(validZ * validY * groupWidth);
        }
      }
    }
  }
  else
  {
    std::vector<PhysicalVoxelRecordType> recordBuffer(plan.transfer_records);
    std::vector<uint32> outputBuffer(plan.transfer_records);
    for(usize start = 0; start < valueCount; start += plan.transfer_records)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(plan.transfer_records, valueCount - start);
      nonstd::span<PhysicalVoxelRecordType> records(recordBuffer.data(), count);
      nonstd::span<std::byte> bytes(reinterpret_cast<std::byte*>(records.data()), records.size() * sizeof(PhysicalVoxelRecordType));
      auto readResult = voxelRecordStore->read(start, count, bytes, shouldCancel);
      if(readResult.invalid())
      {
        return MakeErrorResult(-79061, fmt::format("External watershed final voxel read at offset {} failed: {}", start, describeStoreError(readResult)));
      }
      if(readResult.value() != count)
      {
        return MakeErrorResult(-79061, fmt::format("External watershed final voxel read at offset {} returned {} of {} records.", start, readResult.value(), count));
      }
      for(usize index = 0; index < count; ++index)
      {
        outputBuffer[index] = decodePhysicalVoxel(recordBuffer[index]).output;
      }
      if(Result<> result = outStore.copyFromBuffer(start, nonstd::span<const uint32>(outputBuffer.data(), count)); result.invalid())
      {
        return result;
      }

      progressThrottle.updateCount(start + count);
    }
  }
  return {};
}

template <class TInput>
Result<> ApplyWatershedFromMarkersExternal(const AbstractDataStore<TInput>& inStore, const AbstractDataStore<uint32>& markerStore, AbstractDataStore<uint32>& outStore, const SizeVec3Type& dims,
                                           bool markWatershedLine, bool fullyConnected, uint32 borderSentinel, const std::atomic_bool& shouldCancel,
                                           std::unique_ptr<ITemporaryRecordStore> voxelRecordStore, std::unique_ptr<ITemporaryRecordStore> queueRecordStore,
                                           const detail::WatershedExternalMemoryPlan<TInput>& plan, const IFilter::MessageHandler& messageHandler = {})
{
  if constexpr(detail::k_UseWatershedBucketQueue<TInput>)
  {
    if(plan.use_split_bucket_state)
    {
      return ApplyWatershedFromMarkersExternalImpl<TInput, true, false, false>(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel,
                                                                               std::move(voxelRecordStore), std::move(queueRecordStore), plan, messageHandler);
    }
    if(plan.use_packed_resident_status_state)
    {
      return ApplyWatershedFromMarkersExternalImpl<TInput, false, false, true>(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel,
                                                                               std::move(voxelRecordStore), std::move(queueRecordStore), plan, messageHandler);
    }
    if(plan.use_packed_combined_state)
    {
      return ApplyWatershedFromMarkersExternalImpl<TInput, false, true, false>(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel,
                                                                               std::move(voxelRecordStore), std::move(queueRecordStore), plan, messageHandler);
    }
  }
  return ApplyWatershedFromMarkersExternalImpl<TInput, false, false, false>(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel,
                                                                            std::move(voxelRecordStore), std::move(queueRecordStore), plan, messageHandler);
}

template <class TInput>
Result<> ApplyWatershedFromMarkersExternal(const AbstractDataStore<TInput>& inStore, const AbstractDataStore<uint32>& markerStore, AbstractDataStore<uint32>& outStore, const SizeVec3Type& dims,
                                           bool markWatershedLine, bool fullyConnected, uint32 borderSentinel, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler = {})
{
  auto allocationResult = detail::ReserveWatershedExternalMemoryPlan<TInput>(inStore.getSize(), detail::k_WatershedPreferredUsefulNumerator, detail::k_WatershedPreferredUsefulDenominator,
                                                                             detail::k_WatershedExternalTargetBytes, markWatershedLine);
  if(allocationResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(allocationResult));
  }
  auto allocation = std::move(allocationResult.value());
  const auto& plan = allocation.plan;
  TemporaryRecordStoreConfig voxelConfig;
  voxelConfig.record_size = plan.voxel_record_bytes;
  voxelConfig.max_records_per_batch = std::max(plan.voxel_records_per_page, plan.transfer_records);
  voxelConfig.initial_record_count = inStore.getSize();
  if(plan.use_tiled_voxel_layout)
  {
    auto layoutResult = detail::CreateWatershedTiledRecordLayout(dims, plan.voxel_records_per_page);
    if(layoutResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(layoutResult));
    }
    voxelConfig.initial_record_count = layoutResult.value().record_count;
  }
  TemporaryRecordStoreConfig queueConfig;
  queueConfig.record_size = plan.use_bucket_queue ? sizeof(detail::WatershedQueueBlock) : sizeof(detail::WatershedHeapRecord<TInput>);
  queueConfig.max_records_per_batch = plan.use_bucket_queue ? plan.bucket_blocks_per_page : plan.heap_records_per_page;
  queueConfig.initial_record_count = plan.use_bucket_queue ? plan.bucket_block_count : inStore.getSize();
  try
  {
    auto voxelStoreResult = data_store_utilities::CreateTemporaryRecordStore(voxelConfig);
    if(voxelStoreResult.invalid())
    {
      return MakeErrorResult(-79062, fmt::format("External watershed failed to create its voxel-record store: {}", detail::DescribeWatershedStoreError(voxelStoreResult)));
    }
    auto queueStoreResult = data_store_utilities::CreateTemporaryRecordStore(queueConfig);
    if(queueStoreResult.invalid())
    {
      return MakeErrorResult(-79063, fmt::format("External watershed failed to create its {} queue store with {} records of {} bytes: {}", plan.use_bucket_queue ? "bucket-block" : "min-heap",
                                                 queueConfig.initial_record_count, queueConfig.record_size, detail::DescribeWatershedStoreError(queueStoreResult)));
    }
    return ApplyWatershedFromMarkersExternal<TInput>(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel, std::move(voxelStoreResult.value()),
                                                     std::move(queueStoreResult.value()), plan, messageHandler);
  } catch(const std::exception& exception)
  {
    return MakeErrorResult(-79062, fmt::format("External watershed temporary-record setup failed: {}", exception.what()));
  }
}

// ITK-free port of MorphologicalWatershedFromMarkersImageFilter: a hierarchical-queue (FAH) marker-controlled
// watershed flood, transcribed verbatim from itkMorphologicalWatershedFromMarkersImageFilter.hxx::GenerateData
// (both Meyer/watershed-line and Beucher variants). Labels are flooded in uint32; `borderSentinel` reproduces ITK's
// NumericTraits<TLabel>::max() marker-image boundary constant (pass static_cast<uint32>(markerType_max)) so uint32
// flooding is bit-identical to ITK's native-TLabel flood including the label==type-max collision. `TInput` (the FAH
// key = gray level) is the only template parameter. Resident endpoints use the original contiguous-RAM map-of-FIFOs
// implementation below. An OOC 8/16-bit integral endpoint uses that contiguous implementation only when a conservative
// full-state reservation fits the shared cache budget; a partial grant, allocation failure, or wider/floating input
// uses the external implementation above. External gray/output/status state is held in a fixed-record store behind a
// bounded page cache. The marker image is streamed only during initialization. For 8/16-bit levels, a separate bounded
// queue-block cache stores 64-bit flat indexes in exact bucket/FIFO order. Wider/floating levels use an exact
// (level, insertion-sequence) external min-heap. Both queue forms preserve the resident gray-level/FIFO order.
template <class TInput>
Result<> ApplyWatershedFromMarkersResident(const AbstractDataStore<TInput>& inStore, const AbstractDataStore<uint32>& markerStore, AbstractDataStore<uint32>& outStore, const SizeVec3Type& dims,
                                           bool markWatershedLine, bool fullyConnected, uint32 borderSentinel, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  messageHandler.sendInfoMessage("Computing Watershed From Markers");
  ThrottledMessageHandler progressThrottle(messageHandler);

  // the label used to find background in the marker image, and to mark the watershed line in the output image
  static constexpr uint32 k_BgLabel = 0u;
  static constexpr uint32 k_WsLabel = 0u;

  // Precondition: borderSentinel must be non-zero (it is NumericTraits<TLabel>::max() by contract). The Beucher branch
  // maps out-of-bounds neighbors to borderSentinel and gates propagation on `o == wsLabel(0)`, so a non-zero sentinel
  // guarantees an OOB neighbor can never alias a real label (nor flat index 0). All type maxes are > 0, so this holds.
  assert(borderSentinel != k_WsLabel);

  const int64 nX = static_cast<int64>(dims[0]);
  const int64 nY = static_cast<int64>(dims[1]);
  const int64 nZ = static_cast<int64>(dims[2]);
  if(nX <= 0 || nY <= 0 || nZ <= 0)
  {
    return {};
  }
  const int64 vol = nX * nY * nZ;
  const usize N = static_cast<usize>(vol);

  // Connectivity offset set, in ITK's shaped-neighborhood-iterator activation (ascending neighborhood-index) order.
  const int effDim = (nZ > 1) ? 3 : 2;
  const std::vector<detail::WsNeighborOffset> offsets = detail::BuildWatershedOffsets(effDim, fullyConnected);

  // Precompute the neighbor flat-index delta once per offset: for a voxel at flat f the in-bounds neighbor
  // (x+dx,y+dy,z+dz) is at flat f + flatDelta, with flatDelta = (dz*nY + dy)*nX + dx. This turns each neighbor access
  // in the flood into a direct buf[f + flatDelta] index (the ITK-speed form), keeping the (dx,dy,dz) around only for
  // the per-neighbor bounds test.
  struct WsNeighbor
  {
    int64 dx;
    int64 dy;
    int64 dz;
    int64 flat_delta;
  };
  std::vector<WsNeighbor> neighbors;
  neighbors.reserve(offsets.size());
  for(const detail::WsNeighborOffset& off : offsets)
  {
    neighbors.push_back(WsNeighbor{off.dx, off.dy, off.dz, (off.dz * nY + off.dy) * nX + off.dx});
  }

  // Contiguous RAM working buffers (loaded from / written back to the stores via sequential bulk I/O). The flood is a
  // global gray-level-order random walk, so these must be fully resident; guard the allocation with a bad_alloc backstop
  // (belt-and-suspenders with the caller's preflight RAM-fit guard). markerBuf is freed after the init pass.
  std::vector<TInput> inBuf;
  std::vector<uint32> markerBuf;
  std::vector<uint8> status; // Meyer only (processed / already-in-queue); Beucher uses output==wsLabel instead
  std::vector<uint32> outBuf;
  try
  {
    inBuf.resize(N);
    markerBuf.resize(N);
    outBuf.assign(N, uint32{0}); // wsLabel = 0; the init pass overwrites marker pixels + re-confirms non-markers
    if(markWatershedLine)
    {
      status.assign(N, uint8{0}); // ITK's statusImage->FillBuffer(false)
    }
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(
        detail::k_WatershedResidentOutOfMemory,
        fmt::format("Morphological watershed could not allocate resident working buffers for {} voxels. Input element bytes: {}; watershed line: {}.", N, sizeof(TInput), markWatershedLine));
  }

  // Sequentially load inStore -> inBuf and markerStore -> markerBuf in 65536-element chunks (near-zero overhead in-core;
  // coalesced chunk reads OOC). Honor cancellation per chunk and propagate any read error.
  {
    constexpr usize k_Chunk = 65536;
    progressThrottle.reset(N, "Loading Watershed Inputs");
    for(usize s = 0; s < N; s += k_Chunk)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize n = std::min(k_Chunk, N - s);
      if(Result<> r = inStore.copyIntoBuffer(s, nonstd::span<TInput>(inBuf.data() + s, n)); r.invalid())
      {
        return r;
      }
      if(Result<> r = markerStore.copyIntoBuffer(s, nonstd::span<uint32>(markerBuf.data() + s, n)); r.invalid())
      {
        return r;
      }
      progressThrottle.updateCount(s + n);
    }
  }

  // Decode a flat index -> (x,y,z). flat = ((z*nY)+y)*nX + x.
  auto decode = [nX, nY](int64 flat, int64& x, int64& y, int64& z) {
    x = flat % nX;
    const int64 rem = flat / nX;
    y = rem % nY;
    z = rem / nY;
  };

  // FAH (File d'Attente Hierarchique): map keyed by gray level ascending, FIFO within a level. Values are flat indices.
  using QueueType = std::queue<int64>;
  using MapType = std::map<TInput, QueueType>;
  MapType fah;

  messageHandler.sendInfoMessage("Initializing Watershed Regions");
  if(markWatershedLine)
  {
    //-------------------------------------------------------------------------
    // Meyer's algorithm (with watershed lines)
    //-------------------------------------------------------------------------
    // ---- init stage: mark markers processed, copy markers to output, seed the FAH with background neighbors ----
    progressThrottle.reset(static_cast<usize>(nZ * nY), "Initializing Watershed Rows");
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      for(int64 y = 0; y < nY; ++y)
      {
        for(int64 x = 0; x < nX; ++x)
        {
          const int64 flat = (z * nY + y) * nX + x;
          const uint32 markerPixel = markerBuf[static_cast<usize>(flat)];
          if(markerPixel != k_BgLabel)
          {
            // this pixel belongs to a marker: mark it processed and copy it to the output
            status[static_cast<usize>(flat)] = uint8{1};
            outBuf[static_cast<usize>(flat)] = markerPixel;
            // search the background pixels in the neighborhood
            for(const WsNeighbor& off : neighbors)
            {
              const int64 nx = x + off.dx;
              const int64 ny = y + off.dy;
              const int64 nz = z + off.dz;
              const bool inBounds = (nx >= 0 && nx < nX && ny >= 0 && ny < nY && nz >= 0 && nz < nZ);
              const int64 nFlat = flat + off.flat_delta; // valid only when inBounds
              // status boundary: OOB -> already processed(true); marker boundary: OOB -> borderSentinel.
              const bool nbrStatus = inBounds ? (status[static_cast<usize>(nFlat)] != 0) : true;
              const uint32 nbrMarker = inBounds ? markerBuf[static_cast<usize>(nFlat)] : borderSentinel;
              if(!nbrStatus && nbrMarker == k_BgLabel)
              {
                // background pixel not yet processed: add it to the FAH at its input gray level; mark it queued
                fah[inBuf[static_cast<usize>(nFlat)]].push(nFlat);
                status[static_cast<usize>(nFlat)] = uint8{1};
              }
            }
          }
          else
          {
            // non-marked pixels default to the watershed line (some are never processed)
            outBuf[static_cast<usize>(flat)] = k_WsLabel;
          }
        }
        progressThrottle.updateCount(static_cast<usize>(z * nY + y + 1));
      }
    }

    // The marker image is never read again (neither flood branch touches it) -- free it now to leave RAM headroom for
    // the FAH front, which grows during the flood.
    std::vector<uint32>().swap(markerBuf);

    // ---- flooding stage ----
    messageHandler.sendInfoMessage("Flooding Watershed Regions");
    progressThrottle.reset(0, "Flooding Watershed Regions");
    uint64 popCount = 0;
    while(!fah.empty())
    {
      if(shouldCancel)
      {
        return {};
      }
      const TInput currentValue = fah.begin()->first;
      QueueType currentQueue = std::move(fah.begin()->second); // moved (the entry is erased next) -- byte-identical order
      fah.erase(fah.begin());

      while(!currentQueue.empty())
      {
        // Throttled cancel: a flat/single-level volume floods entirely within one FAH level, so the outer-loop check
        // above would fire only once. Poll every 65536 pops (negligible on the hot path).
        if(((++popCount & 0xFFFFu) == 0) && shouldCancel)
        {
          return {};
        }
        if((popCount & 0xFFFFu) == 0)
        {
          progressThrottle.queueMessage("Flooding Watershed Regions: {} queue entries processed", popCount - 1);
        }
        const int64 flat = currentQueue.front();
        currentQueue.pop();
        int64 x = 0;
        int64 y = 0;
        int64 z = 0;
        decode(flat, x, y, z);

        // If there is only one marker value among the neighbors, give that value to the pixel; else keep it as the
        // watershed line (collision). Output boundary (Meyer): OOB -> wsLabel.
        uint32 marker = k_WsLabel;
        bool collision = false;
        for(const WsNeighbor& off : neighbors)
        {
          const int64 nx = x + off.dx;
          const int64 ny = y + off.dy;
          const int64 nz = z + off.dz;
          const bool inBounds = (nx >= 0 && nx < nX && ny >= 0 && ny < nY && nz >= 0 && nz < nZ);
          const uint32 o = inBounds ? outBuf[static_cast<usize>(flat + off.flat_delta)] : k_WsLabel;
          if(o != k_WsLabel)
          {
            if(marker != k_WsLabel && o != marker)
            {
              collision = true;
              break;
            }
            marker = o;
          }
        }
        if(!collision)
        {
          outBuf[static_cast<usize>(flat)] = marker;
          // propagate to the not-yet-processed neighbors
          for(const WsNeighbor& off : neighbors)
          {
            const int64 nx = x + off.dx;
            const int64 ny = y + off.dy;
            const int64 nz = z + off.dz;
            const bool inBounds = (nx >= 0 && nx < nX && ny >= 0 && ny < nY && nz >= 0 && nz < nZ);
            if(!inBounds)
            {
              continue; // status boundary: OOB -> processed(true) -> never pushed
            }
            const int64 nFlat = flat + off.flat_delta;
            if(status[static_cast<usize>(nFlat)] == 0)
            {
              const TInput grayVal = inBuf[static_cast<usize>(nFlat)];
              if(grayVal <= currentValue)
              {
                currentQueue.push(nFlat);
              }
              else
              {
                fah[grayVal].push(nFlat);
              }
              status[static_cast<usize>(nFlat)] = uint8{1};
            }
          }
        }
      }
    }
  }
  else
  {
    //-------------------------------------------------------------------------
    // Beucher's algorithm (no watershed lines). No status image: output==wsLabel is the "not yet processed" test.
    //-------------------------------------------------------------------------
    // ---- init stage: copy markers to output, seed the FAH with marker pixels that have a background neighbor ----
    progressThrottle.reset(static_cast<usize>(nZ * nY), "Initializing Watershed Rows");
    for(int64 z = 0; z < nZ; ++z)
    {
      if(shouldCancel)
      {
        return {};
      }
      for(int64 y = 0; y < nY; ++y)
      {
        for(int64 x = 0; x < nX; ++x)
        {
          const int64 flat = (z * nY + y) * nX + x;
          const uint32 markerPixel = markerBuf[static_cast<usize>(flat)];
          if(markerPixel != k_BgLabel)
          {
            outBuf[static_cast<usize>(flat)] = markerPixel;
            bool haveBgNeighbor = false;
            for(const WsNeighbor& off : neighbors)
            {
              const int64 nx = x + off.dx;
              const int64 ny = y + off.dy;
              const int64 nz = z + off.dz;
              const bool inBounds = (nx >= 0 && nx < nX && ny >= 0 && ny < nY && nz >= 0 && nz < nZ);
              // marker boundary: OOB -> borderSentinel (never == bgLabel).
              const uint32 nbrMarker = inBounds ? markerBuf[static_cast<usize>(flat + off.flat_delta)] : borderSentinel;
              if(nbrMarker == k_BgLabel)
              {
                haveBgNeighbor = true;
                break;
              }
            }
            if(haveBgNeighbor)
            {
              // seed with the marker pixel itself, at its own input gray level
              fah[inBuf[static_cast<usize>(flat)]].push(flat);
            }
          }
          else
          {
            outBuf[static_cast<usize>(flat)] = k_WsLabel;
          }
        }
        progressThrottle.updateCount(static_cast<usize>(z * nY + y + 1));
      }
    }

    // The marker image is never read again (the flood only reads output/input) -- free it now to leave RAM headroom for
    // the FAH front, which grows during the flood.
    std::vector<uint32>().swap(markerBuf);

    // ---- flooding stage ----
    messageHandler.sendInfoMessage("Flooding Watershed Regions");
    progressThrottle.reset(0, "Flooding Watershed Regions");
    uint64 popCount = 0;
    while(!fah.empty())
    {
      if(shouldCancel)
      {
        return {};
      }
      const TInput currentValue = fah.begin()->first;
      QueueType currentQueue = std::move(fah.begin()->second); // moved (the entry is erased next) -- byte-identical order
      fah.erase(fah.begin());

      while(!currentQueue.empty())
      {
        // Throttled cancel: a flat/single-level volume floods entirely within one FAH level, so the outer-loop check
        // above would fire only once. Poll every 65536 pops (negligible on the hot path).
        if(((++popCount & 0xFFFFu) == 0) && shouldCancel)
        {
          return {};
        }
        if((popCount & 0xFFFFu) == 0)
        {
          progressThrottle.queueMessage("Flooding Watershed Regions: {} queue entries processed", popCount - 1);
        }
        const int64 flat = currentQueue.front();
        currentQueue.pop();
        int64 x = 0;
        int64 y = 0;
        int64 z = 0;
        decode(flat, x, y, z);

        const uint32 currentMarker = outBuf[static_cast<usize>(flat)];
        // propagate the current label to each not-yet-labeled neighbor. Output boundary (Beucher): OOB -> borderSentinel.
        for(const WsNeighbor& off : neighbors)
        {
          const int64 nx = x + off.dx;
          const int64 ny = y + off.dy;
          const int64 nz = z + off.dz;
          const bool inBounds = (nx >= 0 && nx < nX && ny >= 0 && ny < nY && nz >= 0 && nz < nZ);
          const int64 nFlat = flat + off.flat_delta; // valid only when inBounds
          const uint32 o = inBounds ? outBuf[static_cast<usize>(nFlat)] : borderSentinel;
          if(o == k_WsLabel)
          {
            // not yet processed: label it with the current marker and add it to the FAH
            outBuf[static_cast<usize>(nFlat)] = currentMarker;
            const TInput grayVal = inBuf[static_cast<usize>(nFlat)];
            if(grayVal <= currentValue)
            {
              currentQueue.push(nFlat);
            }
            else
            {
              fah[grayVal].push(nFlat);
            }
          }
        }
      }
    }
  }

  // Write the flooded labels back to the output store, sequentially, in 65536-element chunks (near-zero overhead
  // in-core; coalesced chunk writes OOC).
  {
    constexpr usize k_Chunk = 65536;
    messageHandler.sendInfoMessage("Writing Watershed Labels");
    progressThrottle.reset(N, "Writing Watershed Labels");
    for(usize s = 0; s < N; s += k_Chunk)
    {
      const usize n = std::min(k_Chunk, N - s);
      if(Result<> r = outStore.copyFromBuffer(s, nonstd::span<const uint32>(outBuf.data() + s, n)); r.invalid())
      {
        return r;
      }
      progressThrottle.updateCount(s + n);
    }
  }

  return {};
}

template <class TInput>
Result<> ApplyWatershedFromMarkers(const AbstractDataStore<TInput>& inStore, const AbstractDataStore<uint32>& markerStore, AbstractDataStore<uint32>& outStore, const SizeVec3Type& dims,
                                   bool markWatershedLine, bool fullyConnected, uint32 borderSentinel, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  const bool usesOutOfCoreStore =
      inStore.getStoreType() == IDataStore::StoreType::OutOfCore || markerStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
  if(!usesOutOfCoreStore)
  {
    return ApplyWatershedFromMarkersResident(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel, messageHandler);
  }

  RecordAlgorithmPathExecution(AlgorithmPath::OutOfCore, /*usesOutOfCoreStore=*/true);
  if constexpr(detail::k_UseWatershedBucketQueue<TInput>)
  {
    if(inStore.getSize() > 0)
    {
      auto allocationResult = detail::ReserveWatershedResidentWorkingMemory<TInput>(inStore.getSize(), markWatershedLine);
      if(allocationResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(allocationResult));
      }
      auto allocation = std::move(allocationResult.value());
      if(allocation.holdsCompleteState())
      {
        try
        {
          Result<> residentResult = ApplyWatershedFromMarkersResident(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel, messageHandler);
          const bool allocationFailed = residentResult.invalid() && !residentResult.errors().empty() && residentResult.errors().front().code == detail::k_WatershedResidentOutOfMemory;
          if(!allocationFailed)
          {
            return residentResult;
          }
        } catch(const std::bad_alloc&)
        {
          // Release the reservation; the external fallback overwrites the complete output even if a store allocated late.
        }
      }
    }
  }
  return ApplyWatershedFromMarkersExternal(inStore, markerStore, outStore, dims, markWatershedLine, fullyConnected, borderSentinel, shouldCancel, messageHandler);
}
} // namespace nx::core::ImageProcessing
