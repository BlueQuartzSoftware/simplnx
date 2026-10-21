#include "simplnx/Common/BoundedRead.hpp"
#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"
#include "simplnx/Utilities/PositionalFileIO.hpp"
#include <H5Ipublic.h>
#include <H5Zpublic.h>
#include <H5public.h>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <nonstd/expected.hpp>
#include <nonstd/span.hpp>
#include <string>
#include <type_traits>
#include <utility>

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
  const std::filesystem::path& File;
  const std::string& Dataset;

#if SIMPLNX_BUILD_TESTS
  void observe(CodecIoEventForTesting event, uint64 bytes, BoundedCarrier carrier) const noexcept
  {
    ObserveCodecIoForTesting(File, Dataset, event, bytes, static_cast<uint64>(carrier));
  }
#endif
};

/** @brief Stores each zlib allocation's admitted size in its own aligned header. */
struct DecoderHeader
{
  alignas(std::max_align_t) uint64 Bytes = 0;
};

/** @brief Bounds zlib-owned storage; this state outlives inflateEnd and every zfree. */
struct DecoderQuota
{
  const BoundedIdentity* Identity = nullptr;
  uint64 Allowance = 0;
  uint64 Live = 0;
  bool QuotaFailed = false;
  bool AllocationFailed = false;

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
    if(!BoundedRead::Multiply(count, size, payload) || !BoundedRead::Add(payload, sizeof(DecoderHeader), bytes) || bytes > quota.Allowance - quota.Live || bytes > std::numeric_limits<usize>::max())
    {
      quota.QuotaFailed = true;
      return nullptr;
    }
#if SIMPLNX_BUILD_TESTS
    quota.Identity->observe(CodecIoEventForTesting::BoundedRequested, bytes, BoundedCarrier::Decoder);
#endif
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(quota.Identity->File, quota.Identity->Dataset, BoundedFaultPointForTesting::DecoderAllocation))
    {
      quota.AllocationFailed = true;
      return nullptr;
    }
#endif
    auto* headerPtr = static_cast<DecoderHeader*>(std::malloc(static_cast<usize>(bytes)));
    if(headerPtr == nullptr)
    {
      quota.AllocationFailed = true;
      return nullptr;
    }
    headerPtr->Bytes = bytes;
    quota.Live += bytes;
