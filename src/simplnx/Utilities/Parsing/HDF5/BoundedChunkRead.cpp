#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"

#include "simplnx/Common/BoundedRead.hpp"
#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/PositionalFileIO.hpp"
#include <H5Ipublic.h>
#include <H5Zpublic.h>
#include <H5public.h>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fmt/format.h>
#include <mutex>
#include <nonstd/span.hpp>
#include <string>
#include <type_traits>
#include <utility>

#include <H5Dpublic.h>
#include <H5Fpublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>
#include <zconf.h>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace nx::core::HDF5
{
namespace
{

using BoundedRead::Status;

/** @brief Labels the actual owned storage seen by the existing test observer. */
enum class BoundedCarrier : uint64
{
  Stored = 1,
  Nominal,
  Decoder,
  Selected
};

/** @brief Carries allocation identity without copying a path or dataset. */
struct BoundedIdentity
{
  const std::filesystem::path& file;
  const std::string& Dataset;

#if SIMPLNX_BUILD_TESTS
  void observe(CodecIoEventForTesting event, uint64 bytes, BoundedCarrier carrier) const noexcept
  {
    ObserveCodecIoForTesting(file, Dataset, event, bytes, static_cast<uint64>(carrier));
  }
#endif
};

/** @brief Retains each zlib allocation's payload size for quota release; the header is allocator overhead. */
struct DecoderHeader
{
  alignas(std::max_align_t) uint64 bytes = 0;
};

/** @brief Bounds zlib-owned storage; this state outlives inflateEnd and every zfree. */
struct DecoderQuota
{
  const BoundedIdentity* Identity = nullptr;
  uint64 allowance = 0;
  uint64 live = 0;
  bool quota_failed = false;
  bool allocation_failed = false;

  /**
   * @brief Services a zlib request only after charging its workspace payload.
   * @param opaque Borrows the live DecoderQuota.
   * @param count Supplies zlib's element count.
   * @param size Supplies bytes per element.
   * @return Aligned payload, or null with a fixed quota/allocation failure flag.
   */
  static voidpf Allocate(voidpf opaque, uInt count, uInt size) noexcept
  {
    auto& quota = *static_cast<DecoderQuota*>(opaque);
    uint64 payload = 0;
    uint64 bytes = 0;
    if(!BoundedRead::Multiply(count, size, payload) || !BoundedRead::Add(payload, sizeof(DecoderHeader), bytes) || payload > quota.allowance - quota.live || bytes > std::numeric_limits<usize>::max())
    {
      quota.quota_failed = true;
      return nullptr;
    }
#if SIMPLNX_BUILD_TESTS
    quota.Identity->observe(CodecIoEventForTesting::BoundedRequested, payload, BoundedCarrier::Decoder);
#endif
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(quota.Identity->file, quota.Identity->Dataset, BoundedFaultPointForTesting::DecoderAllocation))
    {
      quota.allocation_failed = true;
      return nullptr;
    }
#endif
    auto* headerPtr = static_cast<DecoderHeader*>(std::malloc(static_cast<usize>(bytes)));
    if(headerPtr == nullptr)
    {
      quota.allocation_failed = true;
      return nullptr;
    }
    headerPtr->bytes = payload;
    quota.live += payload;
#if SIMPLNX_BUILD_TESTS
    quota.Identity->observe(CodecIoEventForTesting::BoundedAcquired, payload, BoundedCarrier::Decoder);
#endif
    return headerPtr + 1;
  }

  /**
   * @brief Frees one zlib payload before releasing its actual byte charge.
   * @param opaque Borrows the quota that admitted this payload.
   * @param address Supplies the payload returned by Allocate, or null.
   */
  static void Free(voidpf opaque, voidpf address) noexcept
  {
    if(address == nullptr)
    {
      return;
    }
    auto& quota = *static_cast<DecoderQuota*>(opaque);
    auto* headerPtr = static_cast<DecoderHeader*>(address) - 1;
    const uint64 bytes = headerPtr->bytes;
    std::free(headerPtr);
    quota.live -= bytes;
#if SIMPLNX_BUILD_TESTS
    quota.Identity->observe(CodecIoEventForTesting::BoundedReleased, bytes, BoundedCarrier::Decoder);
#endif
  }
};

/** @brief Owns one exact requested byte allocation, without vector growth. */
struct BoundedBytes
{
  explicit BoundedBytes(const BoundedIdentity& identityValue, BoundedCarrier carrierValue) noexcept
  : Identity(identityValue)
  , carrier(carrierValue)
  {
  }

  BoundedBytes(const BoundedBytes&) = delete;
  BoundedBytes& operator=(const BoundedBytes&) = delete;
  BoundedBytes(BoundedBytes&&) = delete;
  BoundedBytes& operator=(BoundedBytes&&) = delete;

  const BoundedIdentity& Identity;
  BoundedCarrier carrier;
  std::unique_ptr<std::byte[]> data;
  uint64 size = 0;

  ~BoundedBytes() noexcept
  {
    if(data != nullptr)
    {
      data.reset();
#if SIMPLNX_BUILD_TESTS
      Identity.observe(CodecIoEventForTesting::BoundedReleased, size, carrier);
#endif
    }
  }

  /**
   * @brief Requests the exact byte count after the caller has admitted it.
   * @param bytes Supplies the checked, addressable allocation count.
   * @return False on allocation failure; true for zero bytes without allocation.
   * @pre This carrier has no previous allocation.
   */
  bool acquire(uint64 bytes)
  {
    if(bytes == 0)
    {
      return true;
    }
#if SIMPLNX_BUILD_TESTS
    Identity.observe(CodecIoEventForTesting::BoundedRequested, bytes, carrier);
#endif
#if SIMPLNX_BUILD_TESTS
    auto fault = BoundedFaultPointForTesting::NominalAllocation;
    if(carrier == BoundedCarrier::Stored)
    {
      fault = BoundedFaultPointForTesting::StoredAllocation;
    }
    else if(carrier == BoundedCarrier::Selected)
    {
      fault = BoundedFaultPointForTesting::SelectedAllocation;
    }
    if(InjectCodecFaultForTesting(Identity.file, Identity.Dataset, fault))
    {
      return false;
    }
#endif
    try
    {
      data = std::make_unique_for_overwrite<std::byte[]>(static_cast<usize>(bytes));
    } catch(const std::bad_alloc&)
    {
      return false;
    }
    size = bytes;
#if SIMPLNX_BUILD_TESTS
    Identity.observe(CodecIoEventForTesting::BoundedAcquired, size, carrier);
#endif
    return true;
  }
};

static_assert(!std::is_copy_constructible_v<BoundedBytes>);
static_assert(!std::is_copy_assignable_v<BoundedBytes>);
static_assert(!std::is_move_constructible_v<BoundedBytes>);
static_assert(!std::is_move_assignable_v<BoundedBytes>);

/** @brief Holds constant-rank metadata for two physical-record passes and scatter. */
struct BoundedChunkReadMetadata
{
  explicit BoundedChunkReadMetadata(const BoundedIdentity& identity)
  : stored{identity, BoundedCarrier::Stored}
  , nominal{identity, BoundedCarrier::Nominal}
  , selected_output{identity, BoundedCarrier::Selected}
  {
  }

  ~BoundedChunkReadMetadata() = default;
  BoundedChunkReadMetadata(const BoundedChunkReadMetadata&) = delete;
  BoundedChunkReadMetadata(BoundedChunkReadMetadata&&) = delete;
  BoundedChunkReadMetadata& operator=(const BoundedChunkReadMetadata&) = delete;
  BoundedChunkReadMetadata& operator=(BoundedChunkReadMetadata&&) = delete;

  BoundedBytes stored;
  BoundedBytes nominal;
  BoundedBytes selected_output;
  std::array<hsize_t, H5S_MAX_RANK> shape{};
  std::array<hsize_t, H5S_MAX_RANK> chunks{};
  std::array<hsize_t, H5S_MAX_RANK> Origin{};
  std::array<hsize_t, H5S_MAX_RANK> first{};
  std::array<hsize_t, H5S_MAX_RANK> last{};
  std::array<hsize_t, H5S_MAX_RANK> counts{};
  std::array<uint64, H5S_MAX_RANK> nominal_strides{};
  std::array<uint64, H5S_MAX_RANK> output_strides{};
  std::array<uint64, H5S_MAX_RANK> low{};
  std::array<uint64, H5S_MAX_RANK> high{};
  std::array<uint64, H5S_MAX_RANK> cursor{};
  std::array<uint64, H5S_MAX_RANK> selected{};
  ParallelChunkCodec::ChunkInfo Record;
  z_stream stream{};
  DecoderQuota quota;
  std::array<std::byte, 16> fill{};
  uint64 nominal_bytes = 0;
  uint64 maximum_stored = 0;
  usize rank = 0;
  bool has_deflate = false;
  bool needs_decoder = false;
  bool has_sparse = false;
};
static_assert(H5S_MAX_RANK <= BoundedRead::k_MaxRank);

/**
 * @brief Reads one record without treating a failed query as sparse storage.
 * @param dataset Supplies the open dataset identifier.
 * @param origin Supplies an aligned full-rank chunk origin.
 * @param record Receives checked allocation, length and filter state.
 * @param diagnostic Receives fixed query-failure context.
 * @param identity Borrows the scoped test identity.
 * @return Complete or a metadata failure.
 * @pre The caller holds the HDF5 API lock.
 */
Status QueryRecord(hid_t dataset, const hsize_t* origin, ParallelChunkCodec::ChunkInfo& record, Result<>& diagnostic, const BoundedIdentity& identity)
{
  haddr_t address = HADDR_UNDEF;
  hsize_t bytes = 0;
  unsigned mask = 0;
  if(H5Dget_chunk_info_by_coord(dataset, origin, &mask, &address, &bytes) < 0)
  {
    diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk metadata query failed."));
    return Status::Failed;
  }
#if SIMPLNX_BUILD_TESTS
  if(InjectCodecFaultForTesting(identity.file, identity.Dataset, BoundedFaultPointForTesting::ContradictoryRecord))
  {
    address = HADDR_UNDEF;
    bytes = 1;
  }
#else
  static_cast<void>(identity);
#endif
  if((address == HADDR_UNDEF) != (bytes == 0))
  {
    diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk metadata has contradictory address and length."));
    return Status::Failed;
  }
  record = {.allocated = address != HADDR_UNDEF, .filterMask = mask, .storedAddress = static_cast<uint64>(address), .storedSize = static_cast<uint64>(bytes)};
  return Status::Complete;
}

/**
 * @brief Advances to the next physical chunk that contains a selected coordinate.
 * @param metadata Owns the fixed-rank cursor and checked dimensions.
 * @param extent Supplies the original strided selection.
 * @return False after the last selected chunk; the cursor then resets.
 */
bool NextChunk(BoundedChunkReadMetadata& metadata, const Extent& extent) noexcept
{
  for(usize reverse = metadata.rank; reverse != 0; --reverse)
  {
    const usize dimension = reverse - 1;
    const uint64 end = metadata.Origin[dimension] + std::min(metadata.chunks[dimension], metadata.shape[dimension] - metadata.Origin[dimension]) - 1;
    const uint64 nextSelected = ((end - extent.min[dimension]) / extent.stride[dimension]) + 1;
    if(nextSelected < metadata.counts[dimension])
    {
      const uint64 coordinate = extent.min[dimension] + (nextSelected * extent.stride[dimension]);
      metadata.Origin[dimension] = (coordinate / metadata.chunks[dimension]) * metadata.chunks[dimension];
      return true;
    }
    metadata.Origin[dimension] = metadata.first[dimension];
  }
  return false;
}

/**
 * @brief Intersects one physical chunk with the strided selection.
 * @param metadata Receives selected-index bounds and the scatter cursor.
 * @param extent Supplies tuple and component coordinates.
 * @return True when the chunk contains a selected scalar.
 */
bool Intersect(BoundedChunkReadMetadata& metadata, const Extent& extent) noexcept
{
  for(usize dimension = 0; dimension < metadata.rank; ++dimension)
  {
    const uint64 chunkEnd = metadata.Origin[dimension] + std::min(metadata.chunks[dimension], metadata.shape[dimension] - metadata.Origin[dimension]) - 1;
    const uint64 difference = metadata.Origin[dimension] > extent.min[dimension] ? metadata.Origin[dimension] - extent.min[dimension] : 0;
    metadata.low[dimension] = difference / extent.stride[dimension] + (difference % extent.stride[dimension] != 0 ? 1 : 0);
    if(chunkEnd < extent.min[dimension])
    {
      return false;
    }
    metadata.high[dimension] = (std::min(chunkEnd, extent.max[dimension]) - extent.min[dimension]) / extent.stride[dimension];
    if(metadata.low[dimension] > metadata.high[dimension])
    {
      return false;
    }
    metadata.cursor[dimension] = metadata.low[dimension];
  }
  return true;
}

/**
 * @brief Validates the bounded raw representation.
 * @param metadata Supplies checked layout and nominal bytes.
 * @param record Supplies current allocation, filter and stored-byte state.
 * @param diagnostic Receives fixed validation context.
 * @return Complete or an invalid metadata/length result.
 * @note The ordinary decoder retains its separate length and trailing-byte policy.
 */
Status ValidateRecord(const BoundedChunkReadMetadata& metadata, const ParallelChunkCodec::ChunkInfo& record, Result<>& diagnostic)
{
  if(!record.allocated)
  {
    return Status::Complete;
  }
  if((record.filterMask & ~(metadata.has_deflate ? 1U : 0U)) != 0)
  {
    diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk has a filter mask outside its declared pipeline."));
    return Status::Failed;
  }
  const bool compressed = metadata.has_deflate && record.filterMask == 0;
  if(!compressed && record.storedSize != metadata.nominal_bytes)
  {
    diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure,
                                   fmt::format("Bounded raw chunk byte length does not match nominal storage.; required={}, available={}", metadata.nominal_bytes, record.storedSize));
    return Status::Failed;
  }
  if(record.storedSize > std::numeric_limits<usize>::max() || record.storedSize > static_cast<uint64>(std::numeric_limits<std::ptrdiff_t>::max()) ||
     (compressed && (record.storedSize > std::numeric_limits<uInt>::max() || metadata.nominal_bytes > std::numeric_limits<uInt>::max())))
  {
    diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Bounded chunk exceeds the supported raw or zlib length."));
    return Status::Failed;
  }
  return Status::Complete;
}

