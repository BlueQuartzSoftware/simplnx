#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"

#include <H5Dpublic.h>
#include <H5Fpublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>
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
using BoundedRead::Diagnostic;
using BoundedRead::Status;

/** @brief Labels the actual owned storage seen by the existing test observer. */
enum class BoundedCarrier : uint64
{
  Metadata,
  Stored,
  Nominal,
  Decoder
};

/** @brief Carries allocation identity without copying a path or dataset. */
struct BoundedIdentity
{
  const std::filesystem::path& file;
  const std::string& dataset;

#if SIMPLNX_BUILD_TESTS
  void observe(CodecIoEventForTesting event, uint64 bytes, BoundedCarrier carrier) const noexcept
  {
    ObserveCodecIoForTesting(file, dataset, event, bytes, static_cast<uint64>(carrier));
  }
#endif
};

/** @brief Stores each zlib allocation's admitted size in its own aligned header. */
struct DecoderHeader
{
  alignas(std::max_align_t) uint64 bytes = 0;
};

/** @brief Bounds zlib-owned storage; this state outlives inflateEnd and every zfree. */
struct DecoderQuota
{
  const BoundedIdentity* identity = nullptr;
  uint64 allowance = 0;
  uint64 live = 0;
  bool quotaFailed = false;
  bool allocationFailed = false;

  /**
   * @brief Services a zlib request only after charging its payload and aligned header.
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
    if(!BoundedRead::Multiply(count, size, payload) || !BoundedRead::Add(payload, sizeof(DecoderHeader), bytes) || bytes > quota.allowance - quota.live || bytes > std::numeric_limits<usize>::max())
    {
      quota.quotaFailed = true;
      return nullptr;
    }
#if SIMPLNX_BUILD_TESTS
    quota.identity->observe(CodecIoEventForTesting::BoundedRequested, bytes, BoundedCarrier::Decoder);
#endif
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(quota.identity->file, quota.identity->dataset, BoundedFaultPointForTesting::DecoderAllocation))
    {
      quota.allocationFailed = true;
      return nullptr;
    }
#endif
    auto* header = static_cast<DecoderHeader*>(std::malloc(static_cast<usize>(bytes)));
    if(header == nullptr)
    {
      quota.allocationFailed = true;
      return nullptr;
    }
    header->bytes = bytes;
    quota.live += bytes;
#if SIMPLNX_BUILD_TESTS
    quota.identity->observe(CodecIoEventForTesting::BoundedAcquired, bytes, BoundedCarrier::Decoder);
#endif
    return header + 1;
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
    auto* header = static_cast<DecoderHeader*>(address) - 1;
    const uint64 bytes = header->bytes;
    std::free(header);
    quota.live -= bytes;
#if SIMPLNX_BUILD_TESTS
    quota.identity->observe(CodecIoEventForTesting::BoundedReleased, bytes, BoundedCarrier::Decoder);
#endif
  }
};

/** @brief Owns one exact requested byte allocation, without vector growth. */
struct BoundedBytes
{
  const BoundedIdentity& identity;
  BoundedCarrier carrier;
  std::byte* data = nullptr;
  uint64 size = 0;

  ~BoundedBytes() noexcept
  {
    if(data != nullptr)
    {
      ::operator delete(data);
#if SIMPLNX_BUILD_TESTS
      identity.observe(CodecIoEventForTesting::BoundedReleased, size, carrier);
#endif
    }
  }

  /**
   * @brief Requests the exact byte count after the caller has admitted it.
   * @param bytes Supplies the checked, addressable allocation count.
   * @return False on allocation failure; true for zero bytes without allocation.
   * @pre This carrier has no previous allocation.
   */
  bool acquire(uint64 bytes) noexcept
  {
    if(bytes == 0)
    {
      return true;
    }
#if SIMPLNX_BUILD_TESTS
    identity.observe(CodecIoEventForTesting::BoundedRequested, bytes, carrier);
#endif
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(identity.file, identity.dataset, carrier == BoundedCarrier::Stored ? BoundedFaultPointForTesting::StoredAllocation : BoundedFaultPointForTesting::NominalAllocation))
    {
      return false;
    }
#endif
    data = static_cast<std::byte*>(::operator new(static_cast<usize>(bytes), std::nothrow));
    if(data == nullptr)
    {
      return false;
    }
    size = bytes;
#if SIMPLNX_BUILD_TESTS
    identity.observe(CodecIoEventForTesting::BoundedAcquired, size, carrier);
#endif
    return true;
  }
};

/** @brief Holds constant-rank metadata for two physical-record passes and scatter. */
struct BoundedChunkReadMetadata
{
  explicit BoundedChunkReadMetadata(const BoundedIdentity& identity)
  : stored{identity, BoundedCarrier::Stored}
  , nominal{identity, BoundedCarrier::Nominal}
  {
  }

