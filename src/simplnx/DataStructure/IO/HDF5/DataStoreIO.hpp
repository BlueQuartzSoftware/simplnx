#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataStoreIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include <fmt/format.h>

#include <functional>
#include <iterator>
#include <numeric>
#include <stdexcept>

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
 * @return Store or shape-attribute write errors.
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
  auto warnings = std::move(writeResult.warnings());

  const auto tupleShape = dataStore.getTupleShape();
  const auto componentShape = dataStore.getComponentShape();
  auto shapeResult = datasetWriter.writeVectorAttribute(IOConstants::k_TupleShapeTag, tupleShape);
  if(shapeResult.invalid())
  {
    shapeResult.warnings().insert(shapeResult.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
    return shapeResult;
  }
  for(auto&& warning : shapeResult.warnings())
  {
    warnings.push_back(std::move(warning));
  }
  shapeResult = datasetWriter.writeVectorAttribute(IOConstants::k_ComponentShapeTag, componentShape);
  if(shapeResult.invalid())
  {
    shapeResult.warnings().insert(shapeResult.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
    return shapeResult;
  }
  for(auto&& warning : shapeResult.warnings())
  {
    warnings.push_back(std::move(warning));
  }

  Result<> result;
  result.warnings() = std::move(warnings);
  return result;
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
  auto tupleShapeResult = IDataStoreIO::ReadTupleShape(datasetReader);
  if(tupleShapeResult.invalid())
  {
    return ConvertInvalidResult<std::shared_ptr<AbstractDataStore<T>>>(std::move(tupleShapeResult));
  }
  auto warnings = std::move(tupleShapeResult.warnings());
  auto componentShapeResult = IDataStoreIO::ReadComponentShape(datasetReader);
  if(componentShapeResult.invalid())
  {
    auto result = ConvertInvalidResult<std::shared_ptr<AbstractDataStore<T>>>(std::move(componentShapeResult));
    result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
    return result;
  }
  for(auto&& warning : componentShapeResult.warnings())
  {
    warnings.push_back(std::move(warning));
  }
  ShapeType tupleShape = std::move(tupleShapeResult.value());
  ShapeType componentShape = std::move(componentShapeResult.value());

  usize expectedElements = 0;
  try
  {
    expectedElements = static_cast<usize>(CalculateStoreCopyBytes(tupleShape, componentShape, sizeof(T)) / sizeof(T));
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& error)
  {
    auto result = MakeErrorResult<std::shared_ptr<AbstractDataStore<T>>>(
        -89201, fmt::format("Cannot validate numeric dataset '{}' at path '{}': {}", datasetReader.getName(), datasetReader.getObjectPath(), error.what()));
    result.warnings() = std::move(warnings);
    return result;
  }
  usize physicalElements = datasetReader.getNumElements();

  if(physicalElements != expectedElements)
  {
    Result<std::shared_ptr<AbstractDataStore<T>>> result;
    result.warnings() = std::move(warnings);
    result.warnings().push_back(Warning{-89200, fmt::format("Unable to read dataset '{}' at path '{}': the file contains {} elements but the shape "
                                                            "attributes indicate {} elements. This typically means the dataset is an out-of-core placeholder whose "
                                                            "data is not stored inline; reading its full contents requires an out-of-core-enabled build.",
                                                            datasetReader.getName(), datasetReader.getObjectPath(), physicalElements, expectedElements)});
    return result;
  }

  // The higher import layer selects out-of-core stores. This branch always
  // materializes a plain in-memory DataStore.
  auto dataStore = std::make_shared<DataStore<T>>(tupleShape, componentShape, T{});
  auto readResult = dataStore->readHdf5(datasetReader);
  if(readResult.invalid())
  {
    auto result = ConvertInvalidResult<std::shared_ptr<AbstractDataStore<T>>>(std::move(readResult));
    result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
    return result;
  }
  for(auto&& warning : readResult.warnings())
  {
    warnings.push_back(std::move(warning));
  }
  Result<std::shared_ptr<AbstractDataStore<T>>> result{std::move(dataStore)};
  result.warnings() = std::move(warnings);
  return result;
}

} // namespace DataStoreIO
} // namespace HDF5
} // namespace nx::core