#if SIMPLNX_BUILD_TESTS
    quota.Identity->observe(CodecIoEventForTesting::BoundedAcquired, bytes, BoundedCarrier::Decoder);
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
    const uint64 bytes = headerPtr->Bytes;
    std::free(headerPtr);
    quota.Live -= bytes;
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
  , Carrier(carrierValue)
  {
  }

  BoundedBytes(const BoundedBytes&) = delete;
  BoundedBytes& operator=(const BoundedBytes&) = delete;
  BoundedBytes(BoundedBytes&&) = delete;
  BoundedBytes& operator=(BoundedBytes&&) = delete;

  const BoundedIdentity& Identity;
  BoundedCarrier Carrier;
  std::byte* Data = nullptr;
  uint64 Size = 0;

  ~BoundedBytes() noexcept
  {
    if(Data != nullptr)
    {
      ::operator delete(Data);
#if SIMPLNX_BUILD_TESTS
      Identity.observe(CodecIoEventForTesting::BoundedReleased, Size, Carrier);
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
    Identity.observe(CodecIoEventForTesting::BoundedRequested, bytes, Carrier);
#endif
#if SIMPLNX_BUILD_TESTS
    if(InjectCodecFaultForTesting(Identity.File, Identity.Dataset, Carrier == BoundedCarrier::Stored ? BoundedFaultPointForTesting::StoredAllocation : BoundedFaultPointForTesting::NominalAllocation))
    {
      return false;
    }
#endif
    Data = static_cast<std::byte*>(::operator new(static_cast<usize>(bytes), std::nothrow));
    if(Data == nullptr)
    {
      return false;
    }
    Size = bytes;
#if SIMPLNX_BUILD_TESTS
    Identity.observe(CodecIoEventForTesting::BoundedAcquired, Size, Carrier);
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
  : Stored{identity, BoundedCarrier::Stored}
  , Nominal{identity, BoundedCarrier::Nominal}
  {
  }

  BoundedBytes Stored;
  BoundedBytes Nominal;
  std::array<hsize_t, H5S_MAX_RANK> Shape{};
  std::array<hsize_t, H5S_MAX_RANK> Chunks{};
  std::array<hsize_t, H5S_MAX_RANK> Origin{};
  std::array<hsize_t, H5S_MAX_RANK> First{};
  std::array<hsize_t, H5S_MAX_RANK> Last{};
  std::array<hsize_t, H5S_MAX_RANK> Counts{};
  std::array<uint64, H5S_MAX_RANK> NominalStrides{};
  std::array<uint64, H5S_MAX_RANK> OutputStrides{};
  std::array<uint64, H5S_MAX_RANK> Low{};
  std::array<uint64, H5S_MAX_RANK> High{};
  std::array<uint64, H5S_MAX_RANK> Cursor{};
  std::array<uint64, H5S_MAX_RANK> Selected{};
  ParallelChunkCodec::ChunkInfo Record;
  z_stream Stream{};
  DecoderQuota Quota;
  std::array<std::byte, 16> Fill{};
  uint64 NominalBytes = 0;
  uint64 MaximumStored = 0;
  usize Rank = 0;
  bool HasDeflate = false;
  bool NeedsDecoder = false;
  bool HasSparse = false;
};
static_assert(H5S_MAX_RANK <= BoundedRead::k_MaxRank);

/** @brief Bounds a cleanup guard's two-reference capture and active flag on the compiling ABI. */
struct BoundedCleanupControl
{
  std::array<void*, 2> Captures{};
  bool Active = false;
};

/** @brief Inventories the fixed native-query carriers that coexist with chunk metadata. */
struct BoundedNativeQueryControls
{
  std::array<hid_t, 3> Identifiers{};
  std::array<int, 6> Statuses{};
  std::array<unsigned, 6> FilterValuesAndFlags{};
  std::array<usize, 2> CountsAndSize{};
  H5D_layout_t Layout = H5D_LAYOUT_ERROR;
  H5D_fill_value_t Fill = H5D_FILL_VALUE_ERROR;
  haddr_t Address = HADDR_UNDEF;
  hsize_t StoredSize = 0;
  unsigned Mask = 0;
};

/** @brief Keeps all scatter loop state in one admitted fixed-size control. */
struct BoundedScatterControl
{
  uint64 RunValues = 1;
  uint64 JoinedValues = 0;
  uint64 SourceIndex = 0;
  uint64 OutputIndex = 0;
  usize FirstRunDimension = 0;
  usize Dimension = 0;
  usize Reverse = 0;
};

// Scatter borrows two references and two byte pointers, plus scalar width and sparse mode.
// Multiply borrows one result reference and accepts two scalar operands.
constexpr uint64 k_BoundedScatterArgumentBytes = (4 * sizeof(void*)) + sizeof(usize) + sizeof(bool) + sizeof(void*) + (2 * sizeof(uint64));
constexpr uint64 k_BoundedScatterBytes = sizeof(BoundedScatterControl) + k_BoundedScatterArgumentBytes;

// ScopeGuard has one captured callable and an active flag. The two additional
// capture slots per guard cover constructor arguments and source temporaries.
// Four guards cover metadata release, properties, decoder and the POSIX handle.
constexpr uint64 k_BoundedCleanupBytes = 4 * (sizeof(BoundedCleanupControl) + 2 * sizeof(std::array<void*, 2>));
constexpr uint64 k_BoundedReadClosureBytes = 5 * sizeof(void*);
constexpr uint64 k_BoundedControlBytes = sizeof(BoundedIdentity) + sizeof(hid_t) + sizeof(bool) + sizeof(detail::FileHandle) + sizeof(BoundedNativeQueryControls) +
                                         (2 * sizeof(std::lock_guard<std::mutex>)) + k_BoundedCleanupBytes + k_BoundedReadClosureBytes + (2 * sizeof(nonstd::span<std::byte>)) +
                                         sizeof(nonstd::span<const hsize_t>) + (2 * sizeof(Status)) + k_BoundedScatterBytes
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
  if(InjectCodecFaultForTesting(identity.File, identity.Dataset, BoundedFaultPointForTesting::ContradictoryRecord))
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
  for(usize reverse = metadata.Rank; reverse != 0; --reverse)
  {
    const usize dimension = reverse - 1;
    const uint64 end = metadata.Origin[dimension] + std::min(metadata.Chunks[dimension], metadata.Shape[dimension] - metadata.Origin[dimension]) - 1;
    const uint64 nextSelected = ((end - extent.min[dimension]) / extent.stride[dimension]) + 1;
    if(nextSelected < metadata.Counts[dimension])
    {
      const uint64 coordinate = extent.min[dimension] + (nextSelected * extent.stride[dimension]);
      metadata.Origin[dimension] = (coordinate / metadata.Chunks[dimension]) * metadata.Chunks[dimension];
      return true;
    }
    metadata.Origin[dimension] = metadata.First[dimension];
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
  for(usize dimension = 0; dimension < metadata.Rank; ++dimension)
  {
    const uint64 chunkEnd = metadata.Origin[dimension] + std::min(metadata.Chunks[dimension], metadata.Shape[dimension] - metadata.Origin[dimension]) - 1;
    const uint64 difference = metadata.Origin[dimension] > extent.min[dimension] ? metadata.Origin[dimension] - extent.min[dimension] : 0;
    metadata.Low[dimension] = difference / extent.stride[dimension] + (difference % extent.stride[dimension] != 0 ? 1 : 0);
    if(chunkEnd < extent.min[dimension])
    {
      return false;
    }
    metadata.High[dimension] = (std::min(chunkEnd, extent.max[dimension]) - extent.min[dimension]) / extent.stride[dimension];
    if(metadata.Low[dimension] > metadata.High[dimension])
    {
      return false;
    }
    metadata.Cursor[dimension] = metadata.Low[dimension];
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
  if((record.filterMask & ~(metadata.HasDeflate ? 1U : 0U)) != 0)
  {
    return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk has a filter mask outside its declared pipeline.");
  }
  const bool compressed = metadata.HasDeflate && record.filterMask == 0;
  if(!compressed && record.storedSize != metadata.NominalBytes)
  {
    return diagnostic.counts(BoundedRead::MetadataFailure, "Bounded raw chunk byte length does not match nominal storage.", metadata.NominalBytes, record.storedSize);
  }
  if(record.storedSize > std::numeric_limits<usize>::max() || record.storedSize > static_cast<uint64>(std::numeric_limits<std::ptrdiff_t>::max()) ||
     (compressed && (record.storedSize > std::numeric_limits<uInt>::max() || metadata.NominalBytes > std::numeric_limits<uInt>::max())))
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
  control.FirstRunDimension = metadata.Rank;
  if(!sparse && extent.stride[metadata.Rank - 1] == 1)
  {
    control.FirstRunDimension = metadata.Rank - 1;
    control.RunValues = metadata.High[control.FirstRunDimension] - metadata.Low[control.FirstRunDimension] + 1;
    while(control.FirstRunDimension != 0)
    {
      control.Dimension = control.FirstRunDimension - 1;
      // Nominal strides retain edge padding. Output strides retain gaps from split components and partial rows.
      if(extent.stride[control.Dimension] != 1 || metadata.NominalStrides[control.Dimension] != control.RunValues || metadata.OutputStrides[control.Dimension] != control.RunValues ||
         !BoundedRead::Multiply(control.RunValues, metadata.High[control.Dimension] - metadata.Low[control.Dimension] + 1, control.JoinedValues))
      {
        break;
      }
      control.RunValues = control.JoinedValues;
      --control.FirstRunDimension;
    }
  }
  // Each run stays within checked nominal/output products, so its scalar-to-byte conversion fits usize.
  // Sparse fills remain scalar copies from the fixed primitive fill buffer.
  for(;;)
  {
    control.SourceIndex = 0;
    control.OutputIndex = 0;
    for(control.Dimension = 0; control.Dimension < metadata.Rank; ++control.Dimension)
    {
      control.SourceIndex +=
          (extent.min[control.Dimension] + metadata.Cursor[control.Dimension] * extent.stride[control.Dimension] - metadata.Origin[control.Dimension]) * metadata.NominalStrides[control.Dimension];
      control.OutputIndex += metadata.Cursor[control.Dimension] * metadata.OutputStrides[control.Dimension];
    }
    std::memcpy(destination + (static_cast<usize>(control.OutputIndex) * elementSize), sparse ? metadata.Fill.data() : source + (static_cast<usize>(control.SourceIndex) * elementSize),
                static_cast<usize>(control.RunValues) * elementSize);
    control.Reverse = control.FirstRunDimension;
    while(control.Reverse != 0)
    {
      control.Dimension = control.Reverse - 1;
      if(metadata.Cursor[control.Dimension] < metadata.High[control.Dimension])
      {
        ++metadata.Cursor[control.Dimension];
        break;
      }
      metadata.Cursor[control.Dimension] = metadata.Low[control.Dimension];
      --control.Reverse;
    }
    if(control.Reverse == 0)
    {
      return;
    }
  }
}
} // namespace

#if SIMPLNX_BUILD_TESTS
uint64 ParallelChunkCodec::boundedMetadataCapacityBytesForTesting() const noexcept
{
  return ((m_FilePath.native().capacity() + 1) * sizeof(std::filesystem::path::value_type)) + m_DatasetPath.capacity() + 1 +
         ((m_TupleShape.capacity() + m_ChunkShape.capacity() + m_ComponentShape.capacity()) * sizeof(uint64));
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
  const MetadataIdentityForTesting metadataIdentity(m_FilePath, m_DatasetPath);
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
    const std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
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
          status = queryRecord(m_DatasetId, offset.data(), info, diagnostic, BoundedIdentity{.File = m_FilePath, .Dataset = m_DatasetPath});
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
  const BoundedIdentity identity{.File = m_FilePath, .Dataset = m_DatasetPath};
#if SIMPLNX_BUILD_TESTS
  const BoundedReadTaskScopeForTesting taskObservation;
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
  const MetadataIdentityForTesting metadataIdentity(m_FilePath, m_DatasetPath);
#endif
  const uint64 available = payloadBudgetBytes - k_BoundedFixedBytes;
  metadata.Quota.Identity = &identity;
  hid_t properties = H5I_INVALID_HID;
  auto closeProperties = MakeScopeGuard([&properties]() noexcept {
    if(properties >= 0)
    {
      const std::lock_guard<std::mutex> lock(Support::ApiLock());
      H5Pclose(properties);
    }
  });
  static_assert(sizeof(closeProperties) <= sizeof(BoundedCleanupControl));
  {
    const std::lock_guard<std::mutex> lock(Support::ApiLock());
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
      metadata.HasDeflate = true;
    }
    const hid_t space = SIMPLNX_OBSERVE_METADATA_CALL(MetadataSpaceCall, H5Dget_space(m_DatasetId));
    if(space < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk dataspace query failed.");
    }
    const int rank = H5Sget_simple_extent_ndims(space);
    const int dimensions = rank > 0 && rank <= H5S_MAX_RANK ? H5Sget_simple_extent_dims(space, metadata.Shape.data(), nullptr) : -1;
    H5Sclose(space);
    if(rank < 0 || (rank > 0 && rank <= H5S_MAX_RANK && dimensions < 0))
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded chunk rank or dimensions query failed.");
    }
    if(rank == 0 || rank > H5S_MAX_RANK)
    {
      return diagnostic.set(BoundedRead::Unsupported, "Bounded chunk read does not support this rank.");
    }
    metadata.Rank = static_cast<usize>(rank);
    if(H5Pget_chunk(properties, rank, metadata.Chunks.data()) != rank)
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
      BoundedRead::ValidateExtent(extent, nonstd::span<const hsize_t>(metadata.Shape.data(), metadata.Rank), 1, destination.size() / m_ElementSize, selectedValues, diagnostic, m_ElementSize);
  if(extentStatus != Status::Complete)
  {
    return extentStatus;
  }
  uint64 nominalValues = 1;
  uint64 outputStride = 1;
  for(usize reverse = metadata.Rank; reverse != 0; --reverse)
  {
    const usize dimension = reverse - 1;
    if(metadata.Chunks[dimension] == 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded physical chunk shape has a zero dimension.");
    }
    metadata.NominalStrides[dimension] = nominalValues;
    metadata.OutputStrides[dimension] = outputStride;
    metadata.Counts[dimension] = (extent.max[dimension] - extent.min[dimension]) / extent.stride[dimension] + 1;
    if(!BoundedRead::Multiply(nominalValues, metadata.Chunks[dimension], nominalValues) || !BoundedRead::Multiply(outputStride, metadata.Counts[dimension], outputStride))
    {
      return diagnostic.set(BoundedRead::InvalidExtent, "Bounded chunk strides overflow uint64.");
    }
    metadata.First[dimension] = (extent.min[dimension] / metadata.Chunks[dimension]) * metadata.Chunks[dimension];
    metadata.Last[dimension] = (extent.max[dimension] / metadata.Chunks[dimension]) * metadata.Chunks[dimension];
    metadata.Origin[dimension] = metadata.First[dimension];
  }
  if(!BoundedRead::Multiply(nominalValues, m_ElementSize, metadata.NominalBytes) || metadata.NominalBytes > std::numeric_limits<usize>::max())
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
      const std::lock_guard<std::mutex> lock(Support::ApiLock());
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::MetadataQuery))
      {
        return diagnostic.set(BoundedRead::MetadataFailure, "Injected bounded metadata query failure.");
      }