  BoundedBytes stored;
  BoundedBytes nominal;
  std::array<hsize_t, H5S_MAX_RANK> shape{};
  std::array<hsize_t, H5S_MAX_RANK> chunks{};
  std::array<hsize_t, H5S_MAX_RANK> origin{};
  std::array<hsize_t, H5S_MAX_RANK> first{};
  std::array<hsize_t, H5S_MAX_RANK> last{};
  std::array<hsize_t, H5S_MAX_RANK> counts{};
  std::array<uint64, H5S_MAX_RANK> nominalStrides{};
  std::array<uint64, H5S_MAX_RANK> outputStrides{};
  std::array<uint64, H5S_MAX_RANK> low{};
  std::array<uint64, H5S_MAX_RANK> high{};
  std::array<uint64, H5S_MAX_RANK> cursor{};
  std::array<uint64, H5S_MAX_RANK> selected{};
  ParallelChunkCodec::ChunkInfo record;
  z_stream stream{};
  DecoderQuota quota;
  std::array<std::byte, 16> fill{};
  uint64 nominalBytes = 0;
  uint64 maximumStored = 0;
  usize rank = 0;
  bool hasDeflate = false;
  bool needsDecoder = false;
  bool hasSparse = false;
};
static_assert(H5S_MAX_RANK <= BoundedRead::k_MaxRank);

/** @brief Bounds a cleanup guard's two-reference capture and active flag on the compiling ABI. */
struct BoundedCleanupControl
{
  std::array<void*, 2> captures{};
  bool active = false;
};

/** @brief Inventories the fixed native-query carriers that coexist with chunk metadata. */
struct BoundedNativeQueryControls
{
  std::array<hid_t, 3> identifiers{};
  std::array<int, 6> statuses{};
  std::array<unsigned, 6> filterValuesAndFlags{};
  std::array<usize, 2> countsAndSize{};
  H5D_layout_t layout = H5D_LAYOUT_ERROR;
  H5D_fill_value_t fill = H5D_FILL_VALUE_ERROR;
  haddr_t address = HADDR_UNDEF;
  hsize_t storedSize = 0;
  unsigned mask = 0;
};

/** @brief Keeps all scatter loop state in one admitted fixed-size control. */
struct BoundedScatterControl
{
  uint64 runValues = 1;
  uint64 joinedValues = 0;
  uint64 sourceIndex = 0;
  uint64 outputIndex = 0;
  usize firstRunDimension = 0;
  usize dimension = 0;
  usize reverse = 0;
};

// Scatter borrows two references and two byte pointers, plus scalar width and sparse mode.
// Multiply borrows one result reference and accepts two scalar operands.
constexpr uint64 k_BoundedScatterArgumentBytes = 4 * sizeof(void*) + sizeof(usize) + sizeof(bool) + sizeof(void*) + 2 * sizeof(uint64);
constexpr uint64 k_BoundedScatterBytes = sizeof(BoundedScatterControl) + k_BoundedScatterArgumentBytes;

// ScopeGuard has one captured callable and an active flag. The two additional
// capture slots per guard cover constructor arguments and source temporaries.
// Four guards cover metadata release, properties, decoder and the POSIX handle.
constexpr uint64 k_BoundedCleanupBytes = 4 * (sizeof(BoundedCleanupControl) + 2 * sizeof(std::array<void*, 2>));
constexpr uint64 k_BoundedReadClosureBytes = 5 * sizeof(void*);
constexpr uint64 k_BoundedControlBytes = sizeof(BoundedIdentity) + sizeof(hid_t) + sizeof(bool) + sizeof(detail::FileHandle) + sizeof(BoundedNativeQueryControls) +
                                         2 * sizeof(std::lock_guard<std::mutex>) + k_BoundedCleanupBytes + k_BoundedReadClosureBytes + 2 * sizeof(nonstd::span<std::byte>) +
                                         sizeof(nonstd::span<const hsize_t>) + 2 * sizeof(Status) + k_BoundedScatterBytes
#if SIMPLNX_BUILD_TESTS
                                         + sizeof(BoundedReadTaskScopeForTesting) + sizeof(MetadataIdentityForTesting)
#endif
    ;
constexpr uint64 k_BoundedFixedBytes = sizeof(BoundedChunkReadMetadata) + k_BoundedControlBytes;

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
Status queryRecord(hid_t dataset, const hsize_t* origin, ParallelChunkCodec::ChunkInfo& record, Diagnostic& diagnostic, const BoundedIdentity& identity)
{
  haddr_t address = HADDR_UNDEF;
  hsize_t bytes = 0;
  unsigned mask = 0;
  if(H5Dget_chunk_info_by_coord(dataset, origin, &mask, &address, &bytes) < 0)
  {
    return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk metadata query failed.");
  }
#if SIMPLNX_BUILD_TESTS
  if(InjectCodecFaultForTesting(identity.file, identity.dataset, BoundedFaultPointForTesting::ContradictoryRecord))
  {
    address = HADDR_UNDEF;
    bytes = 1;
  }
#else
  static_cast<void>(identity);
#endif
  if((address == HADDR_UNDEF) != (bytes == 0))
  {
    return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk metadata has contradictory address and length.");
  }
  record = {address != HADDR_UNDEF, mask, static_cast<uint64>(address), static_cast<uint64>(bytes)};
  return Status::Complete;
}

