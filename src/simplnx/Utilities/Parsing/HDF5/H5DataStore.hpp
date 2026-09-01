#pragma once

#include "H5Support.hpp"

#include "simplnx/DataStructure/IDataStore.hpp"

#include <algorithm>
#include <atomic>
#include <limits>

namespace nx::core::HDF5
{
namespace Support
{
/**
 * @brief Reads an HDF5 dataset region directly into an in-memory DataStore.
 * @tparam T Specifies the numeric value type.
 * @param dataArray Supplies the destination array.
 * @param dataArrayPath Identifies the destination for diagnostics.
 * @param datasetReader Supplies the open HDF5 dataset.
 * @param start Specifies an optional region origin.
 * @param count Specifies optional region dimensions.
 * @return Valid result or an HDF5 read or store-type error.
 * @pre dataArray uses DataStore<T>. start and count are both present or both absent.
 * @pre A selected region contains the destination value count.
 *
 * DatasetIO owns HDF5 API synchronization. This function performs one read and
 * does not provide cancellation.
 */
template <typename T>
Result<> FillDataStore(DataArray<T>& dataArray, const DataPath& dataArrayPath, const nx::core::HDF5::DatasetIO& datasetReader, const std::optional<std::vector<hsize_t>>& start = std::nullopt,
                       const std::optional<std::vector<hsize_t>>& count = std::nullopt)
{
  try
  {
    using StoreType = DataStore<T>;
    StoreType& dataStore = dataArray.template getIDataStoreRefAs<StoreType>();
    auto dataSpan = dataStore.createSpan();
    Result<> result;
    if(start.has_value() && count.has_value())
    {
      std::vector<uint64> startVec(start->begin(), start->end());
      std::vector<uint64> countVec(count->begin(), count->end());
      result = datasetReader.readIntoSpan<T>(dataSpan, startVec, countVec);
    }
    else
    {
      result = datasetReader.readIntoSpan<T>(dataSpan);
    }
    if(result.invalid())
    {
      return MakeErrorResult(-21002,
                             fmt::format("Error reading dataset '{}' with '{}' total elements into data store for data array '{}' with '{}' total elements ('{}' tuples and '{}' components):\n\n{}",
                                         dataArrayPath.getTargetName(), datasetReader.getNumElements(), dataArrayPath.toString(), dataArray.getSize(), dataArray.getNumberOfTuples(),
                                         dataArray.getNumberOfComponents(), result.errors()[0].message));
    }
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-21003, e.what());
  }

