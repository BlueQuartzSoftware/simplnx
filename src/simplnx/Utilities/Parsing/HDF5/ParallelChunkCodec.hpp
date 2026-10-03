#pragma once

#include "simplnx/Common/BoundedRead.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/PositionalFileIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include <H5Ipublic.h>
#include <nonstd/span.hpp>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace nx::core::HDF5
{
#if SIMPLNX_BUILD_TESTS
/**
 * @enum CodecIoEventForTesting
 * @brief Identifies measured codec work without retaining event records.
 */
enum class CodecIoEventForTesting
{
  StoredRead,                       ///< Complete successful raw read; input is transferred bytes.
  Inflated,                         ///< Successful zlib decode; input is supplied buffer bytes, output is decoded bytes.
  StoredWrite,                      ///< Successful raw write; input is stored bytes.
  StoredReadAttempt,                ///< Native read attempt; input is requested bytes.
  StoredReadShort,                  ///< Short read; input is received bytes and output is requested bytes.
  StoredReadFailed,                 ///< Failed native read; input is requested bytes.
  InflateAttempt,                   ///< Decode attempt; input is stored bytes and output is expected bytes.
  InflateFailed,                    ///< Failed decode; input is stored bytes and output is expected bytes.
  SkippedDeflate,                   ///< Native bytes bypass zlib; input is stored bytes and output is nominal bytes.
  ChunkRecordQuery,                 ///< One completed chunk-record query, including a failed native status.
  RawAcquired,                      ///< Raw carrier acquired; input is actual capacity in bytes.
  RawReleased,                      ///< Raw carrier destroyed; input is its prior capacity in bytes.
  LegacyAcquired,                   ///< Payload acquired; input is capacity and output is its category.
  LegacyReleased,                   ///< Payload destroyed; input is capacity and output is its category.
  LegacyToReturned,                 ///< Category transfer; input is capacity and output is the previous category.
  LegacyTransferred,                ///< Cache takes ownership; input is capacity and output is its category.
  TypedReadAttempt,                 ///< Typed H5Dread attempt; input is logical requested bytes.
  TypedReadCompleted,               ///< Successful typed read; input is logical bytes, not device traffic.
  TypedReadFailed,                  ///< Failed typed H5Dread; input is requested bytes.
  ContiguousReadAttempt,            ///< Positional contiguous read attempt; input is requested bytes.
  ContiguousReadCompleted,          ///< Successful positional contiguous read; input is received bytes.
  ContiguousReadFailed,             ///< Failed or short contiguous read; input is requested bytes.
  MetadataTypeCall,                 ///< One instrumented datatype API call.
  MetadataLayoutCall,               ///< One instrumented layout API call.
  MetadataFilterCall,               ///< One instrumented filter-list API call.
  MetadataFillCall,                 ///< One instrumented fill-policy API call.
  MetadataSpaceCall,                ///< One instrumented dataspace or selection API call.
  ExtentAcquired,                   ///< Returned-extent payload acquired; input is retained byte capacity.
  ExtentReleased,                   ///< Failed extent payload destroyed; input is retained capacity.
  ExtentReturned,                   ///< Successful extent return; input is capacity and output is logical bytes.
  SamplerAcquired,                  ///< Sample carrier acquired; input is bytes and output is category.
  SamplerReleased,                  ///< Sample carrier destroyed; input is bytes and output is category.
  SamplerPublished,                 ///< Sample payload becomes output; input is bytes and output is category.
  SamplerHistogramReturned,         ///< Histogram helper returns charged payload; input is bytes.
  SamplerHistogramAdopted,          ///< Accumulator replacement; input is new capacity, output is destroyed capacity.
  SamplerTypedAdopted,              ///< Caller adopts the already-observed returned extent capacity.
  SamplerReadRequest,               ///< Bulk request; input is values and output is logical bytes.
  SamplerReadCompleted,             ///< Successful bounded read; input is values, output is logical bytes.
  SamplerReserveRequest,            ///< Allocation request; input is requested bytes and output is category.
  SamplerWorkingReservation,        ///< Shared working reservation observed outside backend locks.
  SamplerInvalid,                   ///< Measurement metadata cannot be represented; production flow is unchanged.
  PendingWriteScan,                 ///< Visited resident/parked entries during an optional clean-state proof.
  DiagnosticRequested,              ///< Terminal reserve request; input bytes, output zero vector or one string.
  DiagnosticRetained,               ///< Terminal retained record-vector bytes and message bytes; no pending ownership record.
  BoundedDeferredRequest,           ///< Recorded exact request delivered after identity protection; not a live event.
  BoundedDeferredPublishedCapacity, ///< Recorded retained capacity already published; not a live event.
  BoundedAdmissionCharge,           ///< Conservative pre-service charge; input is bound bytes, output is carrier.
  BoundedRequested,                 ///< Before owned storage service; input is bytes, output is carrier category.
  BoundedAcquired,                  ///< Actual owned bytes acquired; input is bytes, output is carrier category.
  BoundedPublished,                 ///< Owned scratch becomes retained store state; input is bytes, output is carrier.
  BoundedReleased,                  ///< Actual owned bytes released; input is bytes, output is carrier category.
  TypedWriteAttempt,                ///< Typed H5Dwrite attempt; input is selected logical bytes.
  TypedWriteCompleted,              ///< Successful typed H5Dwrite; input is logical bytes, not stored or device bytes.
  TypedWriteFailed,                 ///< Failed or throwing typed write; input is selected logical bytes.
  TypedWriteSizeInvalid,            ///< Unrepresentable logical byte count; invalidates measurement without changing the native call.
  Count                             ///< Supplies the fixed observation-array size.
};

/** @brief Names narrow bounded-read failure sites on the existing quiescent test observer. */
enum class BoundedFaultPointForTesting
{
  CheckedMetadataUnsupportedBuild,
  NativeSizeQuery,
  MetadataQuery,
  ContradictoryRecord,
  SecondPassUnpreparedDecoder,
  SecondPassLargerRecord,
  StoredAllocation,
  NominalAllocation,
  DecoderAllocation,
  DecoderQuotaAfterPayload,
  RawRead
};

/**
 * @struct CodecIoObserverForTesting
 * @brief Borrows a nonallocating callback while all selected codec work runs.
 *
 * Install and restore only while codec workers are quiescent. The context and borrowed identities outlive every callback.
 */
struct SIMPLNX_EXPORT CodecIoObserverForTesting
{
  void* context = nullptr;
  void (*callback)(void*, const std::filesystem::path&, std::string_view, CodecIoEventForTesting, uint64 inputBytes, uint64 outputBytes) noexcept = nullptr;
  bool (*boundedFault)(void*, const std::filesystem::path&, std::string_view, BoundedFaultPointForTesting) noexcept = nullptr;
};

/** @brief Registers synchronous bounded work with the existing observer quiescence counter. */
class SIMPLNX_EXPORT BoundedReadTaskScopeForTesting
{
public:
  BoundedReadTaskScopeForTesting() noexcept;
  ~BoundedReadTaskScopeForTesting() noexcept;
  BoundedReadTaskScopeForTesting(const BoundedReadTaskScopeForTesting&) = delete;
  BoundedReadTaskScopeForTesting& operator=(const BoundedReadTaskScopeForTesting&) = delete;

private:
  bool m_Active = false;
};

/**
 * @brief Installs a test observer without allocating or locking.
 * @param observer Supplies the callback and its borrowed context.
 * @return Previous observer for a scoped restoration guard.
 * @pre All codec workers are quiescent during replacement.
 *
 * Observed live payloads or tasks refuse replacement and record integrity failure. This check does not join or synchronize workers.
 */
SIMPLNX_EXPORT CodecIoObserverForTesting SetCodecIoObserverForTesting(CodecIoObserverForTesting observer) noexcept;

/**
 * @brief Invokes an optional fault leaf through the shared scoped observer.
 * @param file Borrows the backend file identity.
 * @param dataset Borrows the dataset identity.
 * @param point Selects the narrow failure boundary.
 * @return True when this leaf must inject the selected failure.
 */
SIMPLNX_EXPORT bool InjectCodecFaultForTesting(const std::filesystem::path& file, std::string_view dataset, BoundedFaultPointForTesting point) noexcept;

/**
 * @brief Emits one observation to the installed test callback.
 * @param filePath Borrows the source identity for this synchronous callback.
 * @param datasetPath Borrows the dataset identity.
 * @param event Identifies the measured operation.
 * @param inputBytes Supplies the event's input count.
 * @param outputBytes Supplies the event's output count.
 * @pre The callback does not allocate, lock, throw, or reenter storage code.
 */
SIMPLNX_EXPORT void ObserveCodecIoForTesting(const std::filesystem::path& filePath, std::string_view datasetPath, CodecIoEventForTesting event, uint64 inputBytes, uint64 outputBytes) noexcept;

/**
 * @enum LegacyPayloadKindForTesting
 * @brief Identifies measured legacy payload storage across moves.
 */
enum class LegacyPayloadKindForTesting : uint8
{
  Nominal, ///< Full decoded chunk image.
  Edge,    ///< Clamped edge image overlapping the nominal image.
  Returned ///< Image returned to a known loader or consumer.
};

/**
 * @namespace testing_detail
 * @brief Contains fixed records for the scoped ownership observation.
 */
namespace testing_detail
{
/**
 * @struct LegacyPayloadRecordForTesting
 * @brief Keeps one nonowning receipt without allocating a registry.
 */
struct LegacyPayloadRecordForTesting
{
  const std::filesystem::path* file = nullptr;
  std::string_view dataset;
  const std::byte* data = nullptr;
  usize capacity = 0;
  LegacyPayloadKindForTesting kind = LegacyPayloadKindForTesting::Nominal;
  bool charged = false;
};
} // namespace testing_detail

/**
 * @struct CodecOwnershipStateForTesting
 * @brief Reports joined ownership integrity without reading other workers' TLS.
 */
struct CodecOwnershipStateForTesting
{
  uint64 outstandingBuffers = 0;
  uint64 activeTasks = 0;
  uint64 integrityFailures = 0;
  uint64 unobservedReturns = 0;
  uint64 errorAggregations = 0;
};

/**
 * @brief Reads bounded atomic ownership counters.
 * @return Snapshot; compare integrity and unobserved counters with their entry values.
 */
SIMPLNX_EXPORT CodecOwnershipStateForTesting GetCodecOwnershipStateForTesting() noexcept;

/**
 * @class LegacyPayloadTicketForTesting
 * @brief Observes one byte vector until destruction or cache ownership transfer.
 *
 * Declare this ticket before its vector. Moves transfer observation, never vector ownership. Tickets stay on their acquisition thread.
 */
class SIMPLNX_EXPORT LegacyPayloadTicketForTesting
{
public:
  /**
   * @brief Creates an uncharged ticket without allocating storage.
   */
  LegacyPayloadTicketForTesting() = default;
  /**
   * @brief Releases the observation after its associated vector dies.
   */
  ~LegacyPayloadTicketForTesting() noexcept;
  LegacyPayloadTicketForTesting(const LegacyPayloadTicketForTesting&) = delete;
  LegacyPayloadTicketForTesting& operator=(const LegacyPayloadTicketForTesting&) = delete;
  /**
   * @brief Moves the fixed receipt without changing charged bytes.
   * @param other Supplies the receipt and becomes empty.
   */
  LegacyPayloadTicketForTesting(LegacyPayloadTicketForTesting&& other) noexcept;
  LegacyPayloadTicketForTesting& operator=(LegacyPayloadTicketForTesting&&) = delete;

  /**
   * @brief Starts observation after a successful payload allocation.
   * @param file Borrows the stable file identity.
   * @param dataset Borrows the stable dataset identity.
   * @param bytes Supplies the allocated byte vector.
   * @param kind Identifies its initial storage category.
   * @pre This ticket is empty and the identities outlive observation.
   */
  void acquire(const std::filesystem::path& file, std::string_view dataset, const std::vector<std::byte>& bytes, LegacyPayloadKindForTesting kind) noexcept;
  /**
   * @brief Consumes the immediate vector-return receipt without allocating.
   * @param bytes Supplies the returned vector before any fallible caller work.
   * @param allowUnobserved Permits explicitly unobserved sparse or parked-source returns.
   * @pre This ticket is empty; the corresponding vector-return expression just completed.
   */
  void consumeReturn(const std::vector<std::byte>& bytes, bool allowUnobserved = false) noexcept;
  /**
   * @brief Consumes a replacement receipt before releasing the destroyed old payload's observation.
   * @param bytes Supplies the newly assigned vector.
   * @param allowUnobserved Permits a sparse or parked-source replacement without a codec receipt.
   * @pre The vector assignment just destroyed the old payload; no callback or fallible work has followed it.
   */
  void replaceDestroyedFromReturn(const std::vector<std::byte>& bytes, bool allowUnobserved = false) noexcept;
  /**
   * @brief Publishes one receipt for an immediately following nonthrowing vector return.
   * @pre The thread's pending slot is empty and no fallible work follows before return.
   */
  void forwardReturn() noexcept;
  /**
   * @brief Releases observation after destruction of a replaced vector buffer.
   * @pre The observed payload has already been destroyed; no later release belongs to this ticket.
   */
  void releaseDestroyed() noexcept;
  /**
   * @brief Ends operation-scratch observation at the actual cache move or swap.
   * @pre Cache-owned storage has just taken the buffer matched before mutation.
   */
  void transferToCache() noexcept;
  /**
   * @brief Checks a candidate handoff without changing observation.
   * @param bytes Supplies the source vector before a move or swap.
   * @return True only when the charged pointer and capacity match.
   */
  bool matches(const std::vector<std::byte>& bytes) const noexcept;
  bool charged() const noexcept;

private:
  testing_detail::LegacyPayloadRecordForTesting m_Record;
};

/**
 * @class LegacyPayloadTaskScopeForTesting
 * @brief Checks the current worker's pending receipt after its local results die.
 */
class SIMPLNX_EXPORT LegacyPayloadTaskScopeForTesting
{
public:
  /**
   * @brief Starts a task observation while a shared observer is installed.
   */
  LegacyPayloadTaskScopeForTesting() noexcept;
  /**
   * @brief Records and clears a stranded receipt without throwing from cleanup.
   */
  ~LegacyPayloadTaskScopeForTesting() noexcept;
  /**
   * @brief Checks released inner payload state while the worker's error mutex is held.
   * @pre Local result and ticket scopes have already unwound.
   */
  void observeExceptionAggregation() noexcept;
  LegacyPayloadTaskScopeForTesting(const LegacyPayloadTaskScopeForTesting&) = delete;
  LegacyPayloadTaskScopeForTesting& operator=(const LegacyPayloadTaskScopeForTesting&) = delete;

private:
  bool m_Active = false;
  uint64 m_ThreadOutstandingAtEntry = 0;
  LegacyPayloadTicketForTesting* m_AdmissionAtEntry = nullptr;
};

/**
 * @class LegacyPayloadAdmissionScopeForTesting
 * @brief Borrows one worker-owned ticket across synchronous clean-cache admission.
 */
class SIMPLNX_EXPORT LegacyPayloadAdmissionScopeForTesting
{
public:
  /**
   * @brief Selects the ticket inspected by actual cache move/swap sites.
   * @param ticket Borrows the candidate payload ticket through the synchronous call.
   */
  explicit LegacyPayloadAdmissionScopeForTesting(LegacyPayloadTicketForTesting& ticket) noexcept;
  /**
   * @brief Restores the previous thread-local admission pointer.
   */
  ~LegacyPayloadAdmissionScopeForTesting() noexcept;
  LegacyPayloadAdmissionScopeForTesting(const LegacyPayloadAdmissionScopeForTesting&) = delete;
  LegacyPayloadAdmissionScopeForTesting& operator=(const LegacyPayloadAdmissionScopeForTesting&) = delete;

private:
  LegacyPayloadTicketForTesting* m_Previous = nullptr;
};

/**
 * @brief Matches the borrowed admission ticket before a cache move/swap.
 * @param bytes Supplies the still-unmodified caller payload.
 * @return Matching ticket or null; empty preparation and unrelated bytes do not match.
 */
SIMPLNX_EXPORT LegacyPayloadTicketForTesting* MatchLegacyAdmissionForTesting(const std::vector<std::byte>& bytes) noexcept;

/**
 * @class MetadataIdentityForTesting
 * @brief Borrows a read operation's identity for named native metadata calls.
 */
class SIMPLNX_EXPORT MetadataIdentityForTesting
{
public:
  /**
   * @brief Selects a stable identity until this scope ends.
   * @param file Borrows the attached file path.
   * @param dataset Borrows the dataset name.
   */
  MetadataIdentityForTesting(const std::filesystem::path& file, std::string_view dataset) noexcept;
  /**
   * @brief Restores the preceding thread-local identity.
   */
  ~MetadataIdentityForTesting() noexcept;
  MetadataIdentityForTesting(const MetadataIdentityForTesting&) = delete;
  MetadataIdentityForTesting& operator=(const MetadataIdentityForTesting&) = delete;

private:
  const std::filesystem::path* m_PreviousFile = nullptr;
  std::string_view m_PreviousDataset;
};

/**
 * @brief Records one attempted metadata query under the current scoped identity.
 * @param event Selects the named query family.
 */
SIMPLNX_EXPORT void ObserveMetadataCallForTesting(CodecIoEventForTesting event) noexcept;

/**
 * @brief Runs one native metadata expression exactly once, then records the call.
 * @tparam Function Supplies the nonallocating native invocation.
 * @param event Selects the query family.
 * @param function Performs the existing native call.
 * @return The unchanged native result, including failed status.
 */
template <typename Function>
auto InvokeMetadataForTesting(CodecIoEventForTesting event, Function&& function) -> decltype(function())
{
  auto result = function();
  ObserveMetadataCallForTesting(event);
  return result;
}

/**
 * @brief Runs one typed HDF5 read and reports actual attempt/completion boundaries.
 * @tparam Function Supplies the existing H5Dread expression.
 * @param file Borrows the stable file identity.
 * @param dataset Borrows the dataset identity.
 * @param bytes Supplies logical requested bytes, not compressed or device bytes.
 * @param function Performs the native read exactly once.
 * @return Unchanged native read status.
 */
template <typename Function>
auto InvokeTypedReadForTesting(const std::filesystem::path& file, std::string_view dataset, uint64 bytes, Function&& function) -> decltype(function())
{
  ObserveCodecIoForTesting(file, dataset, CodecIoEventForTesting::TypedReadAttempt, bytes, 0);
  auto result = function();
  ObserveCodecIoForTesting(file, dataset, result >= 0 ? CodecIoEventForTesting::TypedReadCompleted : CodecIoEventForTesting::TypedReadFailed, bytes, 0);
  return result;
}

/**
 * @brief Checks the logical byte count for an observed typed-write selection.
 * @param dimensions Selected scalar counts, including component dimensions.
 * @param elementBytes Bytes in one native scalar.
 * @return Logical bytes, or no value when the count cannot be represented.
 */
inline std::optional<uint64> CheckedTypedWriteBytesForTesting(nonstd::span<const hsize_t> dimensions, uint64 elementBytes) noexcept
{
  if(elementBytes == 0)
  {
    return std::nullopt;
  }
  // A zero extent makes the selection empty even when an earlier product would overflow.
  for(const hsize_t extent : dimensions)
  {
    if(extent == 0)
    {
      return uint64{0};
    }
  }
  uint64 bytes = elementBytes;
  for(const hsize_t extent : dimensions)
  {
    if(bytes > std::numeric_limits<uint64>::max() / extent)
    {
      return std::nullopt;
    }
    bytes *= extent;
  }
  return bytes;
}

/**
 * @brief Runs one typed write and preserves its return value or exception.
 * @tparam Function Supplies the native write expression.
 * @param file Borrows the selected file identity.
 * @param dataset Borrows the selected dataset identity.
 * @param bytes Supplies checked logical bytes; absence invalidates the measurement only.
 * @param function Performs the unchanged native call exactly once.
 * @return The unchanged native status.
 */
template <class Function>
auto InvokeTypedWriteForTesting(const std::filesystem::path& file, std::string_view dataset, std::optional<uint64> bytes, Function&& function) -> decltype(function())
{
  if(!bytes)
  {
    ObserveCodecIoForTesting(file, dataset, CodecIoEventForTesting::TypedWriteSizeInvalid, 0, 0);
    return function();
  }
  ObserveCodecIoForTesting(file, dataset, CodecIoEventForTesting::TypedWriteAttempt, *bytes, 0);
  try
  {
    auto result = function();
    ObserveCodecIoForTesting(file, dataset, result >= 0 ? CodecIoEventForTesting::TypedWriteCompleted : CodecIoEventForTesting::TypedWriteFailed, *bytes, 0);
    return result;
  } catch(...)
  {
    ObserveCodecIoForTesting(file, dataset, CodecIoEventForTesting::TypedWriteFailed, *bytes, 0);
    throw;
  }
}
#endif

/**
 * @namespace nx::core::HDF5
 * @brief Contains HDF5 parsing utilities.
 */

/**
 * @class UnallocatedChunkError
 * @brief Signals that the codec cannot obtain raw chunk bytes.
 *
 * UnallocatedChunkError can represent an unallocated sparse chunk or a failed
 * HDF5 metadata query. The exception does not distinguish those causes.
 * An allocated-chunk raw-read or inflate failure uses std::runtime_error instead.
 */
class UnallocatedChunkError : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

/**
 * @class ParallelChunkCodec
 * @brief Reads and writes eligible deflate chunks with parallel off-lock work.
 *
 * The fast path requires one deflate filter and compatible host and file byte
 * order. Ineligible datasets use serial H5Dread fallback paths outside this class.
 * Direct raw reads bypass HDF5 conversion and filter handling.
 *
 * HDF5 metadata and raw-HDF5 calls hold Support::ApiLock(). POSIX raw reads use
 * shared positional I/O outside that lock. Windows uses H5Dread_chunk because
 * its HDF5 VFD holds the file range lock. zlib work remains outside the lock.
 *
 * Chunk tasks use the process-wide oneTBB scheduler when multicore support is
 * available. A serial loop preserves order when it is not. Operations wait for
 * scheduled work and do not accept cancellation.
 *
 * Read tasks preserve input chunk positions while they scatter disjoint byte
 * regions. Each inflate task owns one chunk buffer, so scheduler limits bound
 * concurrent inflation buffers. Deflate nominal preparation keeps at most 64
 * chunks in each batch. The 64 MiB target applies when one nominal chunk is no
 * larger than 64 MiB. A larger nominal chunk forms a one-chunk batch.
 * Batch commits stay serial because non-thread-safe HDF5 serializes writes.
 * When more than one batch is needed, preparation of one batch overlaps the
 * serial commit of the batch before it; both per-batch caps are halved in that
 * case so at most two batches' prepared bytes are resident at once.
 *
 * The caller must keep the HDF5 dataset handle valid and structurally unchanged
 * for the codec lifetime and each operation. The codec does not make generic
 * DataStore access safe for concurrent callers.
 */
class SIMPLNX_EXPORT ParallelChunkCodec
{
public:
  /**
   * @struct ChunkInfo
   * @brief Stores one raw-chunk metadata snapshot.
   */
  struct ChunkInfo
  {
    bool allocated = false;   // A false value can represent sparse storage or a failed HDF5 metadata query.
    uint32 filterMask = 0;    // HDF5 mask for filters skipped on this chunk.
    uint64 storedAddress = 0; // Raw chunk file offset.
    uint64 storedSize = 0;    // Raw chunk byte count.
  };

  /**
   * @brief Binds a codec to an open chunked dataset.
   * @param filePath Backing HDF5 file path for positional reads.
   * @param datasetPath HDF5 dataset path for diagnostics.
   * @param tupleShape Row-major tuple dimensions.
   * @param chunkShape Tuple-space chunk dimensions.
   * @param componentShape Trailing component dimensions that chunks do not split.
   * @param elementSize Bytes in one caller-buffer scalar element.
   * @param datasetId Open HDF5 dataset identifier that must outlive this codec.
   * @param memoryTypeId Exact caller-buffer datatype, borrowed only while the constructor probes eligibility.
   * @pre tupleShape and chunkShape have equal nonzero rank. All tuple and chunk
   * dimensions are nonzero.
   * @pre Each component dimension and elementSize are nonzero. An empty
   * componentShape represents one component.
   * @pre datasetId identifies an open HDF5 dataset.
   * @pre Component, nominal-chunk, and byte-count products fit usize. Each
   * tupleShape[d] + chunkShape[d] - 1 and the total chunk count fit uint64.
   *
   * The constructor probes exact raw datatype identity once and captures the deflate level. It does not retain memoryTypeId.
   * The caller must not hold Support::ApiLock() during construction.
   */
  ParallelChunkCodec(std::filesystem::path filePath, std::string datasetPath, std::vector<uint64> tupleShape, std::vector<uint64> chunkShape, std::vector<uint64> componentShape, usize elementSize,
                     hid_t datasetId, hid_t memoryTypeId);

  /**
   * @brief Releases the positional read handle.
   *
   * The caller must finish all codec operations before destruction.
   */
  ~ParallelChunkCodec();

  ParallelChunkCodec(const ParallelChunkCodec&) = delete;
  ParallelChunkCodec(ParallelChunkCodec&&) = delete;
  ParallelChunkCodec& operator=(const ParallelChunkCodec&) = delete;
  ParallelChunkCodec& operator=(ParallelChunkCodec&&) = delete;

  /**
   * @brief Reports whether this dataset qualifies for raw parallel processing.
   * @return True for one deflate filter, exact file/memory representation, and compatible byte order.
   */
  bool isEligible() const;
#if SIMPLNX_BUILD_TESTS
  /**
   * @brief Reports observable retained native/string/shape capacities.
   * @return Native-string bytes including terminator, dataset bytes including terminator, and three vector payloads.
   * @note GNU path component-list storage is admitted separately and is not observed by this receipt.
   */
  uint64 boundedMetadataCapacityBytesForTesting() const noexcept;
#endif

  /**
   * @brief Reads checked metadata for one full-rank physical chunk origin.
   * @param fullRankChunkOffset Supplies tuple and component coordinates.
   * @return A sparse/allocated record, or an invalid query/validation result.
   * @note An unreviewed diagnostic build returns invalid with empty element collections
   * before metadata work. Supported builds retain reason-coded errors. The caller
   * funds BoundedRead::EmptyInvalidResultControlBytes<ChunkInfo>() for mandatory
   * typed return/move controls; debug empty-container bookkeeping remains permitted.
   */
  Result<ChunkInfo> getChunkInfoChecked(nonstd::span<const uint64> fullRankChunkOffset) const;

  /**
   * @brief Reads full-value coordinates without exceeding admitted backend scratch.
   * @param fullValueExtent Selects tuple and component axes in row-major order.
   * @param destination Receives the exact selected scalar byte count.
   * @param scratchBudgetBytes Funds metadata, raw/decoded carriers, zlib and diagnostics.
   * @return True when complete; false before payload transfer; invalid on a real failure.
   * @note The caller funds mandatory empty Result controls outside scratchBudgetBytes.
   * Subminimum calls start no preparation, payload or diagnostic-text allocation.
   * Iterator-debug empty-vector bookkeeping retains its normal ABI behavior.
   */
  Result<bool> readExtentIntoBufferBounded(const Extent& fullValueExtent, nonstd::span<std::byte> destination, uint64 scratchBudgetBytes) const;

  /**
   * @brief Uses the outer store's diagnostic owner after its minimum is funded.
   * @param fullValueExtent Selects all value-space dimensions.
   * @param destination Receives selected scalar bytes.
   * @param payloadBudgetBytes Excludes the outer diagnostic allowance.
   * @param diagnostic Borrows the sole fixed diagnostic carrier.
   * @return Fixed state; payload scratch has been destroyed before return.
   */
  BoundedRead::Status readExtentIntoBufferBoundedState(const Extent& fullValueExtent, nonstd::span<std::byte> destination, uint64 payloadBudgetBytes, BoundedRead::Diagnostic& diagnostic) const;

  /**
   * @brief Inflates one chunk into an edge-clamped byte buffer.
   * @param flatChunkIndex Row-major logical chunk index.
   * @return In-bounds chunk bytes in row-major tuple and component order.
   * @throws UnallocatedChunkError if metadata does not supply raw chunk bytes.
   * @throws std::runtime_error for an allocated chunk raw-read or inflate failure.
   * @pre isEligible() is true and flatChunkIndex is valid.
   *
   * One metadata probe supplies allocation, size, and filter state. POSIX reads
   * raw bytes through positional I/O. Windows uses H5Dread_chunk. Inflation and
   * edge clamping run outside Support::ApiLock().
   *
   * A false allocation state can identify an unallocated sparse chunk or a
   * failed HDF5 metadata query.
   */
  std::vector<std::byte> inflateChunk(uint64 flatChunkIndex) const;

  /**
   * @brief Locates chunks under one HDF5 API lock.
   * @param flatChunkIndices Row-major logical chunk indices.
   * @return Metadata snapshots aligned with flatChunkIndices positions.
   * @pre isEligible() is true and all indices are valid.
   * @pre The dataset remains structurally unchanged until matching inflates finish.
   *
   * Workers can pass these snapshots to inflateChunk() and avoid repeated
   * metadata calls. The output preserves input order. A false allocation state
   * can identify an unallocated sparse chunk or a failed HDF5 metadata query.
   */
  std::vector<ChunkInfo> getChunkInfos(nonstd::span<const uint64> flatChunkIndices) const;

  /**
   * @brief Inflates a chunk from a metadata snapshot.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param chunkInfo Metadata from getChunkInfos() for flatChunkIndex.
   * @return In-bounds chunk bytes in row-major tuple and component order.
   * @throws UnallocatedChunkError if chunkInfo does not supply raw chunk bytes.
   * @throws std::runtime_error for a raw read or inflate failure.
   * @pre isEligible() is true and flatChunkIndex is valid.
   * @pre chunkInfo describes the same structurally unchanged dataset.
   *
   * A false allocation state can identify an unallocated sparse chunk or a
   * failed HDF5 metadata query.
   */
  std::vector<std::byte> inflateChunk(uint64 flatChunkIndex, const ChunkInfo& chunkInfo) const;

  /**
   * @brief Inflates selected chunks into a full-dataset byte span.
   * @param out Destination span for the full tuple and component layout.
   * @param flatChunkIndices Distinct row-major logical chunk indices.
   * @throws std::runtime_error if out is too small or a chunk read fails.
   * @pre isEligible() is true.
   * @pre flatChunkIndices contains distinct valid indices.
   * @pre Full-dataset tuple, component, and byte-count products fit usize. Flat
   * tuple indices fit uint64.
   *
   * Each task scatters one clamped chunk into a disjoint region of out. This
   * raw-byte contract does not make generic DataStore access concurrently safe.
   * Chunks without raw metadata use a serial fill-aware H5Dread under
   * Support::ApiLock().
   * Other worker failures are rethrown after scheduled tasks finish.
   */
  void inflateChunksIntoSpan(nonstd::span<std::byte> out, nonstd::span<const uint64> flatChunkIndices) const;

  /**
   * @brief Deflates selected chunks in bounded batches and commits them serially.
   * @param source Source tuple bytes beginning at sourceStartTuple.
   * @param flatChunkIndices Row-major logical chunk indices in desired commit order.
   * @param sourceStartTuple Global flat tuple index of source's first tuple.
   * @param firstErrorOut Optional first-failure diagnostic destination.
   * @return True when every requested chunk commits successfully.
   *
   * Worker tasks gather and deflate off the HDF5 lock. Each batch retains at
   * most 64 chunks, halved to 32 when more than one batch is needed. The 64 MiB
   * target applies the same halving rule; a nominal chunk at or above that size
   * is prepared alone. The calling thread commits prepared chunks in input
   * order under leaf H5Dwrite_chunk locks. When more than one batch is needed,
   * this call pipelines the two stages: the batch after the one committing
   * prepares in parallel while the commit runs, so at most two batches' worth
   * of prepared bytes are resident at once (the halved caps keep that peak at
   * the single-batch peak). Observing a failure from either stage stops new
   * preparation and further commits, while any preparation already running is
   * still joined before this call returns.
   *
   * source remains caller-owned and must stay valid until this call returns.
   * Duplicate indices perform repeated serial commits in input order. The method
   * validates source containment and reports validation, zlib, or HDF5 errors
   * through firstErrorOut.
   * @pre Chunk, source, and full-dataset byte-count products fit usize. Flat
   * tuple indices fit uint64.
   * @pre sourceStartTuple plus the tuple count represented by source fits uint64.
   */
  bool deflateSpanIntoChunks(nonstd::span<const std::byte> source, nonstd::span<const uint64> flatChunkIndices, uint64 sourceStartTuple = 0, std::string* firstErrorOut = nullptr) const;

  /**
   * @brief Compresses one nominal chunk with zlib outside Support::ApiLock().
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param nominalBytes Full padded chunk bytes.
   * @param errorOut Optional zlib failure diagnostic destination.
   * @return Compressed bytes, or an empty vector for a compression error.
   * @pre isEligible() is true.
   *
   * This is the serial entry point to the same compression worker implementation.
   */
  std::vector<std::byte> compressNominalChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* errorOut = nullptr) const;

  /**
   * @brief Writes pre-compressed bytes for one chunk under Support::ApiLock().
   * @param flatChunkIndex Row-major logical chunk index.
   * @param compressedBytes Deflate bytes that use this dataset's filter stream.
   * @param errorOut Optional HDF5 failure diagnostic destination.
   * @return True on success; false when H5Dwrite_chunk fails.
   * @pre isEligible() is true and flatChunkIndex is valid.
   *
   * Component dimensions remain unsplit, so the full-rank offset appends zeros.
   * Filter mask zero records a deflate-compressed chunk.
   */
  bool writeCompressedChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> compressedBytes, std::string* errorOut = nullptr) const;

  /**
   * @brief Adaptively writes one nominal chunk.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param nominalBytes Full padded chunk bytes.
   * @param errorOut Optional failure diagnostic destination.
   * @return True on success; false for invalid input, zlib, or HDF5 failure.
   *
   * A distributed trial avoids full compression for clearly incompressible data.
   * Raw chunks set filter-mask bit zero. Compressible chunks use the dataset
   * deflate stream. nominalBytes remains caller-owned during this call.
   */
  bool writeNominalChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* errorOut = nullptr) const;

  /**
   * @brief Reuses one adaptive representation for full repeated chunks.
   * @param nominalBytes Full padded chunk bytes.
   * @param flatChunkIndices Full row-major logical chunk indices.
   * @param errorOut Optional failure diagnostic destination.
   * @return True when every chunk writes successfully.
   *
   * The method validates full chunk bounds. The trial and optional compression
   * run once before serial writes in input order.
   */
  bool writeRepeatedNominalChunk(nonstd::span<const std::byte> nominalBytes, nonstd::span<const uint64> flatChunkIndices, std::string* errorOut = nullptr) const;