/**
 * @brief Advances to the next physical chunk that contains a selected coordinate.
 * @param metadata Owns the fixed-rank cursor and checked dimensions.
 * @param extent Supplies the original strided selection.
 * @return False after the last selected chunk; the cursor then resets.
 */
bool nextChunk(BoundedChunkReadMetadata& metadata, const Extent& extent) noexcept
{
  for(usize reverse = metadata.rank; reverse != 0; --reverse)
  {
    const usize dimension = reverse - 1;
    const uint64 end = metadata.origin[dimension] + std::min(metadata.chunks[dimension], metadata.shape[dimension] - metadata.origin[dimension]) - 1;
    const uint64 nextSelected = (end - extent.min[dimension]) / extent.stride[dimension] + 1;
    if(nextSelected < metadata.counts[dimension])
    {
      const uint64 coordinate = extent.min[dimension] + nextSelected * extent.stride[dimension];
      metadata.origin[dimension] = (coordinate / metadata.chunks[dimension]) * metadata.chunks[dimension];
      return true;
    }
    metadata.origin[dimension] = metadata.first[dimension];
  }
  return false;
}

/**
 * @brief Intersects one physical chunk with the strided selection.
 * @param metadata Receives selected-index bounds and the scatter cursor.
 * @param extent Supplies tuple and component coordinates.
 * @return True when the chunk contains a selected scalar.
 */
