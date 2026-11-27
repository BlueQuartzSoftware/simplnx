#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing::detail
{
inline constexpr usize k_WatershedExternalTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_WatershedTransferTargetBytes = 8ULL * 1024ULL * 1024ULL;
inline constexpr usize k_WatershedPageTargetBytes = 64ULL * 1024ULL;
inline constexpr usize k_WatershedVoxelCacheWeight = 7;
inline constexpr usize k_WatershedHeapCacheWeight = 5;
inline constexpr usize k_WatershedBucketCacheWeight = 1;
inline constexpr usize k_WatershedInputCacheWeight = 2;
inline constexpr uint32 k_WatershedPreferredUsefulNumerator = 1;
inline constexpr uint32 k_WatershedPreferredUsefulDenominator = 1;
// Conservative cross-platform allowances for std::map/std::queue nodes, deque blocks, allocator metadata, and bucket
// objects used by the existing contiguous resident flood. They reserve cache headroom; they are not allocated buffers.
inline constexpr usize k_WatershedResidentQueueBytesPerValue = 32;
inline constexpr usize k_WatershedResidentBucketHeadroomBytes = 256;

template <class TInput>
struct WatershedVoxelRecord
{
  uint32 output = 0;
  uint8 status = 0;
  TInput gray_value = {};
};

template <class TInput>
using WatershedPackedVoxelRecordType = std::array<std::byte, sizeof(uint32) + sizeof(uint8) + sizeof(TInput)>;

static_assert(sizeof(WatershedPackedVoxelRecordType<uint8>) == 6);
static_assert(sizeof(WatershedPackedVoxelRecordType<uint16>) == 7);

template <class TInput>
using WatershedPackedResidentStatusVoxelRecordType = std::array<std::byte, sizeof(uint32) + sizeof(TInput)>;

static_assert(sizeof(WatershedPackedResidentStatusVoxelRecordType<uint8>) == 5);
static_assert(sizeof(WatershedPackedResidentStatusVoxelRecordType<uint16>) == 6);

template <class TInput>
WatershedPackedVoxelRecordType<TInput> EncodeWatershedPackedVoxelRecord(const WatershedVoxelRecord<TInput>& record) noexcept
{
  static_assert(std::is_trivially_copyable_v<TInput>);
  WatershedPackedVoxelRecordType<TInput> packed = {};
  std::memcpy(packed.data(), &record.output, sizeof(record.output));
  std::memcpy(packed.data() + sizeof(record.output), &record.status, sizeof(record.status));
  std::memcpy(packed.data() + sizeof(record.output) + sizeof(record.status), &record.gray_value, sizeof(record.gray_value));
  return packed;
}

template <class TInput>
WatershedVoxelRecord<TInput> DecodeWatershedPackedVoxelRecord(const WatershedPackedVoxelRecordType<TInput>& packed) noexcept
{
  static_assert(std::is_trivially_copyable_v<TInput>);
  WatershedVoxelRecord<TInput> record;
  std::memcpy(&record.output, packed.data(), sizeof(record.output));
  std::memcpy(&record.status, packed.data() + sizeof(record.output), sizeof(record.status));
  std::memcpy(&record.gray_value, packed.data() + sizeof(record.output) + sizeof(record.status), sizeof(record.gray_value));
  return record;
}

template <class TInput>
WatershedPackedResidentStatusVoxelRecordType<TInput> EncodeWatershedPackedResidentStatusVoxelRecord(const WatershedVoxelRecord<TInput>& record) noexcept
{
  static_assert(std::is_trivially_copyable_v<TInput>);
  WatershedPackedResidentStatusVoxelRecordType<TInput> packed = {};
  std::memcpy(packed.data(), &record.output, sizeof(record.output));
  std::memcpy(packed.data() + sizeof(record.output), &record.gray_value, sizeof(record.gray_value));
  return packed;
}

template <class TInput>
WatershedVoxelRecord<TInput> DecodeWatershedPackedResidentStatusVoxelRecord(const WatershedPackedResidentStatusVoxelRecordType<TInput>& packed) noexcept
{
  static_assert(std::is_trivially_copyable_v<TInput>);
  WatershedVoxelRecord<TInput> record;
  std::memcpy(&record.output, packed.data(), sizeof(record.output));
  std::memcpy(&record.gray_value, packed.data() + sizeof(record.output), sizeof(record.gray_value));
  return record;
}

template <class TInput>
struct WatershedHeapRecord
{
  TInput level = {};
  uint64 sequence = 0;
  int64 flat_index = 0;
};

struct WatershedBucketVoxelRecord
{
  uint32 output = 0;
};

template <class TInput>
struct WatershedExternalMemoryPlan
{
  bool use_bucket_queue = false;
  bool use_split_bucket_state = false;
  bool use_packed_combined_state = false;
  bool use_packed_resident_status_state = false;
  bool complete_primary_state_cache = false;
  bool use_tiled_voxel_layout = false;
  bool use_resident_status = false;
  usize bucket_count = 0;
  usize queue_metadata_bytes = 0;
  usize resident_status_word_count = 0;
  usize resident_status_bytes = 0;
  usize voxel_record_bytes = 0;
  usize voxel_records_per_page = 0;
  usize voxel_cache_pages = 0;
  usize input_records_per_page = 0;
  usize input_cache_pages = 0;
  usize bucket_block_count = 0;
  usize bucket_blocks_per_page = 0;
  usize bucket_cache_pages = 0;
  usize heap_records_per_page = 0;
  usize heap_cache_pages = 0;
  usize transfer_records = 0;
  usize resident_bytes = 0;
};

template <class TInput>
struct WatershedExternalMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  WatershedExternalMemoryPlan<TInput> plan;
};

struct WatershedResidentMemoryAllocation
{
  CacheMemoryBudgetManager::WorkingMemoryReservation reservation;
  usize required_bytes = 0;

  [[nodiscard]] bool holdsCompleteState() const noexcept
  {
    return required_bytes > 0 && reservation.sizeBytes() == required_bytes;
  }
};

struct WatershedBucketEndpoints
{
  uint64 head_block = std::numeric_limits<uint64>::max();
  uint64 tail_block = std::numeric_limits<uint64>::max();
  uint32 head_offset = 0;
};

inline constexpr usize k_WatershedQueueBlockBytes = 4096;
inline constexpr usize k_WatershedQueueBlockValueCount = (k_WatershedQueueBlockBytes - 2 * sizeof(uint64)) / sizeof(int64);
inline constexpr uint64 k_WatershedInvalidQueueBlock = std::numeric_limits<uint64>::max();

struct WatershedQueueBlock
{
  uint64 next_block = k_WatershedInvalidQueueBlock;
  uint64 value_count = 0;
  std::array<int64, k_WatershedQueueBlockValueCount> values = {};
};

static_assert(sizeof(WatershedQueueBlock) == k_WatershedQueueBlockBytes);

template <class TInput>
inline constexpr bool k_UseWatershedBucketQueue = std::is_integral_v<TInput> && sizeof(TInput) <= sizeof(uint16);