private:
  /**
   * @struct PreparedChunkBytes
   * @brief Stores one adaptive raw or deflate representation.
   */
  struct PreparedChunkBytes
  {
    std::vector<std::byte> filteredBytes; // Deflate bytes when filterMask is zero.
    uint32 filterMask = 0;                // Bit zero indicates raw bytes bypass deflate.
  };

  /**
   * @brief Inflates one allocated chunk from raw storage metadata.
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param bounds Clamped tuple-space chunk extent.
   * @param storedAddress Raw chunk file offset.
   * @param storedSize Raw chunk byte count.
   * @param filterMask Chunk filter state.
   * @return In-bounds chunk bytes in row-major tuple and component order.
   * @throws std::runtime_error for short raw reads or inflate failure.
   *
   * POSIX raw reads use positional I/O. Windows uses H5Dread_chunk under the
   * HDF5 API lock. Deflate inflation and edge clamping remain outside the lock.
   */
  std::vector<std::byte> inflateChunkFromInfo(uint64 flatChunkIndex, const Extent& bounds, uint64 storedAddress, uint64 storedSize, uint32 filterMask) const;

  /**
   * @brief Gathers one source region into a padded nominal chunk buffer.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param source Tuple bytes beginning at sourceStartTuple.
   * @param sourceStartTuple Global flat tuple index of source's first tuple.
   * @return Full padded chunk bytes in row-major tuple and component order.
   *
   * Edge padding remains zero. The gather merges compatible contiguous rows to
   * avoid one memcpy per tuple.
   */
  std::vector<std::byte> gatherChunkBytes(uint64 flatChunkIndex, nonstd::span<const std::byte> source, uint64 sourceStartTuple) const;

  /**
   * @brief Compresses nominal bytes with zlib outside Support::ApiLock().
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param nominalBytes Full padded chunk bytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return Compressed bytes, or an empty vector after a recorded failure.
   *
   * Parallel batches share one error destination. The public helper uses a
   * private destination and mutex.
   */
  std::vector<std::byte> compressChunkBytesImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const;

  /**
   * @brief Prepares raw or deflate bytes for one nominal chunk.
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param nominalBytes Full padded chunk bytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return Adaptive representation, or a default representation after failure.
   */
  PreparedChunkBytes prepareChunkBytesImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const;

  /**
   * @brief Prepares and writes one nominal chunk.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param nominalBytes Full padded chunk bytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return True on successful adaptive write.
   */
  bool writeNominalChunkImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const;

  /**
   * @brief Writes prepared raw bytes through H5Dwrite_chunk.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param storedBytes Raw bytes to store.
   * @param filterMask HDF5 filter state for storedBytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return True when H5Dwrite_chunk succeeds.
   *
   * The full-rank offset appends component zeros. Support::ApiLock() guards
   * only this leaf write.
   */
  bool writeCompressedChunkImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> storedBytes, uint32 filterMask, std::string* firstErrorOut, std::mutex& errorMutex) const;

