#pragma once

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataStoreIO.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include <fmt/format.h>

namespace nx::core
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
inline Result<> WriteDataStore(nx::core::HDF5::DatasetIO& datasetWriter, const AbstractDataStore<T>& dataStore)
{
  // if(!datasetWriter.isValid())
  //{
  //   std::string ss = "Failed to write DataArray. DatasetIO not valid";
  //   return MakeErrorResult(-1, ss);
  // }

  auto writeResult = dataStore.writeHdf5(datasetWriter);
  if(writeResult.invalid())
  {
    return writeResult;
  }

  // Write shape attributes to the dataset
  const auto tupleShape = dataStore.getTupleShape();
  const auto componentShape = dataStore.getComponentShape();
  datasetWriter.writeVectorAttribute(IOConstants::k_TupleShapeTag, tupleShape);
  datasetWriter.writeVectorAttribute(IOConstants::k_ComponentShapeTag, componentShape);

  return {};
}

/**
 * @brief Attempts to read a DataStore<T> from the dataset reader
 * @param datasetReader
 * @param mode
 * @return std::unique_ptr<DataStore<T>>
 */
template <typename T>
inline std::shared_ptr<AbstractDataStore<T>> ReadDataStore(const nx::core::HDF5::DatasetIO& datasetReader, IDataAction::Mode mode)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // Create DataStore
  auto dataStore = DataStoreUtilities::CreateDataStore<T>(tupleShape, componentShape, mode);
  dataStore->readHdf5(datasetReader);
  return dataStore;
}
} // namespace DataStoreIO
} // namespace HDF5
} // namespace nx::core