bool intersect(BoundedChunkReadMetadata& metadata, const Extent& extent) noexcept
{
  for(usize dimension = 0; dimension < metadata.rank; ++dimension)
  {
    const uint64 chunkEnd = metadata.origin[dimension] + std::min(metadata.chunks[dimension], metadata.shape[dimension] - metadata.origin[dimension]) - 1;
    const uint64 difference = metadata.origin[dimension] > extent.min[dimension] ? metadata.origin[dimension] - extent.min[dimension] : 0;
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
Status validateRecord(const BoundedChunkReadMetadata& metadata, const ParallelChunkCodec::ChunkInfo& record, Diagnostic& diagnostic)
{
  if(!record.allocated)
  {
    return Status::Complete;
  }
  if((record.filterMask & ~(metadata.hasDeflate ? 1U : 0U)) != 0)
  {
    return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk has a filter mask outside its declared pipeline.");
  }
  const bool compressed = metadata.hasDeflate && record.filterMask == 0;
  if(!compressed && record.storedSize != metadata.nominalBytes)
  {
    return diagnostic.counts(BoundedRead::MetadataFailure, "Bounded raw chunk byte length does not match nominal storage.", metadata.nominalBytes, record.storedSize);
  }
  if(record.storedSize > std::numeric_limits<usize>::max() || record.storedSize > static_cast<uint64>(std::numeric_limits<std::ptrdiff_t>::max()) ||
     (compressed && (record.storedSize > std::numeric_limits<uInt>::max() || metadata.nominalBytes > std::numeric_limits<uInt>::max())))
  {
    return diagnostic.set(BoundedRead::InvalidExtent, "Bounded chunk exceeds the supported raw or zlib length.");
  }
  return Status::Complete;
}

/**
 * @brief Copies selected bytes with runs that are contiguous in both source and destination.
 * @param metadata Supplies checked nominal strides, intersection bounds and scalar fill bytes.
 * @param extent Supplies source coordinates and selection strides.
 * @param elementSize Supplies the validated scalar width.
 * @param source Borrows nominal bytes when sparse is false.
 * @param sparse Selects the checked scalar fill value instead of source.
 * @param destination Receives selected bytes at their row-major output positions.
 */
void scatter(BoundedChunkReadMetadata& metadata, const Extent& extent, usize elementSize, const std::byte* source, bool sparse, std::byte* destination) noexcept
{
  BoundedScatterControl control;
  static_assert(sizeof(control) + k_BoundedScatterArgumentBytes <= k_BoundedScatterBytes);
  control.firstRunDimension = metadata.rank;
  if(!sparse && extent.stride[metadata.rank - 1] == 1)
  {
    control.firstRunDimension = metadata.rank - 1;
    control.runValues = metadata.high[control.firstRunDimension] - metadata.low[control.firstRunDimension] + 1;
    while(control.firstRunDimension != 0)
    {
      control.dimension = control.firstRunDimension - 1;
      // Nominal strides retain edge padding. Output strides retain gaps from split components and partial rows.
      if(extent.stride[control.dimension] != 1 || metadata.nominalStrides[control.dimension] != control.runValues || metadata.outputStrides[control.dimension] != control.runValues ||
         !BoundedRead::Multiply(control.runValues, metadata.high[control.dimension] - metadata.low[control.dimension] + 1, control.joinedValues))
      {
        break;
      }
      control.runValues = control.joinedValues;
      --control.firstRunDimension;
    }
  }
  // Each run stays within checked nominal/output products, so its scalar-to-byte conversion fits usize.
  // Sparse fills remain scalar copies from the fixed primitive fill buffer.
  for(;;)
  {
    control.sourceIndex = 0;
    control.outputIndex = 0;
    for(control.dimension = 0; control.dimension < metadata.rank; ++control.dimension)
    {
      control.sourceIndex +=
          (extent.min[control.dimension] + metadata.cursor[control.dimension] * extent.stride[control.dimension] - metadata.origin[control.dimension]) * metadata.nominalStrides[control.dimension];
      control.outputIndex += metadata.cursor[control.dimension] * metadata.outputStrides[control.dimension];
    }
    std::memcpy(destination + static_cast<usize>(control.outputIndex) * elementSize, sparse ? metadata.fill.data() : source + static_cast<usize>(control.sourceIndex) * elementSize,
                static_cast<usize>(control.runValues) * elementSize);
    control.reverse = control.firstRunDimension;
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

#if SIMPLNX_BUILD_TESTS
uint64 ParallelChunkCodec::boundedMetadataCapacityBytesForTesting() const noexcept
{
  return (m_FilePath.native().capacity() + 1) * sizeof(std::filesystem::path::value_type) + m_DatasetPath.capacity() + 1 +
         (m_TupleShape.capacity() + m_ChunkShape.capacity() + m_ComponentShape.capacity()) * sizeof(uint64);
}
#endif

Result<ParallelChunkCodec::ChunkInfo> ParallelChunkCodec::getChunkInfoChecked(nonstd::span<const uint64> fullRankChunkOffset) const
{
  bool representationSupported = BoundedRead::DiagnosticRepresentationSupported();
#if SIMPLNX_BUILD_TESTS
  if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::CheckedMetadataUnsupportedBuild))
  {
    representationSupported = false;
  }
#endif
  if(!representationSupported)
  {
    // Mandatory empty error/warning controls only. No value, element or text is constructed.
    return {{nonstd::make_unexpected(ErrorCollection{})}};
  }
#if SIMPLNX_BUILD_TESTS
  MetadataIdentityForTesting metadataIdentity(m_FilePath, m_DatasetPath);
#endif
  Diagnostic diagnostic;
  ChunkInfo info;
  Status status = Status::Complete;
  std::array<hsize_t, H5S_MAX_RANK> offset{};
  std::array<hsize_t, H5S_MAX_RANK> dimensions{};
  std::array<hsize_t, H5S_MAX_RANK> chunks{};
  if(fullRankChunkOffset.empty() || fullRankChunkOffset.size() > offset.size())
  {
    status = diagnostic.set(BoundedRead::InvalidExtent, "Checked chunk origin has an invalid rank.");
  }
  else
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    const hid_t space = SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Dget_space(m_DatasetId));
    const hid_t properties = SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Dget_create_plist(m_DatasetId));
    auto closeMetadata = MakeScopeGuard([space, properties]() noexcept {
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
      status = diagnostic.set(BoundedRead::MetadataFailure, "Checked chunk query cannot obtain dataset metadata.");
    }
    else
    {
      const int rank = SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Sget_simple_extent_ndims(space));
      if(rank < 0)
      {
        status = diagnostic.set(BoundedRead::MetadataFailure, "Checked chunk query cannot obtain the dataset rank.");
      }
      else if(rank != static_cast<int>(fullRankChunkOffset.size()))
      {
        status = diagnostic.set(BoundedRead::InvalidExtent, "Checked chunk origin does not match the dataset rank.");
      }
      else if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Sget_simple_extent_dims(space, dimensions.data(), nullptr)) != rank ||
              SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Pget_chunk(properties, rank, chunks.data())) != rank)
      {
        status = diagnostic.set(BoundedRead::MetadataFailure, "Checked chunk query cannot obtain dimension and chunk sizes.");
      }
      else
      {
        for(usize dimension = 0; dimension < fullRankChunkOffset.size(); ++dimension)
        {
          if(chunks[dimension] == 0 || fullRankChunkOffset[dimension] >= dimensions[dimension] || fullRankChunkOffset[dimension] % chunks[dimension] != 0)
          {
            status = diagnostic.set(BoundedRead::InvalidExtent, "Checked chunk origin is outside or not aligned to the physical chunk grid.");
            break;
          }
          offset[dimension] = fullRankChunkOffset[dimension];
        }
        if(status == Status::Complete)
        {
#if SIMPLNX_BUILD_TESTS
          ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
          status = queryRecord(m_DatasetId, offset.data(), info, diagnostic, BoundedIdentity{m_FilePath, m_DatasetPath});
        }
      }
    }
  }
  Result<ChunkInfo> result{info};
  if(status != Status::Complete)
  {
    diagnostic.context(m_FilePath, m_DatasetPath);
    auto failure = diagnostic.finish(status);
    result.m_Expected = nonstd::make_unexpected(std::move(failure.errors()));
  }
  return result;
}