/** @brief Stores the cursor and contiguous-run bounds for byte scattering. */
struct BoundedScatterControl
{
  uint64 run_values = 1;
  uint64 joined_values = 0;
  uint64 source_index = 0;
  uint64 output_index = 0;
  usize first_run_dimension = 0;
  usize dimension = 0;
  usize reverse = 0;
};

/**
 * @brief Copies selected bytes with runs that are contiguous in both source and destination.
 * @param metadata Supplies checked nominal strides, intersection bounds and scalar fill bytes.
 * @param extent Supplies source coordinates and selection strides.
 * @param elementSize Supplies the validated scalar width.
 * @param source Borrows nominal bytes when sparse is false.
 * @param sparse Selects the checked scalar fill value instead of source.
 * @param destination Receives selected bytes at their row-major output positions.
 */
void Scatter(BoundedChunkReadMetadata& metadata, const Extent& extent, usize elementSize, const std::byte* source, bool sparse, std::byte* destination) noexcept
{
  BoundedScatterControl control;
  control.first_run_dimension = metadata.rank;
  if(!sparse && extent.stride[metadata.rank - 1] == 1)
  {
    control.first_run_dimension = metadata.rank - 1;
    control.run_values = metadata.high[control.first_run_dimension] - metadata.low[control.first_run_dimension] + 1;
    while(control.first_run_dimension != 0)
    {
      control.dimension = control.first_run_dimension - 1;
      // Nominal strides retain edge padding. Output strides retain gaps from split components and partial rows.
      if(extent.stride[control.dimension] != 1 || metadata.nominal_strides[control.dimension] != control.run_values || metadata.output_strides[control.dimension] != control.run_values ||
         !BoundedRead::Multiply(control.run_values, metadata.high[control.dimension] - metadata.low[control.dimension] + 1, control.joined_values))
      {
        break;
      }
      control.run_values = control.joined_values;
      --control.first_run_dimension;
    }
  }
  // Each run stays within checked nominal/output products, so its scalar-to-byte conversion fits usize.
  // Sparse fills remain scalar copies from the fixed primitive fill buffer.
  for(;;)
  {
    control.source_index = 0;
    control.output_index = 0;
    for(control.dimension = 0; control.dimension < metadata.rank; ++control.dimension)
    {
      control.source_index +=
          (extent.min[control.dimension] + metadata.cursor[control.dimension] * extent.stride[control.dimension] - metadata.Origin[control.dimension]) * metadata.nominal_strides[control.dimension];
      control.output_index += metadata.cursor[control.dimension] * metadata.output_strides[control.dimension];
    }
    std::memcpy(destination + (static_cast<usize>(control.output_index) * elementSize), sparse ? metadata.fill.data() : source + (static_cast<usize>(control.source_index) * elementSize),
                static_cast<usize>(control.run_values) * elementSize);
    control.reverse = control.first_run_dimension;
    while(control.reverse != 0)
    {
      control.dimension = control.reverse - 1;
      if(metadata.cursor[control.dimension] < metadata.high[control.dimension])
      {
        ++metadata.cursor[control.dimension];
        break;
      }
      metadata.cursor[control.dimension] = metadata.low[control.dimension];
      --control.reverse;
    }
    if(control.reverse == 0)
    {
      return;
    }
  }
}
} // namespace