inline bool TryMultiplyWatershedSize(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool TryAddWatershedSize(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline bool TryCalculateWatershedBucketBlockCount(usize valueCount, usize bucketCount, usize& blockCount)
{
  if(valueCount == 0 || bucketCount == 0)
  {
    return false;
  }
  const usize packedBlockCount = valueCount / k_WatershedQueueBlockValueCount + static_cast<usize>(valueCount % k_WatershedQueueBlockValueCount != 0);
  const usize additionalPartialBlocks = std::min(valueCount, bucketCount) - 1;
  return TryAddWatershedSize(packedBlockCount, additionalPartialBlocks, blockCount);
}

inline void GrowWatershedCachePages(usize pageBytes, usize availablePages, usize& cachedPages, usize& remainingBytes)
{
  if(pageBytes == 0 || cachedPages >= availablePages)
  {
    return;
  }
  const usize addedPages = std::min(availablePages - cachedPages, remainingBytes / pageBytes);
  cachedPages += addedPages;
  remainingBytes -= addedPages * pageBytes;
}

struct WatershedTiledRecordLayout
{
  SizeVec3Type dimensions = {0, 0, 0};
  SizeVec3Type tile_dimensions = {0, 0, 0};
  SizeVec3Type tile_counts = {0, 0, 0};
  usize logical_record_count = 0;
  usize record_count = 0;
  usize records_per_page = 0;

  usize physicalIndexUnchecked(usize logicalIndex) const noexcept
  {
    const usize x = logicalIndex % dimensions[0];
    const usize yz = logicalIndex / dimensions[0];
    const usize y = yz % dimensions[1];
    const usize z = yz / dimensions[1];
    const usize tileX = x / tile_dimensions[0];
    const usize tileY = y / tile_dimensions[1];
    const usize tileZ = z / tile_dimensions[2];
    const usize localX = x % tile_dimensions[0];
    const usize localY = y % tile_dimensions[1];
    const usize localZ = z % tile_dimensions[2];
    const usize tileIndex = (tileZ * tile_counts[1] + tileY) * tile_counts[0] + tileX;
    const usize localIndex = (localZ * tile_dimensions[1] + localY) * tile_dimensions[0] + localX;
    return tileIndex * records_per_page + localIndex;
  }

  Result<usize> physicalIndex(usize logicalIndex) const
  {
    if(logicalIndex >= logical_record_count || dimensions[0] == 0 || dimensions[1] == 0 || tile_dimensions[0] == 0 || tile_dimensions[1] == 0 || tile_dimensions[2] == 0)
    {
      return MakeErrorResult<usize>(-79072, fmt::format("External watershed tiled layout rejected logical index {}. Logical records: {}; dimensions: {} x {} x {}.", logicalIndex, logical_record_count,
                                                        dimensions[0], dimensions[1], dimensions[2]));
    }
    const usize physical = physicalIndexUnchecked(logicalIndex);
    if(physical >= record_count)
    {
      return MakeErrorResult<usize>(-79072,
                                    fmt::format("External watershed tiled layout mapped logical index {} to invalid physical index {}. Physical records: {}.", logicalIndex, physical, record_count));
    }
    return {physical};
  }
};

inline Result<WatershedTiledRecordLayout> CreateWatershedTiledRecordLayout(const SizeVec3Type& dimensions, usize recordsPerPage)
{
  if(dimensions[0] == 0 || dimensions[1] == 0 || dimensions[2] == 0 || recordsPerPage == 0)
  {
    return MakeErrorResult<WatershedTiledRecordLayout>(-79072,
                                                       fmt::format("External watershed tiled layout requires nonzero dimensions and records per page. Dimensions: {} x {} x {}; records per page: {}.",
                                                                   dimensions[0], dimensions[1], dimensions[2], recordsPerPage));
  }

  WatershedTiledRecordLayout layout;
  layout.dimensions = dimensions;
  layout.records_per_page = recordsPerPage;
  if(dimensions[2] > 1)
  {
    constexpr usize k_TileX = 32;
    constexpr usize k_TileY = 32;
    if(recordsPerPage % (k_TileX * k_TileY) != 0)
    {
      return MakeErrorResult<WatershedTiledRecordLayout>(
          -79072, fmt::format("External watershed 3-D tiled layout requires records per page divisible by {}. Records per page: {}.", k_TileX * k_TileY, recordsPerPage));
    }
    layout.tile_dimensions = {k_TileX, k_TileY, recordsPerPage / (k_TileX * k_TileY)};
  }
  else
  {
    constexpr usize k_TileX = 32;
    if(recordsPerPage % k_TileX != 0)
    {
      return MakeErrorResult<WatershedTiledRecordLayout>(-79072,
                                                         fmt::format("External watershed 2-D tiled layout requires records per page divisible by {}. Records per page: {}.", k_TileX, recordsPerPage));
    }
    layout.tile_dimensions = {k_TileX, recordsPerPage / k_TileX, 1};
  }
  if(layout.tile_dimensions[2] == 0)
  {
    return MakeErrorResult<WatershedTiledRecordLayout>(-79072, fmt::format("External watershed tiled layout has zero tile depth. Records per page: {}.", recordsPerPage));
  }

  layout.tile_counts = {dimensions[0] / layout.tile_dimensions[0] + static_cast<usize>(dimensions[0] % layout.tile_dimensions[0] != 0),
                        dimensions[1] / layout.tile_dimensions[1] + static_cast<usize>(dimensions[1] % layout.tile_dimensions[1] != 0),
                        dimensions[2] / layout.tile_dimensions[2] + static_cast<usize>(dimensions[2] % layout.tile_dimensions[2] != 0)};
  usize logicalSlice = 0;
  usize tileCount = 0;
  if(!TryMultiplyWatershedSize(dimensions[0], dimensions[1], logicalSlice) || !TryMultiplyWatershedSize(logicalSlice, dimensions[2], layout.logical_record_count) ||
     !TryMultiplyWatershedSize(layout.tile_counts[0], layout.tile_counts[1], tileCount) || !TryMultiplyWatershedSize(tileCount, layout.tile_counts[2], tileCount) ||
     !TryMultiplyWatershedSize(tileCount, recordsPerPage, layout.record_count))
  {
    return MakeErrorResult<WatershedTiledRecordLayout>(
        -79072, fmt::format("External watershed tiled layout size overflowed. Dimensions: {} x {} x {}; records per page: {}.", dimensions[0], dimensions[1], dimensions[2], recordsPerPage));
  }
  return {layout};
}

template <class TInput>
Result<usize> CalculateWatershedResidentWorkingMemoryBytes(usize valueCount, bool markWatershedLine)
{
  if constexpr(!k_UseWatershedBucketQueue<TInput>)
  {
    return MakeErrorResult<usize>(-79070, fmt::format("Resident watershed working memory requires an 8-bit or 16-bit integral input type. Input element bytes: {}; integral type: {}.", sizeof(TInput),
                                                      std::is_integral_v<TInput>));
  }
  else
  {
    if(valueCount == 0)
    {
      return MakeErrorResult<usize>(-79070, "Resident watershed working memory requires a nonzero voxel count. Voxel count: 0.");
    }

    usize bytesPerValue = 0;
    if(!TryAddWatershedSize(sizeof(TInput), 2 * sizeof(uint32), bytesPerValue) || (markWatershedLine && !TryAddWatershedSize(bytesPerValue, sizeof(uint8), bytesPerValue)) ||
       !TryAddWatershedSize(bytesPerValue, k_WatershedResidentQueueBytesPerValue, bytesPerValue))
    {
      return MakeErrorResult<usize>(-79070, fmt::format("Resident watershed per-voxel working-memory size overflowed. Voxel count: {}; input element bytes: {}; watershed line: {}.", valueCount,
                                                        sizeof(TInput), markWatershedLine));
    }

    usize valueBytes = 0;
    constexpr usize k_BucketCount = usize{1} << (sizeof(TInput) * 8);
    usize bucketBytes = 0;
    usize requiredBytes = 0;
    if(!TryMultiplyWatershedSize(valueCount, bytesPerValue, valueBytes) || !TryMultiplyWatershedSize(k_BucketCount, k_WatershedResidentBucketHeadroomBytes, bucketBytes) ||
       !TryAddWatershedSize(valueBytes, bucketBytes, requiredBytes))
    {
      return MakeErrorResult<usize>(-79070, fmt::format("Resident watershed working-memory size overflowed. Voxel count: {}; bytes per voxel: {}; bucket count: {}; bucket headroom bytes: {}.",
                                                        valueCount, bytesPerValue, k_BucketCount, k_WatershedResidentBucketHeadroomBytes));
    }
    return {requiredBytes};
  }
}

template <class TInput>
Result<WatershedResidentMemoryAllocation> ReserveWatershedResidentWorkingMemory(usize valueCount, bool markWatershedLine)
{
  auto requiredResult = CalculateWatershedResidentWorkingMemoryBytes<TInput>(valueCount, markWatershedLine);
  if(requiredResult.invalid())
  {
    return ConvertInvalidResult<WatershedResidentMemoryAllocation>(std::move(requiredResult));
  }
  auto reservation = ReserveWorkingMemory(requiredResult.value(), requiredResult.value());
  return {WatershedResidentMemoryAllocation{std::move(reservation), requiredResult.value()}};
}

template <class TInput>
Result<WatershedExternalMemoryPlan<TInput>> CreateWatershedExternalMemoryPlan(usize valueCount, usize targetBytes = k_WatershedExternalTargetBytes, bool markWatershedLine = true)
{
  static_assert(std::is_trivially_copyable_v<TInput>);
  static_assert(std::is_trivially_copyable_v<WatershedVoxelRecord<TInput>>);
  static_assert(std::is_trivially_copyable_v<WatershedPackedVoxelRecordType<TInput>>);
  static_assert(std::is_trivially_copyable_v<WatershedPackedResidentStatusVoxelRecordType<TInput>>);
  static_assert(std::is_trivially_copyable_v<WatershedHeapRecord<TInput>>);
  static_assert(std::is_trivially_copyable_v<WatershedQueueBlock>);
  if(valueCount == 0 || targetBytes == 0)
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(
        -79050, fmt::format("External watershed requires a nonzero voxel count and working-memory target. Voxel count: {}; target bytes: {}.", valueCount, targetBytes));
  }

  WatershedExternalMemoryPlan<TInput> plan;
  plan.use_bucket_queue = k_UseWatershedBucketQueue<TInput>;
  if(markWatershedLine)
  {
    plan.resident_status_word_count = valueCount / 64 + static_cast<usize>(valueCount % 64 != 0);
    if(!TryMultiplyWatershedSize(plan.resident_status_word_count, sizeof(uint64), plan.resident_status_bytes))
    {
      return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(
          -79050, fmt::format("External watershed resident status size overflowed. Voxel count: {}; status words: {}.", valueCount, plan.resident_status_word_count));
    }
  }
  if constexpr(k_UseWatershedBucketQueue<TInput>)
  {
    plan.bucket_count = usize{1} << (sizeof(TInput) * 8);
    plan.bucket_blocks_per_page = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(WatershedQueueBlock));
    if(!TryMultiplyWatershedSize(plan.bucket_count, sizeof(WatershedBucketEndpoints), plan.queue_metadata_bytes) ||
       !TryCalculateWatershedBucketBlockCount(valueCount, plan.bucket_count, plan.bucket_block_count))
    {
      return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050,
                                                                  fmt::format("External watershed bucket storage size overflowed. Voxel count: {}; bucket count: {}; values per queue block: {}.",
                                                                              valueCount, plan.bucket_count, k_WatershedQueueBlockValueCount));
    }
  }
  const usize planningHeadroomBytes = targetBytes / 8;
  const usize metadataAndHeadroomBytes = std::max(planningHeadroomBytes, plan.queue_metadata_bytes);
  if(metadataAndHeadroomBytes >= targetBytes)
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(
        -79050, fmt::format("External watershed's {}-byte working-memory target cannot hold its {} bytes of queue metadata and planning headroom.", targetBytes, metadataAndHeadroomBytes));
  }
  const usize bytesAfterMetadata = targetBytes - metadataAndHeadroomBytes;
  bool residentStatusPackedRouteFits = false;

  if constexpr(k_UseWatershedBucketQueue<TInput>)
  {
    if(markWatershedLine)
    {
      const usize residentStatusRecordsPerPage = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(WatershedPackedResidentStatusVoxelRecordType<TInput>));
      usize residentStatusVoxelPageBytes = 0;
      usize residentStatusBucketPageBytes = 0;
      usize residentStatusMinimumCacheBytes = 0;
      usize residentStatusTransferBytesPerRecord = 0;
      usize residentStatusFinalizationBytesPerRecord = 0;
      usize residentStatusMinimumTransferBytes = 0;
      if(!TryMultiplyWatershedSize(residentStatusRecordsPerPage, sizeof(WatershedPackedResidentStatusVoxelRecordType<TInput>), residentStatusVoxelPageBytes) ||
         !TryMultiplyWatershedSize(plan.bucket_blocks_per_page, sizeof(WatershedQueueBlock), residentStatusBucketPageBytes) ||
         !TryAddWatershedSize(residentStatusVoxelPageBytes, residentStatusBucketPageBytes, residentStatusMinimumCacheBytes) ||
         !TryAddWatershedSize(sizeof(TInput), sizeof(uint32), residentStatusTransferBytesPerRecord) ||
         !TryAddWatershedSize(residentStatusTransferBytesPerRecord, sizeof(WatershedPackedResidentStatusVoxelRecordType<TInput>), residentStatusTransferBytesPerRecord) ||
         !TryAddWatershedSize(sizeof(WatershedPackedResidentStatusVoxelRecordType<TInput>), sizeof(uint32), residentStatusFinalizationBytesPerRecord) ||
         !TryMultiplyWatershedSize(residentStatusRecordsPerPage, std::max(residentStatusTransferBytesPerRecord, residentStatusFinalizationBytesPerRecord), residentStatusMinimumTransferBytes))
      {
        return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, "External watershed resident-status packed page-size arithmetic overflowed.");
      }
      if(plan.resident_status_bytes <= bytesAfterMetadata && residentStatusMinimumCacheBytes <= bytesAfterMetadata - plan.resident_status_bytes &&
         residentStatusMinimumTransferBytes <= bytesAfterMetadata - plan.resident_status_bytes - residentStatusMinimumCacheBytes)
      {
        residentStatusPackedRouteFits = true;
      }
    }
  }

  if constexpr(k_UseWatershedBucketQueue<TInput>)
  {
    const usize splitVoxelRecordsPerPage = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(WatershedBucketVoxelRecord));
    const usize splitInputRecordsPerPage = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(TInput));
    const usize splitVoxelPageCount = valueCount / splitVoxelRecordsPerPage + static_cast<usize>(valueCount % splitVoxelRecordsPerPage != 0);
    const usize splitInputPageCount = valueCount / splitInputRecordsPerPage + static_cast<usize>(valueCount % splitInputRecordsPerPage != 0);
    usize splitVoxelPageBytes = 0;
    usize splitInputPageBytes = 0;
    usize splitVoxelCacheBytes = 0;
    usize splitInputCacheBytes = 0;
    usize splitQueuePageBytes = 0;
    usize splitRouteBytes = 0;
    usize splitInitializationBytes = 0;
    usize splitFinalizationBytes = 0;
    const usize combinedVoxelRecordsPerPage = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(WatershedPackedVoxelRecordType<TInput>));
    const usize combinedVoxelPageCount = valueCount / combinedVoxelRecordsPerPage + static_cast<usize>(valueCount % combinedVoxelRecordsPerPage != 0);
    usize combinedVoxelPageBytes = 0;
    usize combinedVoxelCacheBytes = 0;
    usize combinedInitializationBytes = 0;
    usize combinedFinalizationBytes = 0;
    usize combinedRouteBytes = 0;
    if(!TryMultiplyWatershedSize(splitVoxelRecordsPerPage, sizeof(WatershedBucketVoxelRecord), splitVoxelPageBytes) ||
       !TryMultiplyWatershedSize(splitInputRecordsPerPage, sizeof(TInput), splitInputPageBytes) || !TryMultiplyWatershedSize(splitVoxelPageCount, splitVoxelPageBytes, splitVoxelCacheBytes) ||
       !TryMultiplyWatershedSize(splitInputPageCount, splitInputPageBytes, splitInputCacheBytes) ||
       !TryMultiplyWatershedSize(plan.bucket_blocks_per_page, sizeof(WatershedQueueBlock), splitQueuePageBytes) || !TryAddWatershedSize(splitVoxelCacheBytes, splitInputCacheBytes, splitRouteBytes) ||
       !TryAddWatershedSize(splitRouteBytes, splitQueuePageBytes, splitRouteBytes) || !TryAddWatershedSize(sizeof(TInput), sizeof(uint32), splitInitializationBytes) ||
       !TryAddWatershedSize(splitInitializationBytes, sizeof(WatershedBucketVoxelRecord), splitInitializationBytes) ||
       !TryAddWatershedSize(sizeof(WatershedBucketVoxelRecord), sizeof(uint32), splitFinalizationBytes) ||
       !TryMultiplyWatershedSize(combinedVoxelRecordsPerPage, sizeof(WatershedPackedVoxelRecordType<TInput>), combinedVoxelPageBytes) ||
       !TryMultiplyWatershedSize(combinedVoxelPageCount, combinedVoxelPageBytes, combinedVoxelCacheBytes) || !TryAddWatershedSize(sizeof(TInput), sizeof(uint32), combinedInitializationBytes) ||
       !TryAddWatershedSize(combinedInitializationBytes, sizeof(WatershedPackedVoxelRecordType<TInput>), combinedInitializationBytes) ||
       !TryAddWatershedSize(sizeof(WatershedPackedVoxelRecordType<TInput>), sizeof(uint32), combinedFinalizationBytes) ||
       !TryAddWatershedSize(combinedVoxelCacheBytes, splitQueuePageBytes, combinedRouteBytes))
    {
      return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, "External watershed split bucket-state planning overflowed.");
    }
    const usize splitTransferBytesPerRecord = std::max(splitInitializationBytes, splitFinalizationBytes);
    const usize splitStatusBytes = markWatershedLine ? plan.resident_status_bytes : 0;
    const usize combinedTransferBytesPerRecord = std::max(combinedInitializationBytes, combinedFinalizationBytes);
    const usize combinedStatusBytes = markWatershedLine ? plan.resident_status_bytes : 0;
    const bool combinedRouteFits = TryAddWatershedSize(combinedRouteBytes, combinedStatusBytes, combinedRouteBytes) &&
                                   TryAddWatershedSize(combinedRouteBytes, combinedTransferBytesPerRecord, combinedRouteBytes) && combinedRouteBytes <= bytesAfterMetadata;
    if(combinedRouteFits)
    {
      plan.complete_primary_state_cache = true;
      plan.use_resident_status = markWatershedLine;
    }
    else if(TryAddWatershedSize(splitRouteBytes, splitStatusBytes, splitRouteBytes) && TryAddWatershedSize(splitRouteBytes, splitTransferBytesPerRecord, splitRouteBytes) &&
            splitRouteBytes <= bytesAfterMetadata)
    {
      plan.use_split_bucket_state = true;
      plan.complete_primary_state_cache = true;
      plan.use_resident_status = markWatershedLine;
    }
  }

  if(!plan.complete_primary_state_cache && !plan.use_split_bucket_state && residentStatusPackedRouteFits)
  {
    plan.use_packed_resident_status_state = true;
    plan.use_resident_status = true;
  }

  plan.use_tiled_voxel_layout = plan.use_bucket_queue && !plan.complete_primary_state_cache && !plan.use_split_bucket_state;
  plan.use_packed_combined_state = plan.use_bucket_queue && !plan.use_split_bucket_state && !plan.use_packed_resident_status_state;
  plan.voxel_record_bytes = plan.use_split_bucket_state ?
                                sizeof(WatershedBucketVoxelRecord) :
                                (plan.use_packed_resident_status_state ? sizeof(WatershedPackedResidentStatusVoxelRecordType<TInput>) :
                                                                         (plan.use_packed_combined_state ? sizeof(WatershedPackedVoxelRecordType<TInput>) : sizeof(WatershedVoxelRecord<TInput>)));
  plan.voxel_records_per_page = std::max<usize>(1, k_WatershedPageTargetBytes / plan.voxel_record_bytes);
  if(plan.use_tiled_voxel_layout)
  {
    constexpr usize k_TiledRecordMultiple = 32 * 32;
    plan.voxel_records_per_page = (plan.voxel_records_per_page / k_TiledRecordMultiple) * k_TiledRecordMultiple;
    if(plan.voxel_records_per_page == 0)
    {
      return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, fmt::format("External watershed's {}-byte page target cannot hold one {}-record tiled page of {}-byte records.",
                                                                                      k_WatershedPageTargetBytes, k_TiledRecordMultiple, plan.voxel_record_bytes));
    }
  }
  plan.input_records_per_page = plan.use_split_bucket_state ? std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(TInput)) : 0;
  plan.heap_records_per_page = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(WatershedHeapRecord<TInput>));
  usize voxelPageBytes = 0;
  usize inputPageBytes = 0;
  usize bucketPageBytes = 0;
  usize heapPageBytes = 0;
  if(!TryMultiplyWatershedSize(plan.voxel_records_per_page, plan.voxel_record_bytes, voxelPageBytes) ||
     (plan.use_split_bucket_state && !TryMultiplyWatershedSize(plan.input_records_per_page, sizeof(TInput), inputPageBytes)) ||
     (plan.use_bucket_queue && !TryMultiplyWatershedSize(plan.bucket_blocks_per_page, sizeof(WatershedQueueBlock), bucketPageBytes)) ||
     (!plan.use_bucket_queue && !TryMultiplyWatershedSize(plan.heap_records_per_page, sizeof(WatershedHeapRecord<TInput>), heapPageBytes)))
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, "External watershed page-size arithmetic overflowed.");
  }
  usize initializationBytesPerRecord = 0;
  usize finalizationBytesPerRecord = 0;
  if(!TryAddWatershedSize(sizeof(TInput), sizeof(uint32), initializationBytesPerRecord) || !TryAddWatershedSize(initializationBytesPerRecord, plan.voxel_record_bytes, initializationBytesPerRecord) ||
     !TryAddWatershedSize(plan.voxel_record_bytes, sizeof(uint32), finalizationBytesPerRecord))
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, "External watershed transfer-size arithmetic overflowed.");
  }
  const usize transferBytesPerRecord = std::max(initializationBytesPerRecord, finalizationBytesPerRecord);
  usize minimumCacheBytes = voxelPageBytes;
  const usize minimumQueueCacheBytes = plan.use_bucket_queue ? bucketPageBytes : heapPageBytes;
  if(!TryAddWatershedSize(minimumCacheBytes, inputPageBytes, minimumCacheBytes) || !TryAddWatershedSize(minimumCacheBytes, minimumQueueCacheBytes, minimumCacheBytes))
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, "External watershed minimum page-cache size overflowed.");
  }
  if(minimumCacheBytes > bytesAfterMetadata)
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(
        -79050, fmt::format("External watershed's {}-byte working-memory target cannot hold queue metadata, planning headroom, and one required record page per cache.", targetBytes));
  }
  const usize selectedStatusBytes = plan.use_resident_status ? plan.resident_status_bytes : 0;
  const usize bytesAfterStatus = bytesAfterMetadata - selectedStatusBytes;
  usize minimumTiledTransferBytes = 0;
  if(plan.use_packed_resident_status_state && !TryMultiplyWatershedSize(plan.voxel_records_per_page, transferBytesPerRecord, minimumTiledTransferBytes))
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, "External watershed resident-status packed transfer-page size overflowed.");
  }
  const usize transferTargetBytes =
      plan.use_packed_resident_status_state ? std::min(k_WatershedTransferTargetBytes, std::max({targetBytes / 16, voxelPageBytes, minimumTiledTransferBytes})) : k_WatershedTransferTargetBytes;
  const usize maximumTransferBytes = std::min(transferTargetBytes, bytesAfterStatus - minimumCacheBytes);
  plan.transfer_records = std::min(valueCount, maximumTransferBytes / transferBytesPerRecord);
  if(plan.transfer_records == 0)
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(
        -79050, fmt::format("External watershed's {}-byte working-memory target leaves a {}-byte transfer allowance that cannot hold one {}-byte transfer record.", targetBytes, maximumTransferBytes,
                            transferBytesPerRecord));
  }

  usize transferBytes = 0;
  if(!TryMultiplyWatershedSize(transferBytesPerRecord, plan.transfer_records, transferBytes))
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, "External watershed transfer-buffer size overflowed.");
  }
  const usize reusableCacheBytes = bytesAfterStatus - transferBytes;
  const usize queueCacheWeight = plan.use_bucket_queue ? k_WatershedBucketCacheWeight : k_WatershedHeapCacheWeight;
  const usize inputCacheWeight = plan.use_split_bucket_state ? k_WatershedInputCacheWeight : 0;
  const usize cacheWeightTotal = k_WatershedVoxelCacheWeight + inputCacheWeight + queueCacheWeight;
  const usize voxelCacheTarget = (reusableCacheBytes / cacheWeightTotal) * k_WatershedVoxelCacheWeight + ((reusableCacheBytes % cacheWeightTotal) * k_WatershedVoxelCacheWeight) / cacheWeightTotal;
  const usize inputCacheTarget = (reusableCacheBytes / cacheWeightTotal) * inputCacheWeight + ((reusableCacheBytes % cacheWeightTotal) * inputCacheWeight) / cacheWeightTotal;
  const usize queueCacheTarget = reusableCacheBytes - voxelCacheTarget - inputCacheTarget;
  const usize voxelPageCount = valueCount / plan.voxel_records_per_page + static_cast<usize>(valueCount % plan.voxel_records_per_page != 0);
  const usize inputPageCount = plan.use_split_bucket_state ? valueCount / plan.input_records_per_page + static_cast<usize>(valueCount % plan.input_records_per_page != 0) : 0;
  const usize bucketPageCount = plan.use_bucket_queue ? plan.bucket_block_count / plan.bucket_blocks_per_page + static_cast<usize>(plan.bucket_block_count % plan.bucket_blocks_per_page != 0) : 0;
  const usize bucketUsefulPageCount = plan.use_bucket_queue ? (plan.complete_primary_state_cache ? bucketPageCount : std::min(bucketPageCount, 2 * plan.bucket_count)) : 0;
  const usize heapPageCount = plan.use_bucket_queue ? 0 : valueCount / plan.heap_records_per_page + static_cast<usize>(valueCount % plan.heap_records_per_page != 0);
  plan.voxel_cache_pages = std::min(voxelPageCount, std::max<usize>(1, voxelCacheTarget / voxelPageBytes));
  plan.input_cache_pages = plan.use_split_bucket_state ? std::min(inputPageCount, std::max<usize>(1, inputCacheTarget / inputPageBytes)) : 0;
  plan.bucket_cache_pages = plan.use_bucket_queue ? std::min(bucketUsefulPageCount, std::max<usize>(1, queueCacheTarget / bucketPageBytes)) : 0;
  plan.heap_cache_pages = plan.use_bucket_queue ? 0 : std::min(heapPageCount, std::max<usize>(1, queueCacheTarget / heapPageBytes));

  usize voxelCacheBytes = 0;
  usize inputCacheBytes = 0;
  usize bucketCacheBytes = 0;
  usize heapCacheBytes = 0;
  usize allocatedCacheBytes = 0;
  if(!TryMultiplyWatershedSize(voxelPageBytes, plan.voxel_cache_pages, voxelCacheBytes) || !TryMultiplyWatershedSize(inputPageBytes, plan.input_cache_pages, inputCacheBytes) ||
     !TryMultiplyWatershedSize(bucketPageBytes, plan.bucket_cache_pages, bucketCacheBytes) || !TryMultiplyWatershedSize(heapPageBytes, plan.heap_cache_pages, heapCacheBytes) ||
     !TryAddWatershedSize(voxelCacheBytes, inputCacheBytes, allocatedCacheBytes) || !TryAddWatershedSize(allocatedCacheBytes, bucketCacheBytes, allocatedCacheBytes) ||
     !TryAddWatershedSize(allocatedCacheBytes, heapCacheBytes, allocatedCacheBytes) || allocatedCacheBytes > reusableCacheBytes)
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, fmt::format("External watershed cache planning exceeded its {}-byte reusable cache allowance.", reusableCacheBytes));
  }

  usize remainingCacheBytes = reusableCacheBytes - allocatedCacheBytes;
  GrowWatershedCachePages(voxelPageBytes, voxelPageCount, plan.voxel_cache_pages, remainingCacheBytes);
  GrowWatershedCachePages(inputPageBytes, inputPageCount, plan.input_cache_pages, remainingCacheBytes);
  if(plan.use_bucket_queue)
  {
    GrowWatershedCachePages(bucketPageBytes, bucketUsefulPageCount, plan.bucket_cache_pages, remainingCacheBytes);
  }
  else
  {
    GrowWatershedCachePages(heapPageBytes, heapPageCount, plan.heap_cache_pages, remainingCacheBytes);
  }

  usize residentBytes = 0;
  if(!TryMultiplyWatershedSize(voxelPageBytes, plan.voxel_cache_pages, voxelCacheBytes) || !TryMultiplyWatershedSize(inputPageBytes, plan.input_cache_pages, inputCacheBytes) ||
     !TryMultiplyWatershedSize(bucketPageBytes, plan.bucket_cache_pages, bucketCacheBytes) || !TryMultiplyWatershedSize(heapPageBytes, plan.heap_cache_pages, heapCacheBytes) ||
     !TryAddWatershedSize(voxelCacheBytes, inputCacheBytes, residentBytes) || !TryAddWatershedSize(residentBytes, bucketCacheBytes, residentBytes) ||
     !TryAddWatershedSize(residentBytes, heapCacheBytes, residentBytes) || !TryAddWatershedSize(residentBytes, transferBytes, residentBytes) ||
     !TryAddWatershedSize(residentBytes, selectedStatusBytes, residentBytes) || !TryAddWatershedSize(residentBytes, plan.queue_metadata_bytes, residentBytes) || residentBytes > targetBytes)
  {
    return MakeErrorResult<WatershedExternalMemoryPlan<TInput>>(-79050, fmt::format("External watershed resident planning exceeded its {}-byte working-memory target.", targetBytes));
  }
  plan.resident_bytes = residentBytes;
  return {plan};
}

