#pragma once

#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"

#include <cstring>
#include <new>
#include <stdexcept>
#include <vector>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{

/**
 * @class InMemoryTemporaryRecordStore
 * @brief Stores fixed-width temporary records in resident memory.
 *
 * Resident and file-backed providers use the same bounded-transfer contract.
 * An algorithm can use this fallback only when record-count memory is permitted.
 * A path that requires external scratch must fail if no external provider exists.
 *
 * The object owns its byte vector and is not thread-safe.
 */
class InMemoryTemporaryRecordStore : public ITemporaryRecordStore
{
public:
  /**
   * @brief Validates the fixed-record configuration and allocates the initial resident byte range.
   * @param config Specifies record width, batch limit, initial count, and read-only state.
   * @return An owned store or a configuration, size-overflow, or allocation error.
   */
  static Result<std::unique_ptr<InMemoryTemporaryRecordStore>> Create(TemporaryRecordStoreConfig config)
  {
    if(config.recordSize == 0 || config.maxRecordsPerBatch == 0 || config.initialRecordCount > std::numeric_limits<usize>::max() / config.recordSize ||
       config.maxRecordsPerBatch > std::numeric_limits<usize>::max() / config.recordSize)
    {
      return MakeErrorResult<std::unique_ptr<InMemoryTemporaryRecordStore>>(-6045, "Invalid in-memory temporary record-store configuration");
    }
    try
    {
      return {std::unique_ptr<InMemoryTemporaryRecordStore>(new InMemoryTemporaryRecordStore(config))};
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult<std::unique_ptr<InMemoryTemporaryRecordStore>>(-6045, "In-memory temporary record-store allocation failed");
    } catch(const std::length_error&)
    {
      return MakeErrorResult<std::unique_ptr<InMemoryTemporaryRecordStore>>(-6045, "In-memory temporary record-store allocation failed");
    }
  }
  uint64 recordSize() const override
  {
    return m_Config.recordSize;
  }
  uint64 recordCount() const override
  {
    return m_Count;
  }
  uint64 maxRecordsPerBatch() const override
  {
    return m_Config.maxRecordsPerBatch;
  }
  bool isReadOnly() const override
  {
    return m_Config.readOnly;
  }
  /**
   * @brief Reads a bounded record range into caller-owned storage.
   * @param offset Zero-based first record.
   * @param count Number of requested records, bounded by maxRecordsPerBatch().
   * @param bytes Receives at least the requested fixed-width record bytes.
   * @param cancel Cancellation flag.
   * @return Number of records read, or a cancellation, range, batch, overflow, or buffer-size error.
   */
  Result<uint64> read(uint64 offset, uint64 count, nonstd::span<std::byte> bytes, const std::atomic_bool& cancel) const override
  {
    if(auto r = validate(offset, count, bytes.size(), false, cancel); r.invalid())
      return ConvertResultTo<uint64>(std::move(r), uint64{});
    if(count == 0)
    {
      return {0};
    }
    std::memcpy(bytes.data(), m_Bytes.data() + offset * m_Config.recordSize, count * m_Config.recordSize);
    return {count};
  }
  /**
   * @brief Writes a bounded range from caller-owned storage.
   * @param offset Zero-based first record.
   * @param count Number of records, bounded by maxRecordsPerBatch().
   * @param bytes Contains exactly the requested fixed-width record bytes.
   * @param cancel Cancellation flag.
   * @return Valid result or a read-only, cancellation, range, batch, overflow, or buffer-size error.
   */
  Result<> write(uint64 offset, uint64 count, nonstd::span<const std::byte> bytes, const std::atomic_bool& cancel) override
  {
    if(m_Config.readOnly)
      return MakeErrorResult(-6046, "Temporary record store is read-only");
    if(auto r = validate(offset, count, bytes.size(), true, cancel); r.invalid())
      return r;
    if(count == 0)
    {
      return {};
    }
    std::memcpy(m_Bytes.data() + offset * m_Config.recordSize, bytes.data(), bytes.size());
    return {};
  }
  /**
   * @brief Repeats one complete record across a logical range.
   * @param offset Zero-based first record.
   * @param count Number of records to fill.
   * @param record Contains one complete fixed-width record.
   * @param cancel Cancellation flag.
   * @return Valid result or a read-only, cancellation, range, overflow, or record-width error.
   */
  Result<> fill(uint64 offset, uint64 count, nonstd::span<const std::byte> record, const std::atomic_bool& cancel) override
  {
    if(record.size() != m_Config.recordSize)
      return MakeErrorResult(-6047, "Temporary record fill width mismatch");
    if(m_Config.readOnly)
    {
      return MakeErrorResult(-6046, "Temporary record store is read-only");
    }
    if(auto r = validate(offset, count, count * m_Config.recordSize, true, cancel, false); r.invalid())
      return r;
    for(uint64 i = 0; i < count; i++)
      std::memcpy(m_Bytes.data() + (offset + i) * m_Config.recordSize, record.data(), static_cast<usize>(m_Config.recordSize));
    return {};
  }
  /**
   * @brief Changes the logical record count without changing record width.
   * @param count New logical record count.
   * @param cancel Cancellation flag.
   * @return Valid result or a read-only, cancellation, overflow, or allocation error.
   */
  Result<> resize(uint64 count, const std::atomic_bool& cancel) override
  {
    if(m_Config.readOnly)
      return MakeErrorResult(-6046, "Temporary record store is read-only");
    if(cancel || count > std::numeric_limits<usize>::max() / m_Config.recordSize)
      return MakeErrorResult(-6048, "Temporary record resize failed");
    try
    {
      m_Bytes.resize(static_cast<usize>(count * m_Config.recordSize));
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult(-6048, "In-memory temporary record-store resize allocation failed");
    } catch(const std::length_error&)
    {
      return MakeErrorResult(-6048, "In-memory temporary record-store resize allocation failed");
    }
    m_Count = count;
    return {};
  }

private:
  /**
   * @brief Constructs a store after Create() validates all size products.
   * @param config Specifies the validated store configuration.
   */
  explicit InMemoryTemporaryRecordStore(TemporaryRecordStoreConfig config)
  : m_Config(config)
  , m_Count(config.initialRecordCount)
  , m_Bytes(static_cast<usize>(config.initialRecordCount * config.recordSize))
  {
  }
  /**
   * @brief Validates one transfer request.
   * @param offset Zero-based first record.
   * @param count Number of requested records.
   * @param bytes Number of caller-buffer bytes.
   * @param exact Requires bytes to equal the requested byte count when true.
   * @param cancel Cancellation flag.
   * @param enforceBatch Applies maxRecordsPerBatch() when true.
   * @return Valid result or a cancellation, range, batch, overflow, or buffer-size error.
   */
  Result<> validate(uint64 offset, uint64 count, usize bytes, bool exact, const std::atomic_bool& cancel, bool enforceBatch = true) const
  {
    if(cancel || (enforceBatch && count > m_Config.maxRecordsPerBatch) || offset > m_Count || count > m_Count - offset || count > std::numeric_limits<usize>::max() / m_Config.recordSize ||
       (exact ? bytes != count * m_Config.recordSize : bytes < count * m_Config.recordSize))
      return MakeErrorResult(-6049, "Invalid temporary record-store request");
    return {};
  }
  TemporaryRecordStoreConfig m_Config;
  uint64 m_Count;
  std::vector<std::byte> m_Bytes;
};
} // namespace nx::core