Result<ParallelChunkCodec::ChunkInfo> ParallelChunkCodec::getChunkInfoChecked(nonstd::span<const uint64> fullRankChunkOffset) const
{
#if SIMPLNX_BUILD_TESTS
  const MetadataIdentityForTesting metadataIdentity(m_FilePath, m_DatasetPath);
#endif
  Result<> diagnostic;
  ChunkInfo info;
  Status status = Status::Complete;
  std::array<hsize_t, H5S_MAX_RANK> offset{};
  std::array<hsize_t, H5S_MAX_RANK> dimensions{};
  std::array<hsize_t, H5S_MAX_RANK> chunks{};
  if(fullRankChunkOffset.empty() || fullRankChunkOffset.size() > offset.size())
  {
    diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Checked chunk origin has an invalid rank."));
    status = Status::Failed;
  }
  else
  {
    const std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t space = SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Dget_space(m_DatasetId));
    hid_t properties = SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Dget_create_plist(m_DatasetId));
    auto closeMetadata = MakeScopeGuard([&space, &properties]() noexcept {
      if(space >= 0)
      {
        H5Sclose(space);
      }
      if(properties >= 0)
      {
        H5Pclose(properties);
      }
    });
    if(space < 0 || properties < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Checked chunk query cannot obtain dataset metadata."));
      status = Status::Failed;
    }
    else
    {
      const int rank = SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Sget_simple_extent_ndims(space));
      if(rank < 0)
      {
        diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Checked chunk query cannot obtain the dataset rank."));
        status = Status::Failed;
      }
      else if(rank != static_cast<int>(fullRankChunkOffset.size()))
      {
        diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Checked chunk origin does not match the dataset rank."));
        status = Status::Failed;
      }
      else if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Sget_simple_extent_dims(space, dimensions.data(), nullptr)) != rank ||
              SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Pget_chunk(properties, rank, chunks.data())) != rank)
      {
        diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Checked chunk query cannot obtain dimension and chunk sizes."));
        status = Status::Failed;
      }
      else
      {
        for(usize dimension = 0; dimension < fullRankChunkOffset.size(); ++dimension)
        {
          if(chunks[dimension] == 0 || fullRankChunkOffset[dimension] >= dimensions[dimension] || fullRankChunkOffset[dimension] % chunks[dimension] != 0)
          {
            diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Checked chunk origin is outside or not aligned to the physical chunk grid."));
            status = Status::Failed;
            break;
          }
          offset[dimension] = fullRankChunkOffset[dimension];
        }
        if(status == Status::Complete)
        {
#if SIMPLNX_BUILD_TESTS
          ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
          status = QueryRecord(m_DatasetId, offset.data(), info, diagnostic, BoundedIdentity{.file = m_FilePath, .Dataset = m_DatasetPath});
        }
      }
    }
    const herr_t spaceClose = space >= 0 ? H5Sclose(space) : 0;
    space = H5I_INVALID_HID;
    const herr_t propertiesClose = properties >= 0 ? H5Pclose(properties) : 0;
    properties = H5I_INVALID_HID;
    if(status == Status::Complete && (spaceClose < 0 || propertiesClose < 0))
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Checked chunk metadata cleanup failed."));
      status = Status::Failed;
    }
  }
  if(status != Status::Complete)
  {
    diagnostic.errors().front().message += BoundedRead::FormatSourceContext(m_FilePath, m_DatasetPath);
    return ConvertInvalidResult<ChunkInfo>(std::move(diagnostic));
  }
  return {info};
}

