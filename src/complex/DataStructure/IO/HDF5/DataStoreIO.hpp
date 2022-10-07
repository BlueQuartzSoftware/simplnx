#pragma once

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/IO/HDF5/IDataStoreIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetWriter.hpp"

namespace complex
{
namespace HDF5
{
namespace DataStoreIO
{
/**
 * @brief Writes the data store to HDF5. Returns the HDF5 error code should
 * one be encountered. Otherwise, returns 0.
 * @param datasetWriter
 * @return H5::ErrorType
 */
template <typename T>
static H5::ErrorType WriteDataStore(H5::DatasetWriter& datasetWriter, const DataStore<T>& dataStore)
{
  if(!datasetWriter.isValid())
  {
    return -1;
  }

  // Consolidate the Tuple and Component Dims into a single array which is used
  // to write the entire data array to HDF5
  const auto tupleShape = dataStore.getTupleShape();
  const auto componentShape = dataStore.getComponentShape();
  std::vector<hsize_t> h5dims;
  for(const auto& value : tupleShape)
  {
    h5dims.push_back(static_cast<hsize_t>(value));
  }
  for(const auto& value : componentShape)
  {
    h5dims.push_back(static_cast<hsize_t>(value));
  }

  usize count = dataStore.getSize();
  const T* dataPtr = dataStore.data();
  herr_t err = datasetWriter.writeSpan(h5dims, nonstd::span<const T>{dataPtr, count});
  if(err < 0)
  {
    return err;
  }

  // Write shape attributes to the dataset
  auto tupleAttribute = datasetWriter.createAttribute(complex::H5::k_TupleShapeTag);
  err = tupleAttribute.writeVector({tupleShape.size()}, tupleShape);
  if(err < 0)
  {
    return err;
  }

  auto componentAttribute = datasetWriter.createAttribute(complex::H5::k_ComponentShapeTag);
  err = componentAttribute.writeVector({componentShape.size()}, componentShape);

  return err;
}

/**
 * @brief Attempts to read a DataStore<T> from the dataset reader
 * @param datasetReader
 * @return std::unique_ptr<DataStore<T>>
 */
template <typename T>
static std::unique_ptr<DataStore<T>> ReadDataStore(const H5::DatasetReader& datasetReader)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // Create DataStore
  auto dataStore = std::make_unique<DataStore<T>>(tupleShape, componentShape, static_cast<T>(0));
  if(!datasetReader.readIntoSpan(dataStore->createSpan()))
  {
    throw std::runtime_error(fmt::format("Error reading data array from DataStore from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }

  return dataStore;
}
} // namespace DataStoreIO
} // namespace HDF5
} // namespace complex
