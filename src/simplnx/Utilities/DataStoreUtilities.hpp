#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Filter/Output.hpp"

#include <fmt/format.h>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
class DataStructure;

/**
 * @namespace nx::core::ArrayCreationUtilities
 * @brief Contains storage-aware array creation utilities.
 */
namespace ArrayCreationUtilities
{
// This declaration avoids an include cycle with ArrayCreationUtilities.hpp, which owns the API documentation.
SIMPLNX_EXPORT std::string ResolveStorageFormat(const DataStructure& dataStructure, const DataPath& path, DataType numericType, uint64 dataSizeBytes, const std::string& requestedFormat);
} // namespace ArrayCreationUtilities
} // namespace nx::core

/**
 * @namespace nx::core::DataStoreUtilities
 * @brief Contains storage-neutral DataStore and ListStore utilities.
 */
namespace nx::core::DataStoreUtilities
{
/**
 * @brief Returns a non-owning reference to the application's DataIOCollection.
 *
 * The Application owns the collection. The reference remains valid while that
 * process Application exists and does not extend its lifetime.
 *
 * @return Reference to the Application's DataIOCollection.
 */
SIMPLNX_EXPORT DataIOCollection& GetIOCollection();

/**
 * @brief Calculates logical array size in bytes.
 * @tparam T Specifies the element type.
 * @param tupleShape Specifies tuple dimensions.
 * @param componentShape Specifies component dimensions.
 * @return Product of tuple count, component count, and sizeof(T).
 * @pre The complete product fits in uint64.
 */
template <class T>
uint64 CalculateDataSize(const ShapeType& tupleShape, const ShapeType& componentShape)
{
  uint64 numValues = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());
  uint64 numComponents = std::accumulate(componentShape.begin(), componentShape.end(), 1ULL, std::multiplies<>());
  return numValues * numComponents * sizeof(T);
}

/**
 * @brief Creates storage for a DataArray.
 *
 * Execute mode applies the DataStructure format resolver. A registered I/O
 * manager creates the selected format.
 *
 * Preflight mode returns an EmptyDataStore with shape metadata. It does not
 * consult the resolver or allocate backing storage.
 *
 * Construct an in-memory DataStore directly for scratch that has no owning
 * DataStructure and DataPath. Those objects do not provide resolution context.
 *
 * @tparam T Specifies the element type.
 * @param dataStructure Contains the future array and supplies resolution context.
 * @param arrayPath Identifies the future array and its geometry ancestors.
 * @param tupleShape Specifies tuple dimensions.
 * @param componentShape Specifies component dimensions.
 * @param mode Selects metadata-only preflight or backing-store execution.
 * @return Created store, or null if no manager supports the resolved format and type.
 * @throws std::runtime_error If mode is not valid.
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateDataStore(const DataStructure& dataStructure, const DataPath& arrayPath, const ShapeType& tupleShape, const ShapeType& componentShape,
                                                      IDataAction::Mode mode = IDataAction::Mode::Execute)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape, std::string{});
  }
  case IDataAction::Mode::Execute: {
    // An empty explicit format delegates to the DataStructure resolver.
    const uint64 requiredBytes = CalculateDataSize<T>(tupleShape, componentShape);
    const std::string resolvedFormat = ArrayCreationUtilities::ResolveStorageFormat(dataStructure, arrayPath, GetDataType<T>(), requiredBytes, "");
    // The manager registered for the resolved format creates the concrete store.
    return GetIOCollection().createDataStoreWithType<T>(resolvedFormat, tupleShape, componentShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

/**
 * @brief Creates storage for a NeighborList.
 *
 * Execute mode applies an explicit format and then the DataStructure resolver.
 *
 * Preflight mode returns an EmptyListStore with shape metadata. It does not
 * consult the resolver or allocate backing storage.
 *
 * NeighborList tuple lengths are unknown at creation. The resolver receives
 * tupleCount * sizeof(T) as a lower-bound size estimate.
 *
 * Construct an in-memory ListStore directly for scratch that has no owning
 * DataStructure and DataPath.
 *
 * @tparam T Specifies the list element type.
 * @param dataStructure Contains the future list and supplies resolution context.
 * @param arrayPath Identifies the future list and its geometry ancestors.
 * @param tupleShape Specifies tuple dimensions.
 * @param mode Selects metadata-only preflight or backing-store execution.
 * @param dataFormat Explicit format, or an empty name to use the resolver.
 * @return Created store, or null if no manager supports the resolved format and type.
 * @throws std::runtime_error If mode is not valid.
 */
template <class T>
std::shared_ptr<AbstractListStore<T>> CreateListStore(const DataStructure& dataStructure, const DataPath& arrayPath, const ShapeType& tupleShape, IDataAction::Mode mode = IDataAction::Mode::Execute,
                                                      const std::string& dataFormat = "")
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyListStore<T>>(tupleShape);
  }
  case IDataAction::Mode::Execute: {
    // Tuple count gives a lower-bound size because list lengths are not known yet.
    const uint64 numTuples = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());
    const uint64 estimatedBytes = numTuples * sizeof(T);
    const std::string resolvedFormat = ArrayCreationUtilities::ResolveStorageFormat(dataStructure, arrayPath, GetDataType<T>(), estimatedBytes, dataFormat);
    // The manager registered for the resolved format creates the concrete list store.
    return GetIOCollection().createListStoreWithType<T>(resolvedFormat, tupleShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

/**
 * @brief Copies a store into a different explicit format.
 * @tparam T Specifies the element type.
 * @param dataStore Source store.
 * @param dataFormat Explicit target format name.
 * @return Converted store. Returns null for an unchanged or unsupported format.
 */
template <typename T>
std::shared_ptr<AbstractDataStore<T>> ConvertDataStore(const AbstractDataStore<T>& dataStore, const std::string& dataFormat)
{
  if(dataStore.getDataFormat() == dataFormat)
  {
    return nullptr;
  }

  // Conversion has no DataStructure or DataPath, so the caller supplies the target format.
  std::shared_ptr<AbstractDataStore<T>> newStore = GetIOCollection().createDataStoreWithType<T>(dataFormat, dataStore.getTupleShape(), dataStore.getComponentShape());
  if(newStore == nullptr)
  {
    return nullptr;
  }

  newStore->copy(dataStore);
  return newStore;
}
} // namespace nx::core::DataStoreUtilities