#endif
      const auto status = queryRecord(m_DatasetId, metadata.Origin.data(), metadata.Record, diagnostic, identity);
      if(status != Status::Complete)
      {
        return status;
      }
    }
    const auto recordStatus = validateRecord(metadata, metadata.Record, diagnostic);
    if(recordStatus != Status::Complete)
    {
      return recordStatus;
    }
    if(metadata.Record.allocated)
    {
      metadata.MaximumStored = std::max(metadata.MaximumStored, metadata.Record.storedSize);
      metadata.NeedsDecoder = metadata.NeedsDecoder || (metadata.HasDeflate && metadata.Record.filterMask == 0);
    }
    else
    {
      metadata.HasSparse = true;
    }
  } while(nextChunk(metadata, extent));
  if(metadata.HasSparse)
  {
    const std::lock_guard<std::mutex> lock(Support::ApiLock());
    H5D_fill_value_t fillStatus = H5D_FILL_VALUE_ERROR;
    if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataFillCall, H5Pfill_value_defined(properties, &fillStatus)) < 0 || fillStatus == H5D_FILL_VALUE_ERROR)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded fill-value status query failed.");
    }
    if(fillStatus == H5D_FILL_VALUE_UNDEFINED)
    {
      return diagnostic.set(BoundedRead::UndefinedFill, "Bounded sparse chunk has an undefined fill value.");
    }
    if(SIMPLNX_OBSERVE_METADATA_CALL(MetadataFillCall, H5Pget_fill_value(properties, m_BoundedNativeType, metadata.Fill.data())) < 0)
    {
      return diagnostic.set(BoundedRead::MetadataFailure, "Bounded fill-value conversion failed.");
    }
  }
  uint64 carrierBytes = 0;
  const uint64 nominalAllocation = metadata.NeedsDecoder ? metadata.NominalBytes : 0;
  if(!BoundedRead::Add(metadata.MaximumStored, nominalAllocation, carrierBytes))
  {
    return diagnostic.set(BoundedRead::InvalidExtent, "Bounded raw and nominal carrier byte sum overflows uint64.");
  }
  if(carrierBytes > available)
  {
    return diagnostic.counts(BoundedRead::Insufficient, "Bounded raw and nominal carrier bytes exceed the allowance.", carrierBytes, available);
  }
  metadata.Quota.Allowance = available - carrierBytes;
  metadata.Stream.zalloc = &DecoderQuota::Allocate;
  metadata.Stream.zfree = &DecoderQuota::Free;
  metadata.Stream.opaque = &metadata.Quota;
  bool initialized = false;
  auto finishDecoder = MakeScopeGuard([&metadata, &initialized]() noexcept {
    if(initialized)
    {
      inflateEnd(&metadata.Stream);
    }
  });
  static_assert(sizeof(finishDecoder) <= sizeof(BoundedCleanupControl));
  if(metadata.NeedsDecoder)
  {
    const int status = inflateInit(&metadata.Stream);
    initialized = status == Z_OK;
    if(!initialized)
    {
      return diagnostic.set(metadata.Quota.QuotaFailed ? BoundedRead::Insufficient : BoundedRead::AllocationFailure, "Bounded zlib initialization cannot acquire admitted state.");
    }
    constexpr uint64 windowBytes = 32768 + sizeof(DecoderHeader);
    if(windowBytes > metadata.Quota.Allowance - metadata.Quota.Live)
    {
      return diagnostic.counts(BoundedRead::Insufficient, "Bounded zlib state and maximum-window bytes exceed the allowance.", metadata.Quota.Live + windowBytes, metadata.Quota.Allowance);
    }
  }
  auto& stored = metadata.Stored;
  auto& nominal = metadata.Nominal;
  if(!stored.acquire(metadata.MaximumStored) || !nominal.acquire(nominalAllocation))
  {
    return diagnostic.set(BoundedRead::AllocationFailure, "Bounded chunk carrier allocation failed within its allowance.");
  }