Result<bool> ParallelChunkCodec::readExtentIntoBufferBounded(const Extent& extent, nonstd::span<std::byte> destination, uint64 scratchBudgetBytes) const
{
#if SIMPLNX_BUILD_TESTS
  const BoundedReadTaskScopeForTesting taskObservation;
#endif
  std::unique_ptr<std::byte[]> selectedOutput;
  auto releaseOutput = MakeScopeGuard([&]() noexcept {
    if(selectedOutput != nullptr)
    {
      selectedOutput.reset();
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::BoundedReleased, destination.size(), static_cast<uint64>(BoundedCarrier::Selected));
#endif
    }
  });
  Result<> diagnostic;
  Status status = Status::Failed;
  try
  {
    status = readExtentIntoBufferBoundedImpl(extent, destination, scratchBudgetBytes, diagnostic, &selectedOutput);
  } catch(const std::bad_alloc&)
  {
    diagnostic = MakeErrorResult<>(BoundedRead::AllocationFailure, fmt::format("Bounded read could not allocate its data scratch or metadata."));
    status = Status::Failed;
  }
  if(status != Status::Complete)
  {
    if(status == Status::Failed)
    {
      diagnostic.errors().front().message += BoundedRead::FormatSourceContext(m_FilePath, m_DatasetPath);
    }
    else
    {
      diagnostic.warnings().front().message += BoundedRead::FormatSourceContext(m_FilePath, m_DatasetPath);
    }
    return ConvertResultTo(std::move(diagnostic), false);
  }
  const int exceptions = std::uncaught_exceptions();
  auto publish = MakeScopeGuard([&]() noexcept {
    if(std::uncaught_exceptions() == exceptions)
    {
      std::memcpy(destination.data(), selectedOutput.get(), destination.size());
    }
  });
  // The stage remains owned and observed until the actual result construction and publication finish.