  return {};
}

/**
 * @brief Streams an HDF5 dataset region into an OOC data store.
 * @tparam T Specifies a non-boolean numeric value type.
 * @param dataArray Supplies the destination array.
 * @param dataArrayPath Identifies the destination for diagnostics.
 * @param datasetReader Supplies the open HDF5 dataset.
 * @param start Specifies optional leading region coordinates.
 * @param count Specifies optional leading region dimensions.
 * @param shouldCancel Supplies optional cancellation state.
 * @return Valid result, validation error, HDF5 read error, or store write error.
 * @pre dataArray uses an OOC AbstractDataStore<T>.
 * @pre Extra start or count entries beyond the dataset rank are absent.
 * @pre The selected region value count equals the destination store size.
 *
 * Missing start coordinates default to zero. Missing count dimensions extend
 * to the dataset boundary. The C-order decomposition selects contiguous regions
 * of at most 65,536 values. It supports a row whose full trailing extent exceeds
 * that target by batching along an inner dimension.
 *
 * Cancellation returns a valid result and preserves completed destination ranges.
 * DatasetIO serializes its HDF5 calls. Destination writes are sequential.
 */
template <typename T>
Result<> FillOocDataStore(DataArray<T>& dataArray, const DataPath& dataArrayPath, const nx::core::HDF5::DatasetIO& datasetReader, const std::optional<std::vector<hsize_t>>& start = std::nullopt,
                          const std::optional<std::vector<hsize_t>>& count = std::nullopt, const std::atomic_bool* shouldCancel = nullptr)
{
  auto& absDataStore = dataArray.getDataStoreRef();

  // Read bounded HDF5 regions instead of allocating the full dataset.
  auto dims = datasetReader.getDimensions();
  if(dims.empty())
  {
    return MakeErrorResult(-21005, fmt::format("Error reading dataset '{}': unable to get dimensions.", dataArrayPath.getTargetName()));
  }

  // Expand partial start and count vectors to the complete rank.
  const usize rank = dims.size();
  std::vector<uint64> effStart(rank, 0);
  std::vector<uint64> effCount(rank);
  for(usize d = 0; d < rank; d++)
  {
    effCount[d] = dims[d];
  }
  if(start.has_value())
  {
    for(usize d = 0; d < std::min(rank, start.value().size()); d++)
    {
      if(start.value()[d] > dims[d])
      {
        return MakeErrorResult(-21006, fmt::format("Error reading dataset '{}': HDF5 start is outside the dataset dimensions.", dataArrayPath.getTargetName()));
      }
      effStart[d] = start.value()[d];
      effCount[d] = dims[d] - start.value()[d];
    }
  }
  if(count.has_value())
  {
    for(usize d = 0; d < std::min(rank, count.value().size()); d++)
    {
      if(count.value()[d] > dims[d] - effStart[d])
      {
        return MakeErrorResult(-21006, fmt::format("Error reading dataset '{}': HDF5 count exceeds the dataset dimensions.", dataArrayPath.getTargetName()));
      }
      effCount[d] = count.value()[d];
    }
  }

  // Select the first dimension whose trailing extent fits the transfer target.
  // Preceding dimensions advance one coordinate at a time. Each hyperslab stays
  // contiguous in the source dataset and flattened destination.
  constexpr usize k_TargetBatchElements = 65536;
  usize expectedElements = 1;
  for(const uint64 dimensionSize : effCount)
  {
    if(dimensionSize == 0 || dimensionSize > std::numeric_limits<usize>::max() || expectedElements > std::numeric_limits<usize>::max() / static_cast<usize>(dimensionSize))
    {
      return MakeErrorResult(-21006, fmt::format("Error reading dataset '{}': invalid or overflowing HDF5 dimensions.", dataArrayPath.getTargetName()));
    }
    expectedElements *= static_cast<usize>(dimensionSize);
  }
  if(expectedElements != absDataStore.getSize())
  {
    return MakeErrorResult(-21009, fmt::format("Error reading dataset '{}': requested HDF5 region has '{}' elements but destination data array '{}' has '{}'.", dataArrayPath.getTargetName(),
                                               expectedElements, dataArrayPath.toString(), absDataStore.getSize()));
  }

  usize trailingElements = 1;
  usize batchDimension = 0;
  for(usize d = rank - 1; d > 0; d--)
  {
    if(trailingElements > k_TargetBatchElements / effCount[d])
    {
      batchDimension = d;
      break;
    }
    trailingElements *= static_cast<usize>(effCount[d]);
    batchDimension = d - 1;
  }

  const usize batchRows = std::max(static_cast<usize>(1), k_TargetBatchElements / trailingElements);
  usize outerCount = 1;
  for(usize d = 0; d < batchDimension; d++)
  {
    if(outerCount > std::numeric_limits<usize>::max() / effCount[d])
    {
      return MakeErrorResult(-21006, fmt::format("Error reading dataset '{}': HDF5 dimensions overflow the transfer iterator.", dataArrayPath.getTargetName()));
    }
    outerCount *= effCount[d];
  }

  std::vector<T> buf(k_TargetBatchElements);
  std::vector<uint64> hStart(rank);
  std::vector<uint64> hCount(rank, 1);
  usize flatOffset = 0;
  for(usize outer = 0; outer < outerCount; outer++)
  {
    if(shouldCancel != nullptr && *shouldCancel)
    {
      return {};
    }
    usize coordinate = outer;
    for(usize d = batchDimension; d-- > 0;)
    {
      hStart[d] = effStart[d] + (coordinate % effCount[d]);
      coordinate /= effCount[d];
    }
    for(usize d = batchDimension + 1; d < rank; d++)
    {
      hStart[d] = effStart[d];
      hCount[d] = effCount[d];
    }

    for(usize batchStart = 0; batchStart < effCount[batchDimension]; batchStart += batchRows)
    {
      if(shouldCancel != nullptr && *shouldCancel)
      {
        return {};
      }
      const usize rowCount = std::min(batchRows, static_cast<usize>(effCount[batchDimension] - batchStart));
      const usize batchElements = rowCount * trailingElements;
      hStart[batchDimension] = effStart[batchDimension] + batchStart;
      hCount[batchDimension] = rowCount;

      nonstd::span<T> batchSpan(buf.data(), batchElements);
      auto result = datasetReader.readIntoSpan<T>(batchSpan, hStart, hCount);
      if(result.invalid())
      {
        return MakeErrorResult(
            -21003, fmt::format("Error reading dataset '{}' into data store for data array '{}':\n\n{}", dataArrayPath.getTargetName(), dataArrayPath.toString(), result.errors()[0].message));
      }

      if(flatOffset > absDataStore.getSize() || batchElements > absDataStore.getSize() - flatOffset)
      {
        return MakeErrorResult(-21007, fmt::format("Error reading dataset '{}': destination bulk-write range exceeds data array '{}'.", dataArrayPath.getTargetName(), dataArrayPath.toString()));
      }
      auto writeResult = absDataStore.copyFromBuffer(flatOffset, nonstd::span<const T>(buf.data(), batchElements));
      if(writeResult.invalid())
      {
        return writeResult;
      }
      flatOffset += batchElements;
    }
  }

  if(flatOffset != expectedElements)
  {
    return MakeErrorResult(-21008, fmt::format("Error reading dataset '{}': bounded transfer did not cover the requested HDF5 region.", dataArrayPath.getTargetName()));
  }

  return {};
}

/**
 * @brief Reads an HDF5 dataset into the destination's current store type.
 * @tparam T Specifies a non-boolean numeric value type.
 * @param dataStructure Owns the destination array.
 * @param dataArrayPath Identifies the destination array.
 * @param datasetReader Supplies the open HDF5 dataset.
 * @param start Specifies an optional region origin.
 * @param count Specifies optional region dimensions.
 * @param shouldCancel Supplies cancellation for the OOC path.
 * @return Result from the in-memory or OOC transfer.
 * @pre For non-OOC stores, start and count are both present or both absent.
 *
 * StoreType::OutOfCore selects bounded streaming. Other store types use the
 * direct DataStore path, which requires DataStore<T> and ignores shouldCancel.
 */
template <typename T>
Result<> FillDataArray(DataStructure& dataStructure, const DataPath& dataArrayPath, const nx::core::HDF5::DatasetIO& datasetReader, const std::optional<std::vector<hsize_t>>& start = std::nullopt,
                       const std::optional<std::vector<hsize_t>>& count = std::nullopt, const std::atomic_bool* shouldCancel = nullptr)
{
  auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
  if(dataArray.getIDataStoreRef().getStoreType() != IDataStore::StoreType::OutOfCore)
  {
    return FillDataStore(dataArray, dataArrayPath, datasetReader, start, count);
  }
  else
  {
    return FillOocDataStore(dataArray, dataArrayPath, datasetReader, start, count, shouldCancel);
  }
}
} // namespace Support
} // namespace nx::core::HDF5