#ifndef _WIN32
  // Borrow the POSIX native string; opening this independent descriptor allocates no C++ path carrier.
  const auto readHandle = metadata.MaximumStored == 0 ? detail::invalidFileHandle() : detail::openFileForRead(m_FilePath.native());
  auto closeReadHandle = MakeScopeGuard([readHandle]() noexcept {
    if(detail::isValidFileHandle(readHandle))
    {
      detail::closeFileHandle(readHandle);
    }
  });
  static_assert(sizeof(closeReadHandle) <= sizeof(BoundedCleanupControl));
  if(metadata.MaximumStored != 0 && !detail::isValidFileHandle(readHandle))
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
      const std::lock_guard<std::mutex> lock(Support::ApiLock());
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::ChunkRecordQuery, 1, 0);
#endif
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::MetadataQuery))
      {
        return diagnostic.set(BoundedRead::MetadataFailure, "Injected bounded metadata query failure.");
      }
#endif
      const auto status = queryRecord(m_DatasetId, metadata.Origin.data(), metadata.Record, diagnostic, identity);
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
        metadata.Record.storedSize = metadata.MaximumStored + 1;
      }
#endif
      if(metadata.Record.storedSize > metadata.MaximumStored || (metadata.Record.allocated && metadata.HasDeflate && metadata.Record.filterMask == 0 && !initialized) ||
         (!metadata.Record.allocated && !metadata.HasSparse))
      {
        diagnostic.counts(BoundedRead::MetadataChanged, "Bounded second-pass record requires unadmitted stored bytes or decoder/fill state.", metadata.Record.storedSize, metadata.MaximumStored);
        diagnostic.append(", filter mask=");
        diagnostic.number(metadata.Record.filterMask);
        diagnostic.append(", decoder prepared=");
        diagnostic.number(initialized ? 1 : 0);
        diagnostic.append(", fill prepared=");
        diagnostic.number(metadata.HasSparse ? 1 : 0);
        return Status::Failed;
      }
      const auto recordStatus = validateRecord(metadata, metadata.Record, diagnostic);
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
        if(injectedFailure || H5Dread_chunk(m_DatasetId, H5P_DEFAULT, metadata.Origin.data(), &ignoredMask, stored.Data) < 0)
        {
#if SIMPLNX_BUILD_TESTS
          ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredReadFailed, metadata.Record.storedSize, 0);
