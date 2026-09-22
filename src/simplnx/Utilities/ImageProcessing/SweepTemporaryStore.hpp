#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace nx::core::ImageProcessing::detail
{
struct Sweep2DPlan
{
  bool fullWidth = true;
  usize coreRows = 0;
  usize coreColumns = 0;
};

/**
 * @brief Plans bounded full-width row blocks or overwide X tiles for a 2-D sweep.
 *
 * @p fullWidthHaloRows is the maximum number of complete halo rows held with one core block. @p tiledRows is the
 * number of `(coreColumns + 2)` rows held in the largest overwide tile buffer.
 */
inline Result<Sweep2DPlan> CreateSweep2DPlan(usize dimX, usize dimY, usize maxBufferValues, usize fullWidthHaloRows, usize tiledRows)
{
  if(dimX == 0 || dimY == 0)
  {
    return {Sweep2DPlan{true, 0, 0}};
  }
  if(maxBufferValues == 0 || tiledRows == 0 || fullWidthHaloRows == std::numeric_limits<usize>::max())
  {
    return MakeErrorResult<Sweep2DPlan>(-8684, "The bounded 2-D sweep requires a nonzero buffer budget and tile-row count.");
  }

  const usize minimumFullWidthRows = fullWidthHaloRows + 1;
  if(dimX <= maxBufferValues / minimumFullWidthRows)
  {
    const usize rowCapacity = maxBufferValues / dimX;
    const usize coreRows = std::min(dimY, rowCapacity - fullWidthHaloRows);
    return {Sweep2DPlan{true, coreRows, dimX}};
  }

  const usize valuesPerTileRow = maxBufferValues / tiledRows;
  if(valuesPerTileRow <= 2)
  {
    return MakeErrorResult<Sweep2DPlan>(-8684,
                                        fmt::format("The bounded 2-D sweep buffer holds {} values, but {} tile rows require room for one core value and two X halos.", maxBufferValues, tiledRows));
  }
  return {Sweep2DPlan{false, 1, std::min(dimX, valuesPerTileRow - 2)}};
}

/**
 * @brief Bulk-only fixed-capacity resident work store for images that fit a compile-time-bounded OOC fast path.
 *
 * Allocation size is @p capacityValues, independent of the logical image size. No element accessor is exposed.
 */
template <class T>
class BoundedMemorySweepStore
{
public:
  static Result<std::unique_ptr<BoundedMemorySweepStore<T>>> Create(usize logicalValues, usize capacityValues)
  {
    if(capacityValues == 0 || logicalValues > capacityValues)
    {
      return MakeErrorResult<std::unique_ptr<BoundedMemorySweepStore<T>>>(
          -8685, fmt::format("The bounded-memory sweep requested {} logical values from a {}-value fixed capacity.", logicalValues, capacityValues));
    }
    try
    {
      return {std::unique_ptr<BoundedMemorySweepStore<T>>(new BoundedMemorySweepStore<T>(logicalValues, capacityValues))};
    } catch(const std::exception& exception)
    {
      return MakeErrorResult<std::unique_ptr<BoundedMemorySweepStore<T>>>(-8685, fmt::format("The bounded-memory sweep could not allocate its fixed capacity: {}", exception.what()));
    }
  }

  usize getSize() const
  {
    return m_LogicalValues;
  }

  std::optional<ShapeType> getChunkShape() const
  {
    return std::nullopt;
  }

  Result<> copyIntoBuffer(usize valueOffset, nonstd::span<T> values) const
  {
    if(valueOffset > m_LogicalValues || values.size() > m_LogicalValues - valueOffset)
    {
      return MakeErrorResult(-8685, fmt::format("The bounded-memory sweep read at offset {} with {} values exceeds its {}-value logical range.", valueOffset, values.size(), m_LogicalValues));
    }
    std::copy_n(m_Values.get() + valueOffset, values.size(), values.data());
    return {};
  }

  Result<> copyFromBuffer(usize valueOffset, nonstd::span<const T> values)
  {
    if(valueOffset > m_LogicalValues || values.size() > m_LogicalValues - valueOffset)
    {
      return MakeErrorResult(-8685, fmt::format("The bounded-memory sweep write at offset {} with {} values exceeds its {}-value logical range.", valueOffset, values.size(), m_LogicalValues));
    }
    std::copy_n(values.data(), values.size(), m_Values.get() + valueOffset);
    return {};
  }

private:
  BoundedMemorySweepStore(usize logicalValues, usize capacityValues)
  : m_LogicalValues(logicalValues)
  , m_Values(std::make_unique<T[]>(capacityValues))
  {
  }

  usize m_LogicalValues = 0;
  std::unique_ptr<T[]> m_Values;
};

template <class ResultT>
std::string DescribeSweepTemporaryStoreError(const Result<ResultT>& result)
{
  if(result.errors().empty())
  {
    return "provider returned an unspecified error";
  }
  const Error& error = result.errors().front();
  return fmt::format("{} (provider code {})", error.message, error.code);
}

/**
 * @brief Typed bulk-I/O view over an owning fixed-record store used by iterative image sweeps.
 *
 * The class deliberately exposes no element accessor. Transfers larger than the provider batch limit are split into
 * bounded contiguous calls.
 */
template <class T>
class SweepTemporaryStore
{
public:
  SweepTemporaryStore(std::unique_ptr<ITemporaryRecordStore> store, const std::atomic_bool& shouldCancel, std::string context)
  : m_Store(std::move(store))
  , m_ShouldCancel(shouldCancel)
  , m_Context(std::move(context))
  {
  }

  usize getSize() const
  {
    return static_cast<usize>(m_Store->recordCount());
  }

  std::optional<ShapeType> getChunkShape() const
  {
    return std::nullopt;
  }

  Result<> copyIntoBuffer(usize valueOffset, nonstd::span<T> values) const
  {
    if(Result<> validation = validateTransfer(valueOffset, values.size(), "read"); validation.invalid())
    {
      return validation;
    }
    usize copied = 0;
    while(copied < values.size())
    {
      const usize count = std::min<usize>(values.size() - copied, static_cast<usize>(m_Store->maxRecordsPerBatch()));
      nonstd::span<T> batch = values.subspan(copied, count);
      nonstd::span<std::byte> bytes(reinterpret_cast<std::byte*>(batch.data()), batch.size() * sizeof(T));
      try
      {
        auto result = m_Store->read(static_cast<uint64>(valueOffset + copied), static_cast<uint64>(count), bytes, m_ShouldCancel);
        if(result.invalid())
        {
          return MakeErrorResult(-8681, fmt::format("{} fixed-record scratch read failed: {}", m_Context, DescribeSweepTemporaryStoreError(result)));
        }
        if(result.value() != count)
        {
          return MakeErrorResult(-8681, fmt::format("{} fixed-record scratch read returned {} of {} values.", m_Context, result.value(), count));
        }
      } catch(const std::exception& exception)
      {
        return MakeErrorResult(-8681, fmt::format("{} fixed-record scratch read failed: {}", m_Context, exception.what()));
      }
      copied += count;
    }
    return {};
  }

  Result<> copyFromBuffer(usize valueOffset, nonstd::span<const T> values)
  {
    if(Result<> validation = validateTransfer(valueOffset, values.size(), "write"); validation.invalid())
    {
      return validation;
    }
    usize copied = 0;
    while(copied < values.size())
    {
      const usize count = std::min<usize>(values.size() - copied, static_cast<usize>(m_Store->maxRecordsPerBatch()));
      nonstd::span<const T> batch = values.subspan(copied, count);
      nonstd::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(batch.data()), batch.size() * sizeof(T));
      try
      {
        Result<> result = m_Store->write(static_cast<uint64>(valueOffset + copied), static_cast<uint64>(count), bytes, m_ShouldCancel);
        if(result.invalid())
        {
          return MakeErrorResult(-8682, fmt::format("{} fixed-record scratch write failed: {}", m_Context, DescribeSweepTemporaryStoreError(result)));
        }
      } catch(const std::exception& exception)
      {
        return MakeErrorResult(-8682, fmt::format("{} fixed-record scratch write failed: {}", m_Context, exception.what()));
      }
      copied += count;
    }
    return {};
  }

private:
  Result<> validateTransfer(usize valueOffset, usize valueCount, std::string_view operation) const
  {
    if(m_Store == nullptr || m_Store->recordSize() != sizeof(T) || m_Store->maxRecordsPerBatch() == 0)
    {
      return MakeErrorResult(-8683, fmt::format("{} fixed-record scratch {} has invalid provider metadata.", m_Context, operation));
    }
    const uint64 recordOffset = static_cast<uint64>(valueOffset);
    const uint64 recordCount = static_cast<uint64>(valueCount);
    if(recordOffset > m_Store->recordCount() || recordCount > m_Store->recordCount() - recordOffset)
    {
      return MakeErrorResult(-8683,
                             fmt::format("{} fixed-record scratch {} at offset {} with {} values exceeds the {}-value store.", m_Context, operation, valueOffset, valueCount, m_Store->recordCount()));
    }
    return {};
  }

  std::unique_ptr<ITemporaryRecordStore> m_Store;
  const std::atomic_bool& m_ShouldCancel;
  std::string m_Context;
};