#if SIMPLNX_BUILD_TESTS
  return Result<bool>{true, BoundedRead::MakeSuccessWarningsForTesting(&m_FilePath, m_DatasetPath)};
#else
  return Result<bool>{true};
#endif
}

BoundedRead::Status ParallelChunkCodec::readExtentIntoBufferBoundedState(const Extent& extent, nonstd::span<std::byte> destination, uint64 payloadBudgetBytes, Result<>& diagnostic) const
{
#if SIMPLNX_BUILD_TESTS
  const BoundedReadTaskScopeForTesting taskObservation;
#endif
  try
  {
    return readExtentIntoBufferBoundedImpl(extent, destination, payloadBudgetBytes, diagnostic, nullptr);
  } catch(const std::bad_alloc&)
  {
    diagnostic = MakeErrorResult<>(BoundedRead::AllocationFailure, fmt::format("Bounded read could not allocate its data scratch or metadata."));
    return Status::Failed;
  }
}

BoundedRead::Status ParallelChunkCodec::readExtentIntoBufferBoundedImpl(const Extent& extent, nonstd::span<std::byte> destination, uint64 payloadBudgetBytes, Result<>& diagnostic,
                                                                        std::unique_ptr<std::byte[]>* selectedOutput) const
{
  const BoundedIdentity identity{.file = m_FilePath, .Dataset = m_DatasetPath};
#if SIMPLNX_BUILD_TESTS
  const MetadataIdentityForTesting metadataIdentity(m_FilePath, m_DatasetPath);
#endif
  BoundedChunkReadMetadata metadata(identity);
  metadata.quota.Identity = &identity;
  hid_t properties = H5I_INVALID_HID;
  auto closeProperties = MakeScopeGuard([&properties]() noexcept {
    if(properties >= 0)
    {
      const std::lock_guard<std::mutex> lock(Support::ApiLock());
      H5Pclose(properties);
    }
  });
  {
    const std::lock_guard<std::mutex> lock(Support::ApiLock());
    if(m_BoundedTypeQueryFailed)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded native type capture failed."));
      return Status::Failed;
    }
    if(m_BoundedNativeType < 0)
    {
      diagnostic = MakeWarningVoidResult(BoundedRead::Unsupported, fmt::format("Bounded chunk reads require an exact native numeric memory type."));
      return Status::Unavailable;
    }
    usize nativeSize = H5Tget_size(m_BoundedNativeType);
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::NativeSizeQuery))
    {
      nativeSize = 0;
    }
