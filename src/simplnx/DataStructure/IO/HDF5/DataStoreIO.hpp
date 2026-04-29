#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataStoreIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include <fmt/format.h>

#include <functional>
#include <numeric>

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
 * @brief Reads an HDF5 dataset into an in-memory DataStore.
 *
 * Reads tuple/component shapes from HDF5 attributes, allocates an
 * in-core DataStore<T>, and loads all data from the dataset into memory.
 * This function does not handle OOC stores or recovery-file placeholders;
 * those are handled by the data store import handler at a higher level.
 *
 * If the physical HDF5 dataset element count does not match the expected
 * count from shape attributes, the dataset is skipped and a warning is
 * returned. This guards against reading placeholder datasets written by
 * an OOC-enabled build.
 *
 * @param datasetReader The HDF5 dataset to read from
 * @return Result containing the in-memory data store, or a warning with
 *         nullptr if the dataset is a placeholder
 */
template <typename T>
inline Result<std::shared_ptr<AbstractDataStore<T>>> ReadDataStoreIntoMemory(const nx::core::HDF5::DatasetIO& datasetReader)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // Check that the physical HDF5 dataset size matches the expected size
  // from shape attributes. A mismatch indicates the dataset is a
  // placeholder (e.g. written by an OOC-enabled build).
  usize expectedElements = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>()) *
                           std::accumulate(componentShape.cbegin(), componentShape.cend(), static_cast<usize>(1), std::multiplies<>());
  usize physicalElements = datasetReader.getNumElements();

  if(physicalElements != expectedElements)
  {
    Result<std::shared_ptr<AbstractDataStore<T>>> result;
    result.warnings().push_back(Warning{-89200, fmt::format("Unable to read dataset '{}' at path '{}': the file contains {} elements but the shape "
                                                            "attributes indicate {} elements. This typically means the dataset is an out-of-core placeholder whose "
                                                            "data is not stored inline; reading its full contents requires an out-of-core-enabled (SIMPLNX_USE_OOC) build.",
                                                            datasetReader.getName(), datasetReader.getObjectPath(), physicalElements, expectedElements)});
    return result;
  }

  // In-core branch of the import pipeline: always allocate a plain in-memory
  // DataStore and load from disk. The OOC branch is handled by the higher-level
  // data store import handler before this is reached, so the resolver should
  // not be consulted here.
  auto dataStore = std::make_shared<DataStore<T>>(tupleShape, componentShape, T{});
  dataStore->readHdf5(datasetReader);
  return {std::move(dataStore)};
}
} // namespace DataStoreIO
} // namespace HDF5
} // namespace nx::core
