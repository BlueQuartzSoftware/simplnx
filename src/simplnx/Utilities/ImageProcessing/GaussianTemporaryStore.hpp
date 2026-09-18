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
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace nx::core::ImageProcessing::detail
{
template <class ResultT>
std::string DescribeGaussianTemporaryStoreError(const Result<ResultT>& result)
{
  if(result.errors().empty())
  {
    return "provider returned an unspecified error";
  }
  const Error& error = result.errors().front();
  return fmt::format("{} (provider code {})", error.message, error.code);
}

/**
 * @brief Typed bulk-I/O view over an owning fixed-record temporary store.
 *
 * This deliberately exposes no element accessor. Transfers larger than the provider's configured batch are split into
 * bounded contiguous calls.
 */
template <class T>
class GaussianTemporaryStore
{
public:
  GaussianTemporaryStore(std::unique_ptr<ITemporaryRecordStore> store, const std::atomic_bool& shouldCancel)
  : m_Store(std::move(store))
  , m_ShouldCancel(shouldCancel)
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
          return MakeErrorResult(-23610, fmt::format("Gaussian fixed-record scratch read failed: {}", DescribeGaussianTemporaryStoreError(result)));
        }
        if(result.value() != count)
        {
          return MakeErrorResult(-23610, fmt::format("Gaussian fixed-record scratch read returned {} of {} values.", result.value(), count));
        }
      } catch(const std::exception& exception)
      {
        return MakeErrorResult(-23610, fmt::format("Gaussian fixed-record scratch read failed: {}", exception.what()));
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
          return MakeErrorResult(-23611, fmt::format("Gaussian fixed-record scratch write failed: {}", DescribeGaussianTemporaryStoreError(result)));
        }
      } catch(const std::exception& exception)
      {
        return MakeErrorResult(-23611, fmt::format("Gaussian fixed-record scratch write failed: {}", exception.what()));
      }
      copied += count;
    }
    return {};
  }

private:
  Result<> validateTransfer(usize valueOffset, usize valueCount, const char* operation) const
  {
    if(m_Store == nullptr || m_Store->recordSize() != sizeof(T) || m_Store->maxRecordsPerBatch() == 0)
    {
      return MakeErrorResult(-23612, fmt::format("Gaussian fixed-record scratch {} has invalid provider metadata.", operation));
    }
    const uint64 recordOffset = static_cast<uint64>(valueOffset);
    const uint64 recordCount = static_cast<uint64>(valueCount);
    if(recordOffset > m_Store->recordCount() || recordCount > m_Store->recordCount() - recordOffset)
    {
      return MakeErrorResult(-23612,
                             fmt::format("Gaussian fixed-record scratch {} at offset {} with {} values exceeds the {}-value store.", operation, valueOffset, valueCount, m_Store->recordCount()));
    }
    return {};
  }

  std::unique_ptr<ITemporaryRecordStore> m_Store;
  const std::atomic_bool& m_ShouldCancel;
};

template <class T>
Result<std::unique_ptr<GaussianTemporaryStore<T>>> CreateGaussianTemporaryStore(usize valueCount, usize maxValuesPerBatch, const std::atomic_bool& shouldCancel)
{
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(T);
  config.maxRecordsPerBatch = static_cast<uint64>(maxValuesPerBatch);
  config.initialRecordCount = static_cast<uint64>(valueCount);
  try
  {
    auto result = DataStoreUtilities::CreateTemporaryRecordStore(config);
    if(result.invalid())
    {
      return MakeErrorResult<std::unique_ptr<GaussianTemporaryStore<T>>>(-23609, fmt::format("Gaussian failed to create fixed-record scratch: {}", DescribeGaussianTemporaryStoreError(result)));
    }
    std::unique_ptr<ITemporaryRecordStore> store = std::move(result.value());
    if(store == nullptr)
    {
      return MakeErrorResult<std::unique_ptr<GaussianTemporaryStore<T>>>(-23609, "Gaussian failed to create fixed-record scratch: provider returned a null store");
    }
    if(store->recordSize() != config.recordSize || store->recordCount() != config.initialRecordCount || store->maxRecordsPerBatch() != config.maxRecordsPerBatch)
    {
      return MakeErrorResult<std::unique_ptr<GaussianTemporaryStore<T>>>(-23609, "Gaussian failed to create fixed-record scratch: provider returned mismatched store metadata");
    }
    return {std::make_unique<GaussianTemporaryStore<T>>(std::move(store), shouldCancel)};
  } catch(const std::exception& exception)
  {
    return MakeErrorResult<std::unique_ptr<GaussianTemporaryStore<T>>>(-23609, fmt::format("Gaussian failed to create fixed-record scratch: {}", exception.what()));
  }
}
} // namespace nx::core::ImageProcessing::detail