#endif
    if(nativeSize == 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded native scalar-size query failed."));
      return Status::Failed;
    }
    if(m_ElementSize == 0 || nativeSize != m_ElementSize)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Bounded native type size does not match scalar bytes.; required={}, available={}", nativeSize, m_ElementSize));
      return Status::Failed;
    }
    const hid_t fileType = SIMPLNX_OBSERVE_METADATA_CALL(MetadataTypeCall, H5Dget_type(m_DatasetId));
    if(fileType < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk type query failed."));
      return Status::Failed;
    }
    const htri_t exact = H5Tequal(fileType, m_BoundedNativeType);
    const herr_t typeClose = H5Tclose(fileType);
    if(typeClose < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk datatype cleanup failed."));
      return Status::Failed;
    }
    if(exact < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk type comparison failed."));
      return Status::Failed;
    }
    if(exact == 0)
    {
      diagnostic = MakeWarningVoidResult(BoundedRead::Unsupported, fmt::format("Bounded chunk read requires datatype conversion."));
      return Status::Unavailable;
    }
    properties = SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Dget_create_plist(m_DatasetId));
    if(properties < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk layout query failed."));
      return Status::Failed;
    }
    const H5D_layout_t layout = SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Pget_layout(properties));
    const int external = H5Pget_external_count(properties);
    if(layout == H5D_LAYOUT_ERROR || external < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk layout or external-storage query failed."));
      return Status::Failed;
    }
    if(layout != H5D_CHUNKED || external != 0)
    {
      diagnostic = MakeWarningVoidResult(BoundedRead::Unsupported, fmt::format("Bounded codec requires internal chunked storage."));
      return Status::Unavailable;
    }
    const int filters = SIMPLNX_OBSERVE_METADATA_CALL(MetadataFilterCall, H5Pget_nfilters(properties));
    if(filters < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk filter count query failed."));
      return Status::Failed;
    }
    if(filters > 1)
    {
      diagnostic =
          MakeWarningVoidResult(BoundedRead::UnsupportedFilter, fmt::format("Bounded chunk pipeline exceeds the supported filter count.; required={}, available={}", static_cast<uint64>(filters), 1));
      return Status::Unavailable;
    }
    if(filters == 1)
    {
      unsigned flags = 0;
      unsigned configuration = 0;
      std::array<unsigned, 4> values{};
      size_t count = values.size();
      const H5Z_filter_t filter = SIMPLNX_OBSERVE_METADATA_CALL(MetadataFilterCall, H5Pget_filter2(properties, 0, &flags, &count, values.data(), 0, nullptr, &configuration));
      if(filter == H5Z_FILTER_ERROR)
      {
        diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk filter query failed."));
        return Status::Failed;
      }
      if(filter != H5Z_FILTER_DEFLATE || count != 1 || values[0] > 9)
      {
        diagnostic =
            MakeWarningVoidResult(BoundedRead::UnsupportedFilter, fmt::format("Bounded chunk pipeline is not a supported single deflate filter; filter={}, parameter count={}, first parameter={}",
                                                                              static_cast<uint64>(filter), count, values[0]));
        return Status::Unavailable;
      }
      metadata.has_deflate = true;
    }
    const hid_t space = SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Dget_space(m_DatasetId));
    if(space < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk dataspace query failed."));
      return Status::Failed;
    }
    const int rank = H5Sget_simple_extent_ndims(space);
    const int dimensions = rank > 0 && rank <= H5S_MAX_RANK ? H5Sget_simple_extent_dims(space, metadata.shape.data(), nullptr) : -1;
    const herr_t spaceClose = H5Sclose(space);
    if(spaceClose < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk dataspace cleanup failed."));
      return Status::Failed;
    }
    if(rank < 0 || (rank > 0 && rank <= H5S_MAX_RANK && dimensions < 0))
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk rank or dimensions query failed."));
      return Status::Failed;
    }
    if(rank == 0 || rank > H5S_MAX_RANK)
    {
      diagnostic = MakeWarningVoidResult(BoundedRead::Unsupported, fmt::format("Bounded chunk read does not support this rank."));
      return Status::Unavailable;
    }
    metadata.rank = static_cast<usize>(rank);
    if(H5Pget_chunk(properties, rank, metadata.chunks.data()) != rank)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded physical chunk shape query failed."));
      return Status::Failed;
    }
  }
  uint64 selectedValues = 0;
  if(destination.size() % m_ElementSize != 0)
  {
    diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Bounded byte destination is not a scalar multiple."));
    return Status::Failed;
  }
  const auto extentStatus =
      BoundedRead::ValidateExtent(extent, nonstd::span<const hsize_t>(metadata.shape.data(), metadata.rank), 1, destination.size() / m_ElementSize, selectedValues, diagnostic, m_ElementSize);
  if(extentStatus != Status::Complete)
  {
    return extentStatus;
  }
  uint64 nominalValues = 1;
  uint64 outputStride = 1;
  for(usize reverse = metadata.rank; reverse != 0; --reverse)
  {
    const usize dimension = reverse - 1;
    if(metadata.chunks[dimension] == 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded physical chunk shape has a zero dimension."));
      return Status::Failed;
    }
    metadata.nominal_strides[dimension] = nominalValues;
    metadata.output_strides[dimension] = outputStride;
    metadata.counts[dimension] = (extent.max[dimension] - extent.min[dimension]) / extent.stride[dimension] + 1;
    if(!BoundedRead::Multiply(nominalValues, metadata.chunks[dimension], nominalValues) || !BoundedRead::Multiply(outputStride, metadata.counts[dimension], outputStride))
    {
      diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Bounded chunk strides overflow uint64."));
      return Status::Failed;
    }
    metadata.first[dimension] = (extent.min[dimension] / metadata.chunks[dimension]) * metadata.chunks[dimension];
    metadata.last[dimension] = (extent.max[dimension] / metadata.chunks[dimension]) * metadata.chunks[dimension];
    metadata.Origin[dimension] = metadata.first[dimension];
  }
  if(!BoundedRead::Multiply(nominalValues, m_ElementSize, metadata.nominal_bytes) || metadata.nominal_bytes > static_cast<uint64>(std::numeric_limits<std::ptrdiff_t>::max()))
  {
    diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Bounded nominal chunk bytes overflow addressable storage."));
    return Status::Failed;
  }
  // Pass one admits every touched record under the existing serial-alias precondition.
  do
  {
    if(!Intersect(metadata, extent))
    {
      continue;
    }
    {
      const std::lock_guard<std::mutex> lock(Support::ApiLock());
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::MetadataQuery))
      {
        diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Injected bounded metadata query failure."));
        return Status::Failed;
      }
#endif
      const auto status = QueryRecord(m_DatasetId, metadata.Origin.data(), metadata.Record, diagnostic, identity);
      if(status != Status::Complete)
      {
        return status;
      }
    }
    const auto recordStatus = ValidateRecord(metadata, metadata.Record, diagnostic);
    if(recordStatus != Status::Complete)
    {
      return recordStatus;
    }
    if(metadata.Record.allocated)
    {
      metadata.maximum_stored = std::max(metadata.maximum_stored, metadata.Record.storedSize);
      metadata.needs_decoder = metadata.needs_decoder || (metadata.has_deflate && metadata.Record.filterMask == 0);
    }
    else
    {
      metadata.has_sparse = true;
    }
  } while(NextChunk(metadata, extent));
  if(metadata.has_sparse)
  {
    const std::lock_guard<std::mutex> lock(Support::ApiLock());
    H5D_fill_value_t fillStatus = H5D_FILL_VALUE_ERROR;
    if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataFillCall, H5Pfill_value_defined(properties, &fillStatus)) < 0 || fillStatus == H5D_FILL_VALUE_ERROR)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded fill-value status query failed."));
      return Status::Failed;
    }
    if(fillStatus == H5D_FILL_VALUE_UNDEFINED)
    {
      diagnostic = MakeWarningVoidResult(BoundedRead::UndefinedFill, fmt::format("Bounded sparse chunk has an undefined fill value."));
      return Status::Unavailable;
    }
    if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataFillCall, H5Pget_fill_value(properties, m_BoundedNativeType, metadata.fill.data())) < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded fill-value conversion failed."));
      return Status::Failed;
    }
  }
  uint64 requiredBytes = 0;
  const uint64 nominalAllocation = metadata.needs_decoder ? metadata.nominal_bytes : 0;
  const uint64 selectedAllocation = selectedOutput != nullptr ? destination.size() : 0;
  const uint64 decoderWorkspace = metadata.needs_decoder ? BoundedRead::k_DefaultDecoderWorkspaceBytes : 0;
  if(selectedAllocation > static_cast<uint64>(std::numeric_limits<std::ptrdiff_t>::max()) || !BoundedRead::Add(metadata.maximum_stored, nominalAllocation, requiredBytes) ||
     !BoundedRead::Add(requiredBytes, selectedAllocation, requiredBytes) || !BoundedRead::Add(requiredBytes, decoderWorkspace, requiredBytes))
  {
    diagnostic = MakeErrorResult<>(BoundedRead::InvalidExtent, fmt::format("Bounded read data scratch byte count overflows addressable storage."));
    return Status::Failed;
  }
  if(requiredBytes > payloadBudgetBytes)
  {
    diagnostic =
        MakeWarningVoidResult(BoundedRead::Insufficient, fmt::format("Bounded read data scratch exceeds the allowance. Request a smaller extent or increase the allowance.; required={}, available={}",
                                                                     requiredBytes, payloadBudgetBytes));
    return Status::Unavailable;
  }
  metadata.quota.allowance = decoderWorkspace;
  metadata.stream.zalloc = &DecoderQuota::Allocate;
  metadata.stream.zfree = &DecoderQuota::Free;
  metadata.stream.opaque = &metadata.quota;
  bool initialized = false;
  auto finishDecoder = MakeScopeGuard([&metadata, &initialized]() noexcept {
    if(initialized)
    {
      inflateEnd(&metadata.stream);
    }
  });
  if(metadata.needs_decoder)
  {
    const int status = inflateInit(&metadata.stream);
    initialized = status == Z_OK;
    if(!initialized)
    {
      diagnostic =
          MakeErrorResult<>(metadata.quota.quota_failed ? BoundedRead::DecoderQuotaFailure : BoundedRead::AllocationFailure, fmt::format("Bounded zlib initialization cannot acquire its workspace."));
      return Status::Failed;
    }
  }
  auto& stored = metadata.stored;
  auto& nominal = metadata.nominal;
  if(!stored.acquire(metadata.maximum_stored) || !nominal.acquire(nominalAllocation) || !metadata.selected_output.acquire(selectedAllocation))
  {
    diagnostic = MakeErrorResult<>(BoundedRead::AllocationFailure, fmt::format("Bounded chunk carrier allocation failed within its allowance."));
    return Status::Failed;
  }
