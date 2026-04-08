#pragma once

#include "H5Support.hpp"

#include "simplnx/DataStructure/IDataStore.hpp"

namespace nx::core::HDF5
{
namespace Support
{
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
      std::vector<uint64> countVec(start->begin(), start->end());
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

template <typename T>
Result<> FillOocDataStore(DataArray<T>& dataArray, const DataPath& dataArrayPath, const nx::core::HDF5::DatasetIO& datasetReader, const std::optional<std::vector<hsize_t>>& start = std::nullopt,
                          const std::optional<std::vector<hsize_t>>& count = std::nullopt)
{
  auto& absDataStore = dataArray.getDataStoreRef();

  // Streaming path: read HDF5 dataset in row-batches using hyperslab reads
  // to avoid allocating a buffer proportional to the full dataset size.
  // When start/count are provided, the read is restricted to that sub-region.
  auto dims = datasetReader.getDimensions();
  if(dims.empty())
  {
    return MakeErrorResult(-21005, fmt::format("Error reading dataset '{}': unable to get dimensions.", dataArrayPath.getTargetName()));
  }

  // Compute effective start/count for the read region
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
      effStart[d] = start.value()[d];
      effCount[d] = dims[d] - start.value()[d];
    }
  }
  if(count.has_value())
  {
    for(usize d = 0; d < std::min(rank, count.value().size()); d++)
    {
      effCount[d] = count.value()[d];
    }
  }

  // Compute elements per row (product of all count dims except the first)
  usize elementsPerRow = 1;
  for(usize d = 1; d < rank; d++)
  {
    elementsPerRow *= effCount[d];
  }
  const usize totalRows = effCount[0];

  // Choose batch size: read enough rows to fill ~256K elements per batch
  constexpr usize k_TargetBatchElements = 262144;
  const usize rowsPerBatch = std::max(static_cast<usize>(1), k_TargetBatchElements / std::max(elementsPerRow, static_cast<usize>(1)));

  const usize batchBufferSize = rowsPerBatch * elementsPerRow;
  std::vector<T> buf(batchBufferSize);

  usize flatOffset = 0;
  for(usize rowStart = 0; rowStart < totalRows; rowStart += rowsPerBatch)
  {
    const usize rowCount = std::min(rowsPerBatch, totalRows - rowStart);
    const usize batchElements = rowCount * elementsPerRow;

    // Build hyperslab for this batch within the effective region
    std::vector<uint64> hStart(rank);
    std::vector<uint64> hCount(rank);
    hStart[0] = effStart[0] + rowStart;
    hCount[0] = rowCount;
    for(usize d = 1; d < rank; d++)
    {
      hStart[d] = effStart[d];
      hCount[d] = effCount[d];
    }

    nonstd::span<T> batchSpan(buf.data(), batchElements);
    auto result = datasetReader.readIntoSpan<T>(batchSpan, hStart, hCount);
    if(result.invalid())
    {
      return {MakeErrorResult(-21003, fmt::format("Error reading dataset '{}' (rows {}-{}) into data store for data array '{}':\n\n{}", dataArrayPath.getTargetName(), effStart[0] + rowStart,
                                                  effStart[0] + rowStart + rowCount - 1, dataArrayPath.toString(), result.errors()[0].message))};
    }

    absDataStore.copyFromBuffer(flatOffset, nonstd::span<const T>(buf.data(), batchElements));
    flatOffset += batchElements;
  }

  return {};
}

template <typename T>
Result<> FillDataArray(DataStructure& dataStructure, const DataPath& dataArrayPath, const nx::core::HDF5::DatasetIO& datasetReader, const std::optional<std::vector<hsize_t>>& start = std::nullopt,
                       const std::optional<std::vector<hsize_t>>& count = std::nullopt)
{
  auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
  if(dataArray.getIDataStoreRef().getStoreType() != IDataStore::StoreType::OutOfCore)
  {
    return FillDataStore(dataArray, dataArrayPath, datasetReader, start, count);
  }
  else
  {
    return FillOocDataStore(dataArray, dataArrayPath, datasetReader, start, count);
  }
}
} // namespace Support
} // namespace nx::core::HDF5