#endif
          return diagnostic.set(BoundedRead::ReadFailure, "Bounded raw HDF5 chunk read failed.");
        }
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::StoredRead, metadata.Record.storedSize, 0);
#endif
      }
#endif
    }
    if(!metadata.Record.allocated)
    {
      scatter(metadata, extent, m_ElementSize, nullptr, true, destination.data());
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
      const auto got = injectedFailure ? std::ptrdiff_t{-1} : detail::positionalRead(readHandle, stored.Data, static_cast<usize>(metadata.Record.storedSize), metadata.Record.storedAddress);
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath,
                               got == static_cast<std::ptrdiff_t>(metadata.Record.storedSize) ? CodecIoEventForTesting::StoredRead :
                               got < 0                                                        ? CodecIoEventForTesting::StoredReadFailed :
                                                                                                CodecIoEventForTesting::StoredReadShort,
                               got >= 0 ? static_cast<uint64>(got) : metadata.Record.storedSize, metadata.Record.storedSize);
#endif
      return got;
    };
    static_assert(sizeof(readStored) <= k_BoundedReadClosureBytes);
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
            return diagnostic.set(BoundedRead::ReadFailure, "Bounded short-read recovery flush failed.");
          }
        }
        got = readStored();
      }
    }
    if(got != static_cast<std::ptrdiff_t>(metadata.Record.storedSize))
    {
      return diagnostic.set(BoundedRead::ReadFailure, "Bounded positional chunk read was short or failed.");
    }