#ifndef _WIN32
  // Borrow the POSIX native string; opening this independent descriptor allocates no C++ path carrier.
  auto readHandle = metadata.maximum_stored == 0 ? detail::invalidFileHandle() : detail::openFileForRead(m_FilePath.native());
  auto closeReadHandle = MakeScopeGuard([&readHandle]() noexcept {
    if(detail::isValidFileHandle(readHandle))
    {
      detail::closeFileHandle(readHandle);
    }
  });
  if(metadata.maximum_stored != 0 && !detail::isValidFileHandle(readHandle))
  {
    diagnostic = MakeErrorResult<>(BoundedRead::ReadFailure, fmt::format("Bounded positional source could not open."));
    return Status::Failed;
  }
#endif
  std::byte* const outputPtr = selectedOutput != nullptr ? metadata.selected_output.data.get() : destination.data();
  // Pass two checks current records against admitted limits. This is not a cross-alias snapshot.
  do
  {
    if(!Intersect(metadata, extent))
    {
      continue;
    }
    {
      const std::lock_guard<std::mutex> lock(Support::ApiLock());
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::MetadataQuery))
      {
        diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Injected bounded metadata query failure."));
        return Status::Failed;
      }
#endif
      const auto status = QueryRecord(m_DatasetId, metadata.Origin.data(), metadata.Record, diagnostic, identity);
      if(status != Status::Complete)
      {
        return status;
      }
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::SecondPassUnpreparedDecoder))
      {
        metadata.Record.filterMask = 0;
      }
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::SecondPassLargerRecord))
      {
        metadata.Record.storedSize = metadata.maximum_stored + 1;
      }
#endif
      if(metadata.Record.storedSize > metadata.maximum_stored || (metadata.Record.allocated && metadata.has_deflate && metadata.Record.filterMask == 0 && !initialized) ||
         (!metadata.Record.allocated && !metadata.has_sparse))
      {
        diagnostic = MakeErrorResult<>(
            BoundedRead::MetadataChanged,
            fmt::format("Bounded second-pass record requires unadmitted stored bytes or decoder/fill state.; required={}, available={}, filter mask={}, decoder prepared={}, fill prepared={}",
                        metadata.Record.storedSize, metadata.maximum_stored, metadata.Record.filterMask, initialized ? 1 : 0, metadata.has_sparse ? 1 : 0));
        return Status::Failed;
      }
      const auto recordStatus = ValidateRecord(metadata, metadata.Record, diagnostic);
      if(recordStatus != Status::Complete)
      {
        return recordStatus;
      }
#ifdef _WIN32
      if(metadata.Record.allocated)
      {
        unsigned ignoredMask = 0;
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredReadAttempt, metadata.Record.storedSize, 0);
#endif
        // H5Dread_chunk has no capacity argument. Query, length check, and read share this lock.
        bool injectedFailure = false;
#if SIMPLNX_BUILD_TESTS
        injectedFailure = InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::RawRead);
#endif
        if(injectedFailure || H5Dread_chunk(m_DatasetId, H5P_DEFAULT, metadata.Origin.data(), &ignoredMask, stored.data.get()) < 0)
        {
#if SIMPLNX_BUILD_TESTS
          ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredReadFailed, metadata.Record.storedSize, 0);
#endif
          diagnostic = MakeErrorResult<>(BoundedRead::ReadFailure, fmt::format("Bounded raw HDF5 chunk read failed."));
          return Status::Failed;
        }
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredRead, metadata.Record.storedSize, 0);
#endif
      }
#endif
    }
    if(!metadata.Record.allocated)
    {
      Scatter(metadata, extent, m_ElementSize, nullptr, true, outputPtr);
      continue;
    }