Result<bool> ParallelChunkCodec::readExtentIntoBufferBounded(const Extent& extent, nonstd::span<std::byte> destination, uint64 scratchBudgetBytes) const
{
  if(scratchBudgetBytes < BoundedRead::k_DiagnosticBytes || !BoundedRead::DiagnosticRepresentationSupported())
  {
    return {false};
  }
  Diagnostic diagnostic;
  const auto status = readExtentIntoBufferBoundedState(extent, destination, scratchBudgetBytes - BoundedRead::k_DiagnosticBytes, diagnostic);
  if(status != Status::Complete)
  {
    diagnostic.context(m_FilePath, m_DatasetPath);
  }
  return diagnostic.finish(status);
}

BoundedRead::Status ParallelChunkCodec::readExtentIntoBufferBoundedState(const Extent& extent, nonstd::span<std::byte> destination, uint64 payloadBudgetBytes,
                                                                         BoundedRead::Diagnostic& diagnostic) const
{
  if(payloadBudgetBytes < k_BoundedFixedBytes)
  {
    return diagnostic.counts(BoundedRead::Insufficient, "Bounded chunk read cannot admit fixed metadata and controls.", k_BoundedFixedBytes, payloadBudgetBytes);
  }
  const BoundedIdentity identity{m_FilePath, m_DatasetPath};
#if SIMPLNX_BUILD_TESTS
  BoundedReadTaskScopeForTesting taskObservation;
  auto metadataRelease = MakeScopeGuard([&identity]() noexcept { identity.observe(CodecIoEventForTesting::BoundedReleased, sizeof(BoundedChunkReadMetadata), BoundedCarrier::Metadata); });
  identity.observe(CodecIoEventForTesting::BoundedRequested, sizeof(BoundedChunkReadMetadata), BoundedCarrier::Metadata);
#endif
#if SIMPLNX_BUILD_TESTS
  static_assert(sizeof(metadataRelease) <= sizeof(BoundedCleanupControl));
  identity.observe(CodecIoEventForTesting::BoundedAdmissionCharge, k_BoundedControlBytes, BoundedCarrier::Metadata);
#endif
  BoundedChunkReadMetadata metadata(identity);
#if SIMPLNX_BUILD_TESTS
  identity.observe(CodecIoEventForTesting::BoundedAcquired, sizeof(metadata), BoundedCarrier::Metadata);
  MetadataIdentityForTesting metadataIdentity(m_FilePath, m_DatasetPath);
#endif
  const uint64 available = payloadBudgetBytes - k_BoundedFixedBytes;
  metadata.quota.identity = &identity;
  hid_t properties = H5I_INVALID_HID;
  auto closeProperties = MakeScopeGuard([&properties]() noexcept {
    if(properties >= 0)
    {
      std::lock_guard<std::mutex> lock(Support::ApiLock());
      H5Pclose(properties);
    }
  });
  static_assert(sizeof(closeProperties) <= sizeof(BoundedCleanupControl));
  {
    std::lock_guard<std::mutex> lock(Support::ApiLock());
    if(m_BoundedTypeQueryFailed)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded native type capture failed.");
    }
    if(m_BoundedNativeType < 0)
    {
      return diagnostic.set(BoundedRead::Unsupported, "Bounded chunk reads require an exact native numeric memory type.");
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
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded native scalar-size query failed.");
    }
    if(m_ElementSize == 0 || nativeSize != m_ElementSize)
    {
      return diagnostic.counts(BoundedRead::InvalidExtent, "Bounded native type size does not match scalar bytes.", nativeSize, m_ElementSize);
    }
    const hid_t fileType = SIMPLNX_OBSERVE_METADATA_CALL(MetadataTypeCall, H5Dget_type(m_DatasetId));
    if(fileType < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk type query failed.");
    }
    const htri_t exact = H5Tequal(fileType, m_BoundedNativeType);
    H5Tclose(fileType);
    if(exact < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk type comparison failed.");
    }
    if(exact == 0)
    {
      return diagnostic.set(BoundedRead::Unsupported, "Bounded chunk read requires datatype conversion.");
    }
    properties = SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Dget_create_plist(m_DatasetId));
    if(properties < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk layout query failed.");
    }
    const H5D_layout_t layout = SIMPLNX_OBSERVE_METADATA_CALL(MetadataLayoutCall, H5Pget_layout(properties));
    const int external = H5Pget_external_count(properties);
    if(layout == H5D_LAYOUT_ERROR || external < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk layout or external-storage query failed.");
    }
    if(layout != H5D_CHUNKED || external != 0)
    {
      return diagnostic.set(BoundedRead::Unsupported, "Bounded codec requires internal chunked storage.");
    }
    const int filters = SIMPLNX_OBSERVE_METADATA_CALL(MetadataFilterCall, H5Pget_nfilters(properties));
    if(filters < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk filter count query failed.");
    }
    if(filters > 1)
    {
      return diagnostic.counts(BoundedRead::UnsupportedFilter, "Bounded chunk pipeline exceeds the supported filter count.", static_cast<uint64>(filters), 1);
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
        return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk filter query failed.");
      }
      if(filter != H5Z_FILTER_DEFLATE || count != 1 || values[0] > 9)
      {
        diagnostic.set(BoundedRead::UnsupportedFilter, "Bounded chunk pipeline is not a supported single deflate filter; filter=");
        diagnostic.number(static_cast<uint64>(filter));
        diagnostic.append(", parameter count=");
        diagnostic.number(count);
        diagnostic.append(", first parameter=");
        diagnostic.number(values[0]);
        return Status::Unavailable;
      }
      metadata.hasDeflate = true;
    }
    const hid_t space = SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Dget_space(m_DatasetId));
    if(space < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk dataspace query failed.");
    }
    const int rank = H5Sget_simple_extent_ndims(space);
    const int dimensions = rank > 0 && rank <= H5S_MAX_RANK ? H5Sget_simple_extent_dims(space, metadata.shape.data(), nullptr) : -1;
    H5Sclose(space);
    if(rank < 0 || (rank > 0 && rank <= H5S_MAX_RANK && dimensions < 0))
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk rank or dimensions query failed.");
    }
    if(rank == 0 || rank > H5S_MAX_RANK)
    {
      return diagnostic.set(BoundedRead::Unsupported, "Bounded chunk read does not support this rank.");
    }
    metadata.rank = static_cast<usize>(rank);
    if(H5Pget_chunk(properties, rank, metadata.chunks.data()) != rank)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded physical chunk shape query failed.");
    }
  }
  uint64 selectedValues = 0;
  if(destination.size() % m_ElementSize != 0)
  {
    return diagnostic.set(BoundedRead::InvalidExtent, "Bounded byte destination is not a scalar multiple.");
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
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded physical chunk shape has a zero dimension.");
    }
    metadata.nominalStrides[dimension] = nominalValues;
    metadata.outputStrides[dimension] = outputStride;
    metadata.counts[dimension] = (extent.max[dimension] - extent.min[dimension]) / extent.stride[dimension] + 1;
    if(!BoundedRead::Multiply(nominalValues, metadata.chunks[dimension], nominalValues) || !BoundedRead::Multiply(outputStride, metadata.counts[dimension], outputStride))
    {
      return diagnostic.set(BoundedRead::InvalidExtent, "Bounded chunk strides overflow uint64.");
    }
    metadata.first[dimension] = (extent.min[dimension] / metadata.chunks[dimension]) * metadata.chunks[dimension];
    metadata.last[dimension] = (extent.max[dimension] / metadata.chunks[dimension]) * metadata.chunks[dimension];
    metadata.origin[dimension] = metadata.first[dimension];
  }
  if(!BoundedRead::Multiply(nominalValues, m_ElementSize, metadata.nominalBytes) || metadata.nominalBytes > std::numeric_limits<usize>::max())
  {
    return diagnostic.set(BoundedRead::InvalidExtent, "Bounded nominal chunk bytes overflow addressable storage.");
  }
  // Pass one admits every touched record under the existing serial-alias precondition.
  do
  {
    if(!intersect(metadata, extent))
    {
      continue;
    }
    {
      std::lock_guard<std::mutex> lock(Support::ApiLock());
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::MetadataQuery))
      {
        return diagnostic.set(BoundedRead::MetadataFailure, "Injected bounded metadata query failure.");
      }