#endif
    const std::byte* decodedPtr = stored.Data;
    if(metadata.HasDeflate && metadata.Record.filterMask == 0)
    {
      if(inflateReset(&metadata.Stream) != Z_OK)
      {
        return diagnostic.set(BoundedRead::DecodeFailure, "Bounded decoder reset failed.");
      }
      metadata.Stream.next_in = reinterpret_cast<Bytef*>(stored.Data);
      metadata.Stream.avail_in = static_cast<uInt>(metadata.Record.storedSize);
      metadata.Stream.next_out = reinterpret_cast<Bytef*>(nominal.Data);
      metadata.Stream.avail_out = static_cast<uInt>(metadata.NominalBytes);
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::InflateAttempt, metadata.Record.storedSize, metadata.NominalBytes);
#endif
      int status = Z_OK;
#if SIMPLNX_BUILD_TESTS
      if(InjectCodecFaultForTesting(m_FilePath, m_DatasetPath, BoundedFaultPointForTesting::DecoderQuotaAfterPayload))
      {
        // Withhold the last checksum byte to exercise zlib's real window request.
        // This deterministic fault changes only the admitted test allocator limit.
        metadata.Quota.Allowance = metadata.Quota.Live;
        --metadata.Stream.avail_in;
        status = inflate(&metadata.Stream, Z_NO_FLUSH);
      }
      else