template <class TInput>
Result<usize> CalculateWatershedUsefulWorkingMemoryBytes(usize valueCount, bool markWatershedLine = true)
{
  if(valueCount == 0)
  {
    return MakeErrorResult<usize>(-79050, "External watershed cannot size useful working memory for zero voxels.");
  }

  // The useful endpoint must hold the largest complete bucket route. The packed combined route is larger than the split
  // output-plus-input route for both uint8 and uint16 inputs, so it is the conservative bucket estimate.
  constexpr usize k_VoxelRecordBytes = k_UseWatershedBucketQueue<TInput> ? sizeof(WatershedPackedVoxelRecordType<TInput>) : sizeof(WatershedVoxelRecord<TInput>);
  const usize voxelRecordsPerPage = std::max<usize>(1, k_WatershedPageTargetBytes / k_VoxelRecordBytes);
  const usize bucketBlocksPerPage = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(WatershedQueueBlock));
  const usize heapRecordsPerPage = std::max<usize>(1, k_WatershedPageTargetBytes / sizeof(WatershedHeapRecord<TInput>));
  usize voxelPageBytes = 0;
  usize bucketPageBytes = 0;
  usize heapPageBytes = 0;
  if(!TryMultiplyWatershedSize(voxelRecordsPerPage, k_VoxelRecordBytes, voxelPageBytes) || !TryMultiplyWatershedSize(bucketBlocksPerPage, sizeof(WatershedQueueBlock), bucketPageBytes) ||
     !TryMultiplyWatershedSize(heapRecordsPerPage, sizeof(WatershedHeapRecord<TInput>), heapPageBytes))
  {
    return MakeErrorResult<usize>(-79050, "External watershed useful-memory page size overflowed.");
  }

  const usize voxelPageCount = valueCount / voxelRecordsPerPage + static_cast<usize>(valueCount % voxelRecordsPerPage != 0);
  usize bucketBlockCount = 0;
  usize bucketCount = 0;
  if constexpr(k_UseWatershedBucketQueue<TInput>)
  {
    bucketCount = usize{1} << (sizeof(TInput) * 8);
    if(!TryCalculateWatershedBucketBlockCount(valueCount, bucketCount, bucketBlockCount))
    {
      return MakeErrorResult<usize>(-79050, fmt::format("External watershed useful-memory bucket storage size overflowed. Voxel count: {}; bucket count: {}; values per queue block: {}.", valueCount,
                                                        bucketCount, k_WatershedQueueBlockValueCount));
    }
  }
  const usize bucketPageCount = bucketBlockCount / bucketBlocksPerPage + static_cast<usize>(bucketBlockCount % bucketBlocksPerPage != 0);
  const usize bucketUsefulPageCount = k_UseWatershedBucketQueue<TInput> ? bucketPageCount : 0;
  const usize heapPageCount = valueCount / heapRecordsPerPage + static_cast<usize>(valueCount % heapRecordsPerPage != 0);
  usize voxelCacheBytes = 0;
  usize bucketCacheBytes = 0;
  usize heapCacheBytes = 0;
  if(!TryMultiplyWatershedSize(voxelPageCount, voxelPageBytes, voxelCacheBytes) ||
     (k_UseWatershedBucketQueue<TInput> && !TryMultiplyWatershedSize(bucketUsefulPageCount, bucketPageBytes, bucketCacheBytes)) ||
     (!k_UseWatershedBucketQueue<TInput> && !TryMultiplyWatershedSize(heapPageCount, heapPageBytes, heapCacheBytes)))
  {
    return MakeErrorResult<usize>(-79050, "External watershed useful-memory page-cache size overflowed.");
  }

  usize initializationBytesPerRecord = 0;
  usize finalizationBytesPerRecord = 0;
  if(!TryAddWatershedSize(sizeof(TInput), sizeof(uint32), initializationBytesPerRecord) || !TryAddWatershedSize(initializationBytesPerRecord, k_VoxelRecordBytes, initializationBytesPerRecord) ||
     !TryAddWatershedSize(k_VoxelRecordBytes, sizeof(uint32), finalizationBytesPerRecord))
  {
    return MakeErrorResult<usize>(-79050, "External watershed useful-memory transfer size overflowed.");
  }
  const usize transferBytesPerRecord = std::max(initializationBytesPerRecord, finalizationBytesPerRecord);
  const usize transferRecords = std::min(valueCount, k_WatershedTransferTargetBytes / transferBytesPerRecord);
  usize transferBytes = 0;
  if(transferRecords == 0 || !TryMultiplyWatershedSize(transferRecords, transferBytesPerRecord, transferBytes))
  {
    return MakeErrorResult<usize>(-79050, "External watershed useful-memory transfer buffer cannot hold one record.");
  }

  usize queueMetadataBytes = 0;
  if constexpr(k_UseWatershedBucketQueue<TInput>)
  {
    if(!TryMultiplyWatershedSize(bucketCount, sizeof(WatershedBucketEndpoints), queueMetadataBytes))
    {
      return MakeErrorResult<usize>(-79050, "External watershed useful-memory bucket metadata size overflowed.");
    }
  }

  usize pageAndTransferBytes = 0;
  if(!TryAddWatershedSize(voxelCacheBytes, bucketCacheBytes, pageAndTransferBytes) || !TryAddWatershedSize(pageAndTransferBytes, heapCacheBytes, pageAndTransferBytes) ||
     !TryAddWatershedSize(pageAndTransferBytes, transferBytes, pageAndTransferBytes))
  {
    return MakeErrorResult<usize>(-79050, "External watershed useful-memory payload size overflowed.");
  }
  if(markWatershedLine && k_UseWatershedBucketQueue<TInput>)
  {
    const usize statusWordCount = valueCount / 64 + static_cast<usize>(valueCount % 64 != 0);
    usize statusBytes = 0;
    if(!TryMultiplyWatershedSize(statusWordCount, sizeof(uint64), statusBytes) || !TryAddWatershedSize(pageAndTransferBytes, statusBytes, pageAndTransferBytes))
    {
      return MakeErrorResult<usize>(-79050, fmt::format("External watershed useful resident-status size overflowed. Voxel count: {}; status words: {}.", valueCount, statusWordCount));
    }
  }
  const usize pageContainerHeadroomBytes = pageAndTransferBytes / 7 + static_cast<usize>(pageAndTransferBytes % 7 != 0);
  const usize reservedHeadroomBytes = std::max(pageContainerHeadroomBytes, queueMetadataBytes);
  usize usefulBytes = 0;
  if(!TryAddWatershedSize(pageAndTransferBytes, reservedHeadroomBytes, usefulBytes))
  {
    return MakeErrorResult<usize>(-79050, "External watershed useful working-memory size overflowed.");
  }
  return {usefulBytes};
}