#endif
      const auto status = queryRecord(m_DatasetId, metadata.origin.data(), metadata.record, diagnostic, identity);
      if(status != Status::Complete)
      {
        return status;
      }
    }
    const auto recordStatus = validateRecord(metadata, metadata.record, diagnostic);
    if(recordStatus != Status::Complete)
    {
      return recordStatus;
    }
    if(metadata.record.allocated)
    {
      metadata.maximumStored = std::max(metadata.maximumStored, metadata.record.storedSize);
      metadata.needsDecoder = metadata.needsDecoder || (metadata.hasDeflate && metadata.record.filterMask == 0);
    }
    else
    {
      metadata.hasSparse = true;
    }
  } while(nextChunk(metadata, extent));
  if(metadata.hasSparse)
  {
    std::lock_guard<std::mutex> lock(Support::ApiLock());
    H5D_fill_value_t fillStatus = H5D_FILL_VALUE_ERROR;
    if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataFillCall, H5Pfill_value_defined(properties, &fillStatus)) < 0 || fillStatus == H5D_FILL_VALUE_ERROR)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded fill-value status query failed.");
    }
    if(fillStatus == H5D_FILL_VALUE_UNDEFINED)
    {
      return diagnostic.set(BoundedRead::UndefinedFill, "Bounded sparse chunk has an undefined fill value.");
    }
    if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataFillCall, H5Pget_fill_value(properties, m_BoundedNativeType, metadata.fill.data())) < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded fill-value conversion failed.");
    }
  }
  uint64 carrierBytes = 0;
  const uint64 nominalAllocation = metadata.needsDecoder ? metadata.nominalBytes : 0;
  if(!BoundedRead::Add(metadata.maximumStored, nominalAllocation, carrierBytes))
  {
    return diagnostic.set(BoundedRead::InvalidExtent, "Bounded raw and nominal carrier byte sum overflows uint64.");
  }
  if(carrierBytes > available)
  {
    return diagnostic.counts(BoundedRead::Insufficient, "Bounded raw and nominal carrier bytes exceed the allowance.", carrierBytes, available);
  }
  metadata.quota.allowance = available - carrierBytes;
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
  static_assert(sizeof(finishDecoder) <= sizeof(BoundedCleanupControl));
  if(metadata.needsDecoder)
  {
    const int status = inflateInit(&metadata.stream);
    initialized = status == Z_OK;
    if(!initialized)
    {
      return diagnostic.set(metadata.quota.quotaFailed ? BoundedRead::Insufficient : BoundedRead::AllocationFailure, "Bounded zlib initialization cannot acquire admitted state.");
    }
    constexpr uint64 windowBytes = 32768 + sizeof(DecoderHeader);
    if(windowBytes > metadata.quota.allowance - metadata.quota.live)
    {
      return diagnostic.counts(BoundedRead::Insufficient, "Bounded zlib state and maximum-window bytes exceed the allowance.", metadata.quota.live + windowBytes, metadata.quota.allowance);
    }
  }
  auto& stored = metadata.stored;
  auto& nominal = metadata.nominal;
  if(!stored.acquire(metadata.maximumStored) || !nominal.acquire(nominalAllocation))
  {
    return diagnostic.set(BoundedRead::AllocationFailure, "Bounded chunk carrier allocation failed within its allowance.");
  }