#ifndef _WIN32
  /**
   * @brief Returns the lazy POSIX descriptor for positional reads.
   * @return Shared descriptor for positional reads.
   * @throws std::runtime_error if the backing file cannot open.
   *
   * pread does not change a shared file position, so workers reuse one descriptor.
   */
  detail::FileHandle getPositionalReadHandle() const;
#endif

  std::filesystem::path m_FilePath;
  std::string m_DatasetPath;
  std::vector<uint64> m_TupleShape;
  std::vector<uint64> m_ChunkShape;     // Tuple-space chunk dimensions.
  std::vector<uint64> m_ComponentShape; // Unsplittable trailing component dimensions.
  usize m_ElementSize = 1;
  hid_t m_DatasetId = H5I_INVALID_HID; // Borrowed HDF5 identifier; caller owns its lifetime.

  usize m_NumComponents = 1;        // Product of component dimensions.
  usize m_NominalChunkElements = 1; // Full padded tuple and component count.
  int32 m_DeflateLevel = 1;         // Eligibility-probe deflate level for compress2.
  uint64 m_NumChunks = 1;           // Total logical chunks for index validation.
  bool m_Eligible = false;
  // Stable predefined native identifier; never retains the constructor's borrowed type.
  hid_t m_BoundedNativeType = H5I_INVALID_HID;
  bool m_BoundedTypeQueryFailed = false;