template <class TInput>
Result<WatershedExternalMemoryAllocation<TInput>> ReserveWatershedExternalMemoryPlan(usize valueCount, uint32 preferredNumerator = k_WatershedPreferredUsefulNumerator,
                                                                                     uint32 preferredDenominator = k_WatershedPreferredUsefulDenominator,
                                                                                     uint64 minimumBytes = k_WatershedExternalTargetBytes, bool markWatershedLine = true)
{
  auto usefulBytesResult = CalculateWatershedUsefulWorkingMemoryBytes<TInput>(valueCount, markWatershedLine);
  if(usefulBytesResult.invalid())
  {
    return ConvertInvalidResult<WatershedExternalMemoryAllocation<TInput>>(std::move(usefulBytesResult));
  }

  auto reservation = ReserveWorkingMemoryFraction(usefulBytesResult.value(), preferredNumerator, preferredDenominator, minimumBytes);
  if(reservation.sizeBytes() == 0 || reservation.sizeBytes() > std::numeric_limits<usize>::max())
  {
    return MakeErrorResult<WatershedExternalMemoryAllocation<TInput>>(
        -79050, fmt::format("External watershed could not reserve working memory. Preferred useful fraction: {}/{}; minimum bytes: {}; useful bytes: {}; granted bytes: {}.", preferredNumerator,
                            preferredDenominator, minimumBytes, usefulBytesResult.value(), reservation.sizeBytes()));
  }

  auto planResult = CreateWatershedExternalMemoryPlan<TInput>(valueCount, static_cast<usize>(reservation.sizeBytes()), markWatershedLine);
  if(planResult.invalid())
  {
    return ConvertInvalidResult<WatershedExternalMemoryAllocation<TInput>>(std::move(planResult));
  }

  WatershedExternalMemoryAllocation<TInput> allocation{std::move(reservation), std::move(planResult.value())};
  return {std::move(allocation)};
}

