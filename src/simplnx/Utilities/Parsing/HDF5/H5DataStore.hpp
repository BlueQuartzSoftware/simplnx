#pragma once

#include "H5Support.hpp"

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
      result = datasetReader.readIntoSpan<T>(dataSpan, start.value(), count.value());
    }
    else
    {
      result = datasetReader.readIntoSpan<T>(dataSpan);
    }
    if(result.invalid())
    {
      return {MakeErrorResult(-21002,
                              fmt::format("Error reading dataset '{}' with '{}' total elements into data store for data array '{}' with '{}' total elements ('{}' tuples and '{}' components):\n\n{}",
                                          dataArrayPath.getTargetName(), datasetReader.getNumElements(), dataArrayPath.toString(), dataArray.getSize(), dataArray.getNumberOfTuples(),
                                          dataArray.getNumberOfComponents(), result.errors()[0].message))};
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
  if(Memory::GetTotalMemory() <= dataArray.getSize() * sizeof(T))
  {
    return MakeErrorResult(-21004, fmt::format("Error reading dataset '{}' with '{}' total elements. Not enough memory to import data.", dataArray.getName(), datasetReader.getNumElements()));
  }

  auto& absDataStore = dataArray.getDataStoreRef();
  std::vector<T> data(absDataStore.getSize());
  nonstd::span<T> span{data.data(), data.size()};
  Result<> result;
  if(start.has_value() && count.has_value())
  {
    result = datasetReader.readIntoSpan<T>(span, start.value(), count.value());
  }
  else
  {
    result = datasetReader.readIntoSpan<T>(span);
  }

  if(result.invalid())
  {
    return {
        MakeErrorResult(-21003, fmt::format("Error reading dataset '{}' with '{}' total elements into data store for data array '{}' with '{}' total elements ('{}' tuples and '{}' components):\n\n{}",
                                            dataArrayPath.getTargetName(), datasetReader.getNumElements(), dataArrayPath.toString(), dataArray.getSize(), dataArray.getNumberOfTuples(),
                                            dataArray.getNumberOfComponents(), result.errors()[0].message))};
  }
  std::copy(data.begin(), data.end(), absDataStore.begin());

  return {};
}

template <typename T>
Result<> FillDataArray(DataStructure& dataStructure, const DataPath& dataArrayPath, const nx::core::HDF5::DatasetIO& datasetReader, const std::optional<std::vector<hsize_t>>& start = std::nullopt,
                       const std::optional<std::vector<hsize_t>>& count = std::nullopt)
{
  auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
  if(dataArray.getDataFormat().empty())
  {
    return FillDataStore(dataArray, dataArrayPath, datasetReader, start, count);
  }
  else
  {
    return FillOocDataStore(dataArray, dataArrayPath, datasetReader, start, count);
  }
}
}
} // namespace nx::core::HDF5
