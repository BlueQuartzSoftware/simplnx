#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/SimplnxConfig.hpp"
#ifdef SIMPLNX_USE_OOC
#include "SimplnxOoc/StoreFactory.hpp"
#endif

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Filter/Output.hpp"

#include <fmt/format.h>

namespace nx::core::DataStoreUtilities
{
/**
 * @brief Returns a non-owning reference to the application's DataIOCollection.
 *
 * The DataIOCollection is owned by the Application singleton and lives for the
 * entire process lifetime. Callers receive a reference, not a shared_ptr, to
 * make the non-ownership relationship explicit and prevent accidental lifetime
 * extension.
 *
 * @return Reference to the Application's DataIOCollection.
 */
SIMPLNX_EXPORT DataIOCollection& GetIOCollection();

template <class T>
uint64 CalculateDataSize(const ShapeType& tupleShape, const ShapeType& componentShape)
{
  uint64 numValues = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());
  uint64 numComponents = std::accumulate(componentShape.begin(), componentShape.end(), 1ULL, std::multiplies<>());
  return numValues * numComponents * sizeof(T);
}

/**
 * @brief Simple factory that creates a DataStore with the given properties.
 *
 * This function does NOT resolve the storage format. The caller is responsible
 * for determining the correct format (e.g., via the SimplnxOoc::resolveFormat
 * call in CreateArray) and passing it in via the dataFormat parameter.
 *
 * In Preflight mode, returns an EmptyDataStore that records shape metadata
 * without allocating any storage. In Execute mode, forwards directly to
 * createDataStoreWithType() which creates either an in-memory DataStore
 * (for "" unset or k_InMemoryFormat explicit) or an OOC-backed store
 * (for "HDF5-OOC" etc.).
 *
 * @tparam T Primitive type (int8, float32, uint64, etc.)
 * @param tupleShape The tuple dimensions (e.g., {100, 200, 300} for a 3D volume)
 * @param componentShape The component dimensions (e.g., {3} for a 3-component vector)
 * @param mode PREFLIGHT returns an EmptyDataStore; EXECUTE allocates real storage
 * @param dataFormat The already-resolved format name. An empty string means
 *                   "unset/auto — default to in-memory". k_InMemoryFormat means
 *                   "explicit in-memory". Any other non-empty value must be a
 *                   registered format name (e.g., "HDF5-OOC").
 * @return Shared pointer to the created AbstractDataStore
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, IDataAction::Mode mode, const std::string& dataFormat = "")
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape, dataFormat);
  }
  case IDataAction::Mode::Execute: {
#ifdef SIMPLNX_USE_OOC
    // OOC compiled in: intercept the OOC format and construct the store
    // directly. Any other format (in-memory / unset) falls through to the
    // built-in core factory.
    if(dataFormat == SimplnxOoc::k_OocFormatName)
    {
      return SimplnxOoc::createChunkedStore<T>(tupleShape, componentShape);
    }
#endif
    return GetIOCollection().createDataStoreWithType<T>(dataFormat, tupleShape, componentShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

/**
 * @brief Creates a DataStore whose format is resolved through the IOCollection's
 * format resolver, exactly like filter CreateArrayActions do.
 *
 * Use this instead of CreateDataStore when you need the store format to respect
 * user preferences (OOC thresholds, forced OOC mode, etc.). This is the correct
 * function for test code that builds DataArrays which should be OOC when the
 * OOC plugin is active.
 *
 * @tparam T Primitive type (int8, float32, uint64, etc.)
 * @param dataStructure The DataStructure the array will live in (needed for format resolution)
 * @param arrayPath The DataPath where the array will be inserted
 * @param tupleShape The tuple dimensions
 * @param componentShape The component dimensions
 * @return Shared pointer to the created AbstractDataStore
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateResolvedDataStore(const DataStructure& dataStructure, const DataPath& arrayPath, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  uint64 numElements = 1;
  for(auto dim : tupleShape)
  {
    numElements *= dim;
  }
  for(auto dim : componentShape)
  {
    numElements *= dim;
  }
  uint64 requiredBytes = numElements * sizeof(T);
  std::string resolvedFormat = GetIOCollection().resolveFormat(dataStructure, arrayPath, GetDataType<T>(), requiredBytes);
  return GetIOCollection().createDataStoreWithType<T>(resolvedFormat, tupleShape, componentShape);
}

/**
 * @brief Simple factory that creates a ListStore with the given properties.
 *
 * This function does NOT resolve the storage format. The caller is responsible
 * for determining the correct format (e.g., via the SimplnxOoc::resolveFormat
 * call in CreateNeighborListAction) and passing it in via the dataFormat parameter.
 *
 * In Preflight mode, returns an EmptyListStore that records shape metadata
 * without allocating any storage. In Execute mode, forwards directly to
 * createListStoreWithType() which creates either an in-memory ListStore
 * (for "" unset or k_InMemoryFormat explicit) or an OOC-backed store
 * (for "HDF5-OOC" etc.).
 *
 * @tparam T Primitive type of the list elements
 * @param tupleShape The tuple dimensions
 * @param mode PREFLIGHT returns an EmptyListStore; EXECUTE allocates real storage
 * @param dataFormat The already-resolved format name. An empty string means
 *                   "unset/auto — default to in-memory". k_InMemoryFormat means
 *                   "explicit in-memory". Any other non-empty value must be a
 *                   registered format name (e.g., "HDF5-OOC").
 * @return Shared pointer to the created AbstractListStore
 */
template <class T>
std::shared_ptr<AbstractListStore<T>> CreateListStore(const ShapeType& tupleShape, IDataAction::Mode mode = IDataAction::Mode::Execute, const std::string& dataFormat = "")
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    // Preflight: no storage allocated, just record the tuple shape
    return std::make_unique<EmptyListStore<T>>(tupleShape);
  }
  case IDataAction::Mode::Execute: {
#ifdef SIMPLNX_USE_OOC
    if(dataFormat == SimplnxOoc::k_OocFormatName)
    {
      return SimplnxOoc::createChunkedListStore<T>(tupleShape);
    }
#endif
    return GetIOCollection().createListStoreWithType<T>(dataFormat, tupleShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

template <typename T>
std::shared_ptr<AbstractDataStore<T>> ConvertDataStore(const AbstractDataStore<T>& dataStore, const std::string& dataFormat)
{
  if(dataStore.getDataFormat() == dataFormat)
  {
    return nullptr;
  }

  std::shared_ptr<AbstractDataStore<T>> newStore;
#ifdef SIMPLNX_USE_OOC
  if(dataFormat == SimplnxOoc::k_OocFormatName)
  {
    newStore = SimplnxOoc::createChunkedStore<T>(dataStore.getTupleShape(), dataStore.getComponentShape());
  }
  else
#endif
  {
    newStore = GetIOCollection().createDataStoreWithType<T>(dataFormat, dataStore.getTupleShape(), dataStore.getComponentShape());
  }
  if(newStore == nullptr)
  {
    return nullptr;
  }

  newStore->copy(dataStore);
  return newStore;
}
} // namespace nx::core::DataStoreUtilities