#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <atomic>
#include <cstddef>
#include <memory>

#include <nonstd/span.hpp>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{

/**
 * @struct TemporaryRecordStoreConfig
 * @brief Configures a storage-neutral, fixed-width algorithm scratch store.
 */
struct SIMPLNX_EXPORT TemporaryRecordStoreConfig
{
  /**
   * @brief Specifies the exact byte width of each logical record.
   */
  uint64 recordSize = 0;
  /**
   * @brief Limits the record count in one read or write call.
   */
  uint64 maxRecordsPerBatch = 0;
  /**
   * @brief Specifies the initial logical capacity in records.
   */
  uint64 initialRecordCount = 0;
  /**
   * @brief Requires write, fill, and resize requests to fail when true.
   */
  bool readOnly = false;
};

/**
 * @class ITemporaryRecordStore
 * @brief Provides storage-neutral scratch storage for fixed-width records.
 *
 * All transfers use caller-owned buffers. read() and write() accept at most
 * maxRecordsPerBatch() records. fill() can cover a larger range but uses bounded
 * internal transfers. Implementations reject overflow, invalid ranges, and buffer-size mismatches.
 *
 * The interface does not guarantee concurrent access. A caller must serialize
 * calls unless the concrete implementation documents a stronger contract.
 */
class SIMPLNX_EXPORT ITemporaryRecordStore
{
public:
  /**
   * @brief Destroys the temporary record store.
   */
  virtual ~ITemporaryRecordStore() noexcept = default;

  virtual uint64 recordSize() const = 0;
  virtual uint64 recordCount() const = 0;
  virtual uint64 maxRecordsPerBatch() const = 0;
  virtual bool isReadOnly() const = 0;

  /**
   * @brief Reads a bounded record range into caller-owned storage.
   * @param recordOffset Zero-based first record.
   * @param requestedRecordCount Maximum records requested, bounded by maxRecordsPerBatch().
   * @param records Output bytes large enough for the requested fixed-width records.
   * @param shouldCancel Cancellation flag checked by the implementation.
   * @return The number of records read, or a range, size, storage, or cancellation error.
   */
  virtual Result<uint64> read(uint64 recordOffset, uint64 requestedRecordCount, nonstd::span<std::byte> records, const std::atomic_bool& shouldCancel) const = 0;
  /**
   * @brief Writes a bounded range of complete fixed-width records.
   * @param recordOffset Zero-based first record.
   * @param recordCount Number of records, bounded by maxRecordsPerBatch().
   * @param records Contains exactly the requested fixed-width record bytes.
   * @param shouldCancel Cancellation flag checked by the implementation.
   * @return A valid result or a read-only, range, size, storage, or cancellation error.
   */
  virtual Result<> write(uint64 recordOffset, uint64 recordCount, nonstd::span<const std::byte> records, const std::atomic_bool& shouldCancel) = 0;
  /**
   * @brief Repeats one complete record across a logical range using internally bounded transfers.
   * @param recordOffset Zero-based first record.
   * @param recordCount Number of records to fill.
   * @param record Contains one complete fixed-width record.
   * @param shouldCancel Cancellation flag checked by the implementation.
   * @return A valid result or a read-only, range, record-width, storage, or cancellation error.
   */
  virtual Result<> fill(uint64 recordOffset, uint64 recordCount, nonstd::span<const std::byte> record, const std::atomic_bool& shouldCancel) = 0;
  /**
   * @brief Changes the logical record count without changing record width.
   * @param recordCount New logical record count.
   * @param shouldCancel Cancellation flag checked by the implementation.
   * @return A valid result or a read-only, capacity, storage, or cancellation error.
   */
  virtual Result<> resize(uint64 recordCount, const std::atomic_bool& shouldCancel) = 0;
};
} // namespace nx::core