#ifndef _WIN32
  // Serializes short-read recovery only. Successful positional reads remain lock free.
  mutable std::mutex m_PositionalReadRecoveryMutex;
  mutable std::once_flag m_ReadHandleOpenOnce;
  mutable detail::FileHandle m_ReadFileHandle = detail::invalidFileHandle();
#endif
};

} // namespace nx::core::HDF5

/**
 * @def SIMPLNX_OBSERVE_METADATA_CALL
 * @brief Preserves one native expression and observes it only in test builds.
 * @param event Names the metadata call family.
 * @param expression Supplies the native expression, evaluated exactly once.
 */
#if SIMPLNX_BUILD_TESTS
#define SIMPLNX_OBSERVE_METADATA_CALL(event, expression) nx::core::HDF5::InvokeMetadataForTesting(nx::core::HDF5::CodecIoEventForTesting::event, [&]() { return (expression); })
#else
#define SIMPLNX_OBSERVE_METADATA_CALL(event, expression) (expression)
#endif

/**
 * @def SIMPLNX_OBSERVE_TYPED_READ
 * @brief Preserves a typed read and observes its native boundary only in test builds.
 * @param file Borrows the file identity.
 * @param dataset Borrows the dataset identity.
 * @param bytes Supplies logical requested bytes.
 * @param expression Supplies H5Dread, evaluated exactly once.
 */
#if SIMPLNX_BUILD_TESTS
#define SIMPLNX_OBSERVE_TYPED_READ(file, dataset, bytes, expression) nx::core::HDF5::InvokeTypedReadForTesting(file, dataset, bytes, [&]() { return (expression); })
#else
#define SIMPLNX_OBSERVE_TYPED_READ(file, dataset, bytes, expression) (expression)
#endif

/**
 * @def SIMPLNX_OBSERVE_TYPED_WRITE
 * @brief Preserves a typed write and observes its native boundary only in test builds.
 * @param file Borrows the file identity.
 * @param dataset Borrows the dataset identity.
 * @param bytes Supplies checked logical bytes.
 * @param expression Supplies H5Dwrite, evaluated exactly once.
 */
#if SIMPLNX_BUILD_TESTS
#define SIMPLNX_OBSERVE_TYPED_WRITE(file, dataset, bytes, expression) nx::core::HDF5::InvokeTypedWriteForTesting(file, dataset, bytes, [&]() { return (expression); })
#else
#define SIMPLNX_OBSERVE_TYPED_WRITE(file, dataset, bytes, expression) (expression)
#endif