#ifndef _WIN32
  // Borrow the POSIX native string; opening this independent descriptor allocates no C++ path carrier.
  const auto readHandle = metadata.maximumStored == 0 ? detail::invalidFileHandle() : detail::openFileForRead(m_FilePath.native());
  auto closeReadHandle = MakeScopeGuard([readHandle]() noexcept {
    if(detail::isValidFileHandle(readHandle))
    {
      detail::closeFileHandle(readHandle);
    }
  });
  static_assert(sizeof(closeReadHandle) <= sizeof(BoundedCleanupControl));
  if(metadata.maximumStored != 0 && !detail::isValidFileHandle(readHandle))
  {
    return diagnostic.set(BoundedRead::ReadFailure, "Bounded positional source could not open.");
  }
#endif
  // Pass two checks current records against admitted limits. This is not a cross-alias snapshot.
  do
  {
    if(!intersect(metadata, extent))
    {
      continue;
    }
    {
      std::lock_guard<std::mutex> lock(Support::ApiLock());
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::MetadataQuery))
      {
        return diagnostic.set(BoundedRead::MetadataFailure, "Injected bounded metadata query failure.");
      }
#endif
      const auto status = queryRecord(m_DatasetId, metadata.origin.data(), metadata.record, diagnostic, identity);
      if(status != Status::Complete)
      {
        return status;
      }
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::SecondPassUnpreparedDecoder))
      {
        metadata.record.filterMask = 0;
      }
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::SecondPassLargerRecord))
      {
        metadata.record.storedSize = metadata.maximumStored + 1;
      }
#endif
      if(metadata.record.storedSize > metadata.maximumStored || (metadata.record.allocated && metadata.hasDeflate && metadata.record.filterMask == 0 && !initialized) ||
         (!metadata.record.allocated && !metadata.hasSparse))
      {
        diagnostic.counts(BoundedRead::MetadataChanged, "Bounded second-pass record requires unadmitted stored bytes or decoder/fill state.", metadata.record.storedSize, metadata.maximumStored);
        diagnostic.append(", filter mask=");
        diagnostic.number(metadata.record.filterMask);
        diagnostic.append(", decoder prepared=");
        diagnostic.number(initialized ? 1 : 0);
        diagnostic.append(", fill prepared=");
        diagnostic.number(metadata.hasSparse ? 1 : 0);
        return Status::Failed;
      }
      const auto recordStatus = validateRecord(metadata, metadata.record, diagnostic);
      if(recordStatus != Status::Complete)
      {
        return recordStatus;
      }