#ifndef _WIN32
    auto readStored = [&]() {
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredReadAttempt, metadata.Record.storedSize, 0);
#endif
      bool injectedFailure = false;
#if SIMPLNX_BUILD_TESTS
      injectedFailure = InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::RawRead);
#endif
      const auto got = injectedFailure ? std::ptrdiff_t{-1} : detail::positionalRead(readHandle, stored.data.get(), static_cast<usize>(metadata.Record.storedSize), metadata.Record.storedAddress);
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath,
                               got == static_cast<std::ptrdiff_t>(metadata.Record.storedSize) ? CodecIoEventForTesting::StoredRead :
                               got < 0                                                        ? CodecIoEventForTesting::StoredReadFailed :
                                                                                                CodecIoEventForTesting::StoredReadShort,
                               got >= 0 ? static_cast<uint64>(got) : metadata.Record.storedSize, metadata.Record.storedSize);
#endif
      return got;
    };
    auto got = readStored();
    if(got != static_cast<std::ptrdiff_t>(metadata.Record.storedSize))
    {
      std::lock_guard<std::mutex> recoveryLock(m_PositionalReadRecoveryMutex);
      got = readStored();
      if(got != static_cast<std::ptrdiff_t>(metadata.Record.storedSize))
      {
        {
          const std::lock_guard<std::mutex> lock(Support::ApiLock());
          if(H5Fflush(m_DatasetId, H5F_SCOPE_LOCAL) < 0)
          {
            diagnostic = MakeErrorResult<>(BoundedRead::ReadFailure, fmt::format("Bounded short-read recovery flush failed."));
            return Status::Failed;
          }
        }
        got = readStored();
      }
    }
    if(got != static_cast<std::ptrdiff_t>(metadata.Record.storedSize))
    {
      diagnostic = MakeErrorResult<>(BoundedRead::ReadFailure, fmt::format("Bounded positional chunk read was short or failed."));
      return Status::Failed;
    }
#endif
    const std::byte* decodedPtr = stored.data.get();
    if(metadata.has_deflate && metadata.Record.filterMask == 0)
    {
      if(inflateReset(&metadata.stream) != Z_OK)
      {
        diagnostic = MakeErrorResult<>(BoundedRead::DecodeFailure, fmt::format("Bounded decoder reset failed."));
        return Status::Failed;
      }
      metadata.stream.next_in = reinterpret_cast<Bytef*>(stored.data.get());
      metadata.stream.avail_in = static_cast<uInt>(metadata.Record.storedSize);
      metadata.stream.next_out = reinterpret_cast<Bytef*>(nominal.data.get());
      metadata.stream.avail_out = static_cast<uInt>(metadata.nominal_bytes);
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::InflateAttempt, metadata.Record.storedSize, metadata.nominal_bytes);
#endif
      int status = Z_OK;
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::DecoderQuotaAfterPayload))
      {
        // Withhold the last checksum byte to exercise zlib's real window request.
        // This deterministic fault changes only the admitted test allocator limit.
        metadata.quota.allowance = metadata.quota.live;
        --metadata.stream.avail_in;
        status = inflate(&metadata.stream, Z_NO_FLUSH);
      }
      else
#endif
      {
        status = inflate(&metadata.stream, Z_FINISH);
      }
      if(status != Z_STREAM_END || metadata.stream.total_out != metadata.nominal_bytes || metadata.stream.avail_in != 0)
      {
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::InflateFailed, metadata.Record.storedSize, metadata.stream.total_out);
#endif
        int32 code = BoundedRead::DecodeFailure;
        if(metadata.quota.quota_failed)
        {
          code = BoundedRead::DecoderQuotaFailure;
        }
        else if(metadata.quota.allocation_failed)
        {
          code = BoundedRead::AllocationFailure;
        }
        diagnostic = MakeErrorResult<>(code, fmt::format("Bounded decoder did not consume one complete nominal stream."));
        return Status::Failed;
      }
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::Inflated, metadata.Record.storedSize, metadata.stream.total_out);
#endif
      decodedPtr = nominal.data.get();
    }
    else if(metadata.has_deflate)
    {
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::SkippedDeflate, metadata.Record.storedSize, metadata.nominal_bytes);
#endif
    }
    Scatter(metadata, extent, m_ElementSize, decodedPtr, false, outputPtr);
  } while(NextChunk(metadata, extent));
  // Complete fallible cleanup before publishing any selected byte.
  if(initialized)
  {
    initialized = false;
    int status = inflateEnd(&metadata.stream);
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::DecoderCleanup))
    {
      status = Z_DATA_ERROR;
    }
#endif
    if(status != Z_OK)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::DecodeFailure, fmt::format("Bounded decoder cleanup failed."));
      return Status::Failed;
    }
  }
  {
    const std::lock_guard<std::mutex> lock(Support::ApiLock());
    int status = H5Pclose(properties);
    properties = H5I_INVALID_HID;
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::MetadataCleanup))
    {
      status = -1;
    }
#endif
    if(status < 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::MetadataFailure, fmt::format("Bounded chunk metadata cleanup failed."));
      return Status::Failed;
    }
  }
#ifndef _WIN32
  if(detail::isValidFileHandle(readHandle))
  {
    const int status = ::close(readHandle);
    readHandle = detail::invalidFileHandle();
    if(status != 0)
    {
      diagnostic = MakeErrorResult<>(BoundedRead::ReadFailure, fmt::format("Bounded positional source cleanup failed."));
      return Status::Failed;
    }
  }
#endif
  if(selectedOutput != nullptr)
  {
    // Transfer ownership without a release event. The public boundary releases observation after actual destruction.
    *selectedOutput = std::move(metadata.selected_output.data);
  }
  return Status::Complete;
}
} // namespace nx::core::HDF5