template <class TInput>
TInput WatershedEffectiveLevel(TInput grayValue, TInput currentLevel)
{
  return grayValue <= currentLevel ? currentLevel : grayValue;
}

template <class T>
std::string DescribeWatershedStoreError(const Result<T>& result)
{
  if(result.errors().empty())
  {
    return "temporary store returned an unspecified error";
  }
  return result.errors().front().message;
}

template <class TInput>
class WatershedExternalMinHeap
{
public:
  using RecordType = WatershedHeapRecord<TInput>;

  static Result<std::unique_ptr<WatershedExternalMinHeap>> Create(std::unique_ptr<ITemporaryRecordStore> store, usize recordsPerPage, usize maximumPages, const std::atomic_bool& shouldCancel)
  {
    if(store == nullptr || store->recordSize() != sizeof(RecordType) || store->recordCount() == 0 || store->maxRecordsPerBatch() == 0 || recordsPerPage == 0 ||
       recordsPerPage > store->maxRecordsPerBatch() || maximumPages == 0)
    {
      return MakeErrorResult<std::unique_ptr<WatershedExternalMinHeap>>(
          -79051,
          fmt::format("External watershed heap received invalid storage metadata: record size {}, record count {}, batch {}, page records {}, cache pages {}.",
                      store == nullptr ? 0 : store->recordSize(), store == nullptr ? 0 : store->recordCount(), store == nullptr ? 0 : store->maxRecordsPerBatch(), recordsPerPage, maximumPages));
    }
    try
    {
      return {std::unique_ptr<WatershedExternalMinHeap>(new WatershedExternalMinHeap(std::move(store), recordsPerPage, maximumPages, shouldCancel))};
    } catch(const std::exception& exception)
    {
      return MakeErrorResult<std::unique_ptr<WatershedExternalMinHeap>>(-79051, fmt::format("External watershed heap allocation failed: {}", exception.what()));
    }
  }

