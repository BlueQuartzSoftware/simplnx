#pragma once

#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/IO/HDF5/IDataStoreIO.hpp"

namespace complex
{
namespace HDF5
{
namespace EmptyDataStoreIO
{
/**
 * @brief Attempts to read an EmptyDataStore from HDF5.
 * @param datasetReader
 * @return std::unique_ptr<EmptyDataStore<T>>
 */
template <typename T>
static std::unique_ptr<EmptyDataStore<T>> ReadDataStore(const H5::DatasetReader& datasetReader)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // Create DataStore
  auto dataStore = std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape);
  return dataStore;
}
} // namespace EmptyDataStoreIO
} // namespace HDF5
} // namespace complex
