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
 * @brief Writes a data store and its shape attributes.
 * @tparam T Stored value type.
 * @param datasetWriter Destination HDF5 dataset.
 * @param dataStore Source store.
 * @return Write errors from the store operation.
 *
 * The implementation does not return shape-attribute write results.
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

  const auto tupleShape = dataStore.getTupleShape();
  const auto componentShape = dataStore.getComponentShape();
  datasetWriter.writeVectorAttribute(IOConstants::k_TupleShapeTag, tupleShape);
  datasetWriter.writeVectorAttribute(IOConstants::k_ComponentShapeTag, componentShape);

  return {};
}

/**
 * @brief Reads an HDF5 dataset into an in-memory DataStore.
 * @tparam T Stored value type.
 * @param datasetReader Source HDF5 dataset.
 * @return Materialized in-memory store, or a warning with nullptr for a
 * recovery placeholder.
 *
 * The function treats a shape and physical-count mismatch as a recovery
 * placeholder. Malformed data can produce the same mismatch. The caller
 * selects an out-of-core store before this in-memory path.
 * @pre Tuple and component shape products, including their full product, fit
 * usize.
 */
template <typename T>
inline Result<std::shared_ptr<AbstractDataStore<T>>> ReadDataStoreIntoMemory(const nx::core::HDF5::DatasetIO& datasetReader)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // This path treats a physical-count mismatch as a recovery placeholder. A
  // malformed inline dataset can produce the same mismatch.
  usize expectedElements = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>()) *
                           std::accumulate(componentShape.cbegin(), componentShape.cend(), static_cast<usize>(1), std::multiplies<>());
  usize physicalElements = datasetReader.getNumElements();

  if(physicalElements != expectedElements)
  {
    Result<std::shared_ptr<AbstractDataStore<T>>> result;
    result.warnings().push_back(Warning{-89200, fmt::format("Unable to read dataset '{}' at path '{}': the file contains {} elements but the shape "
                                                            "attributes indicate {} elements. This typically means the dataset is an out-of-core placeholder whose "
                                                            "data is not stored inline; reading its full contents requires an out-of-core-enabled build.",
                                                            datasetReader.getName(), datasetReader.getObjectPath(), physicalElements, expectedElements)});
    return result;
  }

  // The higher import layer selects out-of-core stores. This branch always
  // materializes a plain in-memory DataStore.
  auto dataStore = std::make_shared<DataStore<T>>(tupleShape, componentShape, T{});
  // The current implementation does not inspect the readHdf5() Result.
  dataStore->readHdf5(datasetReader);
  return {std::move(dataStore)};
}

} // namespace DataStoreIO
} // namespace HDF5
} // namespace nx::core