  Result<> push(TInput level, int64 flatIndex)
  {
    if(m_ShouldCancel)
    {
      return MakeErrorResult(-79052, "External watershed heap push was cancelled.");
    }
    if(m_Size >= m_Store->recordCount())
    {
      return MakeErrorResult(-79052, fmt::format("External watershed heap capacity ({}) is exhausted while pushing flat index {}.", m_Store->recordCount(), flatIndex));
    }
    if(m_NextSequence == std::numeric_limits<uint64>::max())
    {
      return MakeErrorResult(-79052, "External watershed heap insertion sequence overflowed.");
    }
    const RecordType inserted{level, m_NextSequence++, flatIndex};
    uint64 index = m_Size++;
    while(index > 0)
    {
      const uint64 parentIndex = (index - 1) / 2;
      auto parentResult = read(parentIndex);
      if(parentResult.invalid())
      {
        return ConvertResult(std::move(parentResult));
      }
      if(!Less(inserted, parentResult.value()))
      {
        break;
      }
      if(Result<> result = write(index, parentResult.value()); result.invalid())
      {
        return result;
      }
      index = parentIndex;
    }
    return write(index, inserted);
  }

  Result<RecordType> pop()
  {
    if(m_ShouldCancel)
    {
      return MakeErrorResult<RecordType>(-79053, "External watershed heap pop was cancelled.");
    }
    if(m_Size == 0)
    {
      return MakeErrorResult<RecordType>(-79053, "External watershed heap pop requested an empty queue.");
    }
    auto rootResult = read(0);
    if(rootResult.invalid())
    {
      return rootResult;
    }
    --m_Size;
    if(m_Size == 0)
    {
      return rootResult;
    }
    auto lastResult = read(m_Size);
    if(lastResult.invalid())
    {
      return lastResult;
    }
    const RecordType replacement = lastResult.value();
    uint64 index = 0;
    while(true)
    {
      if(index > (std::numeric_limits<uint64>::max() - 1) / 2)
      {
        break;
      }
      const uint64 left = index * 2 + 1;
      if(left >= m_Size)
      {
        break;
      }
      const uint64 right = left + 1;
      auto childResult = read(left);
      if(childResult.invalid())
      {
        return childResult;
      }
      uint64 selectedIndex = left;
      RecordType selected = childResult.value();
      if(right < m_Size)
      {
        auto rightResult = read(right);
        if(rightResult.invalid())
        {
          return rightResult;
        }
        if(Less(rightResult.value(), selected))
        {
          selectedIndex = right;
          selected = rightResult.value();
        }
      }
      if(!Less(selected, replacement))
      {
        break;
      }
      if(Result<> result = write(index, selected); result.invalid())
      {
        return ConvertInvalidResult<RecordType>(std::move(result));
      }
      index = selectedIndex;
    }
    if(Result<> result = write(index, replacement); result.invalid())
    {
      return ConvertInvalidResult<RecordType>(std::move(result));
    }
    return rootResult;
  }