#ifdef _WIN32
      if(metadata.record.allocated)
      {
        unsigned ignoredMask = 0;
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredReadAttempt, metadata.record.storedSize, 0);
#endif
        // H5Dread_chunk has no capacity argument. Query, length check, and read share this lock.
        bool injectedFailure = false;
#if SIMPLNX_BUILD_TESTS
        injectedFailure = InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::RawRead);
#endif
        if(injectedFailure || H5Dread_chunk(m_DatasetId, H5P_DEFAULT, metadata.origin.data(), &ignoredMask, stored.data) < 0)
        {
#if SIMPLNX_BUILD_TESTS
          ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredReadFailed, metadata.record.storedSize, 0);
#endif
          return diagnostic.set(BoundedRead::ReadFailure, "Bounded raw HDF5 chunk read failed.");
        }
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredRead, metadata.record.storedSize, 0);
#endif
      }
#endif
    }
    if(!metadata.record.allocated)
    {
      scatter(metadata, extent, m_ElementSize, nullptr, true, destination.data());
      continue;
    }
#ifndef _WIN32
    auto readStored = [&]() {
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredReadAttempt, metadata.record.storedSize, 0);
#endif
      bool injectedFailure = false;
#if SIMPLNX_BUILD_TESTS
      injectedFailure = InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::RawRead);
#endif
      const auto got = injectedFailure ? std::ptrdiff_t{-1} : detail::positionalRead(readHandle, stored.data, static_cast<usize>(metadata.record.storedSize), metadata.record.storedAddress);
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath,
                               got == static_cast<std::ptrdiff_t>(metadata.record.storedSize) ? CodecIoEventForTesting::StoredRead :
                               got < 0                                                        ? CodecIoEventForTesting::StoredReadFailed :
                                                                                                CodecIoEventForTesting::StoredReadShort,
                               got >= 0 ? static_cast<uint64>(got) : metadata.record.storedSize, metadata.record.storedSize);
#endif
      return got;
    };
    static_assert(sizeof(readStored) <= k_BoundedReadClosureBytes);
    auto got = readStored();
    if(got != static_cast<std::ptrdiff_t>(metadata.record.storedSize))
    {
      std::lock_guard<std::mutex> recoveryLock(m_PositionalReadRecoveryMutex);
      got = readStored();
      if(got != static_cast<std::ptrdiff_t>(metadata.record.storedSize))
      {
        {
          std::lock_guard<std::mutex> lock(Support::ApiLock());
          if(H5Fflush(m_DatasetId, H5F_SCOPE_LOCAL) < 0)
          {
            return diagnostic.set(BoundedRead::ReadFailure, "Bounded short-read recovery flush failed.");
          }
        }
        got = readStored();
      }
    }
    if(got != static_cast<std::ptrdiff_t>(metadata.record.storedSize))
    {
      return diagnostic.set(BoundedRead::ReadFailure, "Bounded positional chunk read was short or failed.");
    }
#endif
    const std::byte* decoded = stored.data;
    if(metadata.hasDeflate && metadata.record.filterMask == 0)
    {
      if(inflateReset(&metadata.stream) != Z_OK)
      {
        return diagnostic.set(BoundedRead::DecodeFailure, "Bounded decoder reset failed.");
      }
      metadata.stream.next_in = reinterpret_cast<Bytef*>(stored.data);
      metadata.stream.avail_in = static_cast<uInt>(metadata.record.storedSize);
      metadata.stream.next_out = reinterpret_cast<Bytef*>(nominal.data);
      metadata.stream.avail_out = static_cast<uInt>(metadata.nominalBytes);
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::InflateAttempt, metadata.record.storedSize, metadata.nominalBytes);
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
      if(status != Z_STREAM_END || metadata.stream.total_out != metadata.nominalBytes || metadata.stream.avail_in != 0)
      {
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::InflateFailed, metadata.record.storedSize, metadata.stream.total_out);
#endif
        return diagnostic.set(metadata.quota.quotaFailed      ? BoundedRead::DecoderQuotaFailure :
                              metadata.quota.allocationFailed ? BoundedRead::AllocationFailure :
                                                                BoundedRead::DecodeFailure,
                              "Bounded decoder did not consume one complete nominal stream.");
      }
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::Inflated, metadata.record.storedSize, metadata.stream.total_out);
#endif
      decoded = nominal.data;
    }
    else if(metadata.hasDeflate)
    {
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::SkippedDeflate, metadata.record.storedSize, metadata.nominalBytes);
#endif
    }
    scatter(metadata, extent, m_ElementSize, decoded, false, destination.data());
  } while(nextChunk(metadata, extent));
  return Status::Complete;
}
} // namespace nx::core::HDF5