#endif
      {
        status = inflate(&metadata.Stream, Z_FINISH);
      }
      if(status != Z_STREAM_END || metadata.Stream.total_out != metadata.NominalBytes || metadata.Stream.avail_in != 0)
      {
#if SIMPLNX_BUILD_TESTS
        ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::InflateFailed, metadata.Record.storedSize, metadata.Stream.total_out);
#endif
        return diagnostic.set(metadata.Quota.QuotaFailed      ? BoundedRead::DecoderQuotaFailure :
                              metadata.Quota.AllocationFailed ? BoundedRead::AllocationFailure :
                                                                BoundedRead::DecodeFailure,
                              "Bounded decoder did not consume one complete nominal stream.");
      }
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::Inflated, metadata.Record.storedSize, metadata.Stream.total_out);
#endif
      decodedPtr = nominal.Data;
    }
    else if(metadata.HasDeflate)
    {
#if SIMPLNX_BUILD_TESTS
      ObserveCodecIoForTesting(m_FilePath, m_DatasetPath, CodecIoEventForTesting::SkippedDeflate, metadata.Record.storedSize, metadata.NominalBytes);
#endif
    }
    scatter(metadata, extent, m_ElementSize, decodedPtr, false, destination.data());
  } while(nextChunk(metadata, extent));
  return Status::Complete;
}
} // namespace nx::core::HDF5
