#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>

#include <nonstd/span.hpp>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
/**
 * @brief Compares two complete fixed-width records.
 *
 * A negative value orders left first. Zero preserves append order. A positive
 * value orders right first.
 */
using ExternalSortCompare = std::function<int32(nonstd::span<const std::byte> left, nonstd::span<const std::byte> right)>;

/**
 * @brief Reports operation-local completed and total record counts.
 *
 * Implementations do not report a completed count greater than the total.
 */
using ExternalSortProgressCallback = std::function<void(uint64 completedRecords, uint64 totalRecords)>;

/**
 * @struct ExternalSortConfig
 * @brief Configures fixed-width stable external sorting.
 */
struct SIMPLNX_EXPORT ExternalSortConfig
{
  /**
   * @brief Specifies the exact byte width of each record.
   */
  uint64 recordSize = 0;
  /**
   * @brief Limits the record count in one append or read request.
   */
  uint64 maxRecordsPerBatch = 0;
  /**
   * @brief Defines deterministic ordering. Equal records retain append order.
   */
  ExternalSortCompare compare;
};

/**
 * @class IExternalSort
 * @brief Provides a stable external sort for fixed-width records.
 *
 * Callers append bounded batches and then call finish() before read(). Reads can
 * repeat. An append or finish cancellation makes the instance terminal. A read
 * cancellation permits retry. Implementations keep resident storage bounded.
 *
 * The interface does not guarantee concurrent access. Callers must serialize operations.
 */
class SIMPLNX_EXPORT IExternalSort
{
public:
  /**
   * @brief Destroys the external sort.
   */
  virtual ~IExternalSort() noexcept = default;

  /**
   * @brief Appends unsorted records before finish() is called.
   * @param recordCount Number of complete records in records.
   * @param records Caller-owned bytes containing exactly recordCount fixed-width records.
   * @param shouldCancel Cancellation flag. Cancellation makes the append and finish lifecycle terminal.
   * @param progressCallback Optional operation-local progress callback.
   * @return A valid result or a configuration, size, storage, lifecycle, or cancellation error.
   */
  virtual Result<> append(uint64 recordCount, nonstd::span<const std::byte> records, const std::atomic_bool& shouldCancel, const ExternalSortProgressCallback& progressCallback) = 0;
  /**
   * @brief Completes stable sorting and transitions the instance to repeatable read mode.
   * @param shouldCancel Cancellation flag. A cancelled finish invalidates the instance.
   * @param progressCallback Optional operation-local progress callback.
   * @return A valid result or the sort, provider, or cancellation error.
   */
  virtual Result<> finish(const std::atomic_bool& shouldCancel, const ExternalSortProgressCallback& progressCallback) = 0;
  /**
   * @brief Reads a bounded range from the completed sorted stream.
   * @param recordOffset Zero-based sorted record offset.
   * @param recordCount Maximum number of requested records.
   * @param records Caller-owned output buffer for recordCount records.
   * @param shouldCancel Cancellation flag. A cancelled read permits retry.
   * @return Number of records read, or an error. Only end-of-stream reads can be short.
   */
  virtual Result<uint64> read(uint64 recordOffset, uint64 recordCount, nonstd::span<std::byte> records, const std::atomic_bool& shouldCancel) const = 0;

  virtual uint64 recordCount() const = 0;
};
} // namespace nx::core