  Result<> flush()
  {
    auto result = m_Cache.flush(m_ShouldCancel);
    if(result.invalid())
    {
      return MakeErrorResult(-79054, fmt::format("External watershed heap flush failed: {}", DescribeWatershedStoreError(result)));
    }
    return {};
  }

  bool empty() const
  {
    return m_Size == 0;
  }

  uint64 size() const
  {
    return m_Size;
  }

private:
  WatershedExternalMinHeap(std::unique_ptr<ITemporaryRecordStore> store, usize recordsPerPage, usize maximumPages, const std::atomic_bool& shouldCancel)
  : m_Store(std::move(store))
  , m_Cache(*m_Store, recordsPerPage, maximumPages)
  , m_ShouldCancel(shouldCancel)
  {
  }

  static bool Less(const RecordType& left, const RecordType& right)
  {
    if(left.level < right.level)
    {
      return true;
    }
    if(right.level < left.level)
    {
      return false;
    }
    return left.sequence < right.sequence;
  }

  Result<RecordType> read(uint64 index)
  {
    auto result = m_Cache.read(index, m_ShouldCancel);
    if(result.invalid())
    {
      return MakeErrorResult<RecordType>(-79054, fmt::format("External watershed heap read at index {} failed: {}", index, DescribeWatershedStoreError(result)));
    }
    return result;
  }

  Result<> write(uint64 index, const RecordType& value)
  {
    auto result = m_Cache.write(index, value, m_ShouldCancel);
    if(result.invalid())
    {
      return MakeErrorResult(-79054, fmt::format("External watershed heap write at index {} failed: {}", index, DescribeWatershedStoreError(result)));
    }
    return {};
  }

  std::unique_ptr<ITemporaryRecordStore> m_Store;
  BoundedRecordPageCache<RecordType> m_Cache;
  const std::atomic_bool& m_ShouldCancel;
  uint64 m_Size = 0;
  uint64 m_NextSequence = 0;
};

template <class TInput>
class WatershedExternalBucketQueue
{
public:
  using RecordType = WatershedHeapRecord<TInput>;
  using BlockType = WatershedQueueBlock;

  static Result<std::unique_ptr<WatershedExternalBucketQueue>> Create(std::unique_ptr<ITemporaryRecordStore> store, usize recordsPerPage, usize maximumPages, uint64 capacity,
                                                                      const std::atomic_bool& shouldCancel)
  {
    static_assert(k_UseWatershedBucketQueue<TInput>);
    if(store == nullptr || store->recordSize() != sizeof(BlockType) || store->recordCount() == 0 || store->maxRecordsPerBatch() == 0 || recordsPerPage == 0 ||
       recordsPerPage > store->maxRecordsPerBatch() || maximumPages == 0 || capacity == 0 || capacity > static_cast<uint64>(std::numeric_limits<int64>::max()))
    {
      return MakeErrorResult<std::unique_ptr<WatershedExternalBucketQueue>>(
          -79064, fmt::format("External watershed bucket queue received invalid storage or cache limits. Record bytes: {}; records: {}; provider batch: {}; records per page: {}; cache pages: {}; "
                              "voxel capacity: {}.",
                              store == nullptr ? 0 : store->recordSize(), store == nullptr ? 0 : store->recordCount(), store == nullptr ? 0 : store->maxRecordsPerBatch(), recordsPerPage, maximumPages,
                              capacity));
    }
    try
    {
      return {std::unique_ptr<WatershedExternalBucketQueue>(new WatershedExternalBucketQueue(std::move(store), recordsPerPage, maximumPages, capacity, shouldCancel))};
    } catch(const std::exception& exception)
    {
      return MakeErrorResult<std::unique_ptr<WatershedExternalBucketQueue>>(-79064, fmt::format("External watershed bucket queue allocation failed for {} voxels: {}", capacity, exception.what()));
    }
  }

  Result<> push(TInput level, int64 flatIndex)
  {
    if(m_ShouldCancel)
    {
      return MakeErrorResult(-79065, "External watershed bucket push was cancelled.");
    }
    if(flatIndex < 0 || static_cast<uint64>(flatIndex) >= m_Capacity || m_Size >= m_Capacity)
    {
      return MakeErrorResult(-79065, fmt::format("External watershed bucket push rejected flat index {}. Voxel capacity: {}; queued values: {}.", flatIndex, m_Capacity, m_Size));
    }
    const usize bucketIndex = OrderedIndex(level);
    WatershedBucketEndpoints& bucket = m_Buckets[bucketIndex];
    uint64 blockIndex = bucket.tail_block;
    if(blockIndex == k_WatershedInvalidQueueBlock)
    {
      auto blockIndexResult = allocateBlock();
      if(blockIndexResult.invalid())
      {
        return ConvertResult(std::move(blockIndexResult));
      }
      blockIndex = blockIndexResult.value();
      bucket.head_block = blockIndex;
      bucket.tail_block = blockIndex;
      bucket.head_offset = 0;
      auto initializeResult = m_Cache.modify(
          blockIndex,
          [flatIndex](BlockType& block) noexcept {
            block = {};
            block.values[0] = flatIndex;
            block.value_count = 1;
          },
          m_ShouldCancel);
      if(initializeResult.invalid())
      {
        return MakeErrorResult(-79068, fmt::format("External watershed bucket queue initialization of block {} failed: {}", blockIndex, DescribeWatershedStoreError(initializeResult)));
      }
    }
    else
    {
      bool appended = false;
      bool invalidMetadata = false;
      auto appendResult = m_Cache.modify(
          blockIndex,
          [flatIndex, &appended, &invalidMetadata](BlockType& block) noexcept {
            if(block.value_count > block.values.size())
            {
              invalidMetadata = true;
              return;
            }
            if(block.value_count < block.values.size())
            {
              block.values[static_cast<usize>(block.value_count)] = flatIndex;
              ++block.value_count;
              appended = true;
            }
          },
          m_ShouldCancel);
      if(appendResult.invalid())
      {
        return MakeErrorResult(-79068, fmt::format("External watershed bucket queue append to block {} failed: {}", blockIndex, DescribeWatershedStoreError(appendResult)));
      }
      if(appended && m_LoadedHeadBlockIndex == blockIndex)
      {
        m_LoadedHeadBlock.values[static_cast<usize>(m_LoadedHeadBlock.value_count)] = flatIndex;
        ++m_LoadedHeadBlock.value_count;
      }
      if(invalidMetadata)
      {
        return MakeErrorResult(-79065, fmt::format("External watershed bucket queue block {} contains more than {} values.", blockIndex, BlockType{}.values.size()));
      }
      if(!appended)
      {
        auto nextBlockResult = allocateBlock();
        if(nextBlockResult.invalid())
        {
          return ConvertResult(std::move(nextBlockResult));
        }
        const uint64 nextBlock = nextBlockResult.value();
        auto linkResult = m_Cache.modify(blockIndex, [nextBlock](BlockType& block) noexcept { block.next_block = nextBlock; }, m_ShouldCancel);
        if(linkResult.invalid())
        {
          return MakeErrorResult(-79068, fmt::format("External watershed bucket queue link from block {} to block {} failed: {}", blockIndex, nextBlock, DescribeWatershedStoreError(linkResult)));
        }
        if(m_LoadedHeadBlockIndex == blockIndex)
        {
          m_LoadedHeadBlock.next_block = nextBlock;
        }
        blockIndex = nextBlock;
        bucket.tail_block = blockIndex;
        auto initializeResult = m_Cache.modify(
            blockIndex,
            [flatIndex](BlockType& block) noexcept {
              block = {};
              block.values[0] = flatIndex;
              block.value_count = 1;
            },
            m_ShouldCancel);
        if(initializeResult.invalid())
        {
          return MakeErrorResult(-79068, fmt::format("External watershed bucket queue initialization of block {} failed: {}", blockIndex, DescribeWatershedStoreError(initializeResult)));
        }
      }
    }
    m_MinimumBucket = std::min(m_MinimumBucket, bucketIndex);
    ++m_Size;
    return {};
  }