template <class T>
Result<std::unique_ptr<SweepTemporaryStore<T>>> CreateSweepTemporaryStore(usize valueCount, usize maxValuesPerBatch, const std::atomic_bool& shouldCancel, std::string_view context)
{
  if(maxValuesPerBatch == 0)
  {
    return MakeErrorResult<std::unique_ptr<SweepTemporaryStore<T>>>(-8680, fmt::format("{} fixed-record scratch requires a nonzero transfer batch.", context));
  }

  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(T);
  config.maxRecordsPerBatch = static_cast<uint64>(maxValuesPerBatch);
  config.initialRecordCount = static_cast<uint64>(valueCount);
  try
  {
    auto result = DataStoreUtilities::CreateTemporaryRecordStore(config);
    if(result.invalid())
    {
      return MakeErrorResult<std::unique_ptr<SweepTemporaryStore<T>>>(-8680, fmt::format("{} failed to create fixed-record scratch: {}", context, DescribeSweepTemporaryStoreError(result)));
    }
    std::unique_ptr<ITemporaryRecordStore> store = std::move(result.value());
    if(store == nullptr)
    {
      return MakeErrorResult<std::unique_ptr<SweepTemporaryStore<T>>>(-8680, fmt::format("{} failed to create fixed-record scratch: provider returned a null store", context));
    }
    if(store->recordSize() != config.recordSize || store->recordCount() != config.initialRecordCount || store->maxRecordsPerBatch() != config.maxRecordsPerBatch)
    {
      return MakeErrorResult<std::unique_ptr<SweepTemporaryStore<T>>>(-8680, fmt::format("{} failed to create fixed-record scratch: provider returned mismatched store metadata", context));
    }
    return {std::make_unique<SweepTemporaryStore<T>>(std::move(store), shouldCancel, std::string(context))};
  } catch(const std::exception& exception)
  {
    return MakeErrorResult<std::unique_ptr<SweepTemporaryStore<T>>>(-8680, fmt::format("{} failed to create fixed-record scratch: {}", context, exception.what()));
  }
}
} // namespace nx::core::ImageProcessing::detail