  Result<RecordType> pop()
  {
    if(m_ShouldCancel)
    {
      return MakeErrorResult<RecordType>(-79066, "External watershed bucket pop was cancelled.");
    }
    if(m_Size == 0 || m_MinimumBucket >= m_Buckets.size())
    {
      return MakeErrorResult<RecordType>(-79066, "External watershed bucket pop requested an empty queue.");
    }
    WatershedBucketEndpoints& bucket = m_Buckets[m_MinimumBucket];
    if(m_LoadedHeadBlockIndex != bucket.head_block)
    {
      auto inspectResult = m_Cache.inspect(bucket.head_block, [this](const BlockType& block) noexcept { m_LoadedHeadBlock = block; }, m_ShouldCancel);
      if(inspectResult.invalid())
      {
        return MakeErrorResult<RecordType>(-79067, fmt::format("External watershed bucket queue inspection of block {} failed: {}", bucket.head_block, DescribeWatershedStoreError(inspectResult)));
      }
      m_LoadedHeadBlockIndex = bucket.head_block;
    }
    if(m_LoadedHeadBlock.value_count == 0 || m_LoadedHeadBlock.value_count > m_LoadedHeadBlock.values.size() || bucket.head_offset >= m_LoadedHeadBlock.value_count)
    {
      return MakeErrorResult<RecordType>(-79066, fmt::format("External watershed bucket block {} has invalid read metadata. Stored values: {}; head offset: {}; block capacity: {}.", bucket.head_block,
                                                             m_LoadedHeadBlock.value_count, bucket.head_offset, m_LoadedHeadBlock.values.size()));
    }
    const int64 flatIndex = m_LoadedHeadBlock.values[bucket.head_offset];
    ++bucket.head_offset;
    const TInput level = LevelFromOrderedIndex(m_MinimumBucket);
    --m_Size;
    if(bucket.head_offset == m_LoadedHeadBlock.value_count)
    {
      bucket.head_block = m_LoadedHeadBlock.next_block;
      bucket.head_offset = 0;
      m_LoadedHeadBlockIndex = k_WatershedInvalidQueueBlock;
      if(bucket.head_block == k_WatershedInvalidQueueBlock)
      {
        bucket.tail_block = k_WatershedInvalidQueueBlock;
      }
    }
    if(bucket.head_block == k_WatershedInvalidQueueBlock)
    {
      while(m_MinimumBucket < m_Buckets.size() && m_Buckets[m_MinimumBucket].head_block == k_WatershedInvalidQueueBlock)
      {
        ++m_MinimumBucket;
      }
    }
    return {RecordType{level, uint64{0}, flatIndex}};
  }

  Result<> flush()
  {
    auto result = m_Cache.flush(m_ShouldCancel);
    if(result.invalid())
    {
      return MakeErrorResult(-79068, fmt::format("External watershed bucket queue flush failed: {}", DescribeWatershedStoreError(result)));
    }
    return {};
  }

  bool empty() const
  {
    return m_Size == 0;
  }

private:
  WatershedExternalBucketQueue(std::unique_ptr<ITemporaryRecordStore> store, usize recordsPerPage, usize maximumPages, uint64 capacity, const std::atomic_bool& shouldCancel)
  : m_Store(std::move(store))
  , m_Cache(*m_Store, recordsPerPage, maximumPages)
  , m_Capacity(capacity)
  , m_ShouldCancel(shouldCancel)
  , m_Buckets(usize{1} << (sizeof(TInput) * 8))
  , m_MinimumBucket(m_Buckets.size())
  {
  }

  static usize OrderedIndex(TInput level)
  {
    using UnsignedType = std::make_unsigned_t<TInput>;
    UnsignedType bits = {};
    if constexpr(std::is_signed_v<TInput>)
    {
      bits = std::bit_cast<UnsignedType>(level) ^ (UnsignedType{1} << (sizeof(TInput) * 8 - 1));
    }
    else
    {
      bits = level;
    }
    return static_cast<usize>(bits);
  }

  static TInput LevelFromOrderedIndex(usize index)
  {
    using UnsignedType = std::make_unsigned_t<TInput>;
    UnsignedType bits = static_cast<UnsignedType>(index);
    if constexpr(std::is_signed_v<TInput>)
    {
      bits ^= UnsignedType{1} << (sizeof(TInput) * 8 - 1);
      return std::bit_cast<TInput>(bits);
    }
    else
    {
      return static_cast<TInput>(bits);
    }
  }

  Result<uint64> allocateBlock()
  {
    if(m_NextBlock >= m_Store->recordCount())
    {
      return MakeErrorResult<uint64>(-79065, fmt::format("External watershed bucket queue exhausted its {} block records after queuing {} values.", m_Store->recordCount(), m_Size));
    }
    return {m_NextBlock++};
  }

  std::unique_ptr<ITemporaryRecordStore> m_Store;
  BoundedRecordPageCache<BlockType> m_Cache;
  uint64 m_Capacity = 0;
  const std::atomic_bool& m_ShouldCancel;
  std::vector<WatershedBucketEndpoints> m_Buckets;
  usize m_MinimumBucket = 0;
  uint64 m_Size = 0;
  uint64 m_NextBlock = 0;
  BlockType m_LoadedHeadBlock;
  uint64 m_LoadedHeadBlockIndex = k_WatershedInvalidQueueBlock;
};

template <class TInput>
class WatershedExternalQueue
{
public:
  using RecordType = WatershedHeapRecord<TInput>;
  using VoxelRecordType = WatershedVoxelRecord<TInput>;

  static Result<std::unique_ptr<WatershedExternalQueue>> Create(std::unique_ptr<ITemporaryRecordStore> queueStore, const WatershedExternalMemoryPlan<TInput>& plan, uint64 capacity,
                                                                const std::atomic_bool& shouldCancel)
  {
    try
    {
      auto queue = std::unique_ptr<WatershedExternalQueue>(new WatershedExternalQueue());
      if constexpr(k_UseWatershedBucketQueue<TInput>)
      {
        if(!plan.use_bucket_queue)
        {
          return MakeErrorResult<std::unique_ptr<WatershedExternalQueue>>(-79069, "External watershed bucket queue plan did not select bucket storage.");
        }
        auto result = WatershedExternalBucketQueue<TInput>::Create(std::move(queueStore), plan.bucket_blocks_per_page, plan.bucket_cache_pages, capacity, shouldCancel);
        if(result.invalid())
        {
          return ConvertInvalidResult<std::unique_ptr<WatershedExternalQueue>>(std::move(result));
        }
        queue->m_Buckets = std::move(result.value());
      }
      else
      {
        auto result = WatershedExternalMinHeap<TInput>::Create(std::move(queueStore), plan.heap_records_per_page, plan.heap_cache_pages, shouldCancel);
        if(result.invalid())
        {
          return ConvertInvalidResult<std::unique_ptr<WatershedExternalQueue>>(std::move(result));
        }
        queue->m_Heap = std::move(result.value());
      }
      return {std::move(queue)};
    } catch(const std::exception& exception)
    {
      return MakeErrorResult<std::unique_ptr<WatershedExternalQueue>>(-79069, fmt::format("External watershed queue setup failed: {}", exception.what()));
    }
  }

  Result<> push(TInput level, int64 flatIndex)
  {
    if constexpr(k_UseWatershedBucketQueue<TInput>)
    {
      return m_Buckets->push(level, flatIndex);
    }
    else
    {
      return m_Heap->push(level, flatIndex);
    }
  }

  Result<RecordType> pop()
  {
    if constexpr(k_UseWatershedBucketQueue<TInput>)
    {
      return m_Buckets->pop();
    }
    else
    {
      return m_Heap->pop();
    }
  }

  Result<> flush()
  {
    if constexpr(k_UseWatershedBucketQueue<TInput>)
    {
      return m_Buckets->flush();
    }
    else
    {
      return m_Heap->flush();
    }
  }

  bool empty() const
  {
    if constexpr(k_UseWatershedBucketQueue<TInput>)
    {
      return m_Buckets->empty();
    }
    else
    {
      return m_Heap->empty();
    }
  }

private:
  WatershedExternalQueue() = default;

  std::unique_ptr<WatershedExternalBucketQueue<TInput>> m_Buckets;
  std::unique_ptr<WatershedExternalMinHeap<TInput>> m_Heap;
};
} // namespace nx::core::ImageProcessing::detail
