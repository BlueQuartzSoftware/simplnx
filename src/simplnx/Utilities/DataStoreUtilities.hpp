#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/SimplnxConfig.hpp"
#ifdef SIMPLNX_USE_OOC
#include "SimplnxOoc/OocDataIOManager.hpp" // SimplnxOoc::resolveFormat
#include "SimplnxOoc/StoreFactory.hpp"     // SimplnxOoc::createChunkedStore, createChunkedListStore, k_OocFormatName
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
 * @brief Creates a DataStore whose format is resolved through the IOCollection's
 * registered format resolver.
 *
 * This is the standard way to allocate a DataStore that will live inside a
 * DataStructure. The resolver consults user preferences (OOC thresholds,
 * forced OOC mode), the array's data type, its size, and walks the array's
 * parent objects to determine geometry context — for example, it may force
 * in-core for unstructured/poly geometry topology arrays. Callers that go
 * through this function automatically get OOC-backed storage when the OOC
 * plugin is loaded and the array exceeds the configured threshold.
 *
 * In Preflight mode, returns an EmptyDataStore that records shape metadata
 * without allocating any storage. In Execute mode, calls the resolver and
 * forwards to createDataStoreWithType() to allocate the real backing store.
 *
 * For DataStores that are NOT going to live inside a DataStructure (e.g.,
 * scratch buffers, raw HDF5 reader output), construct the in-memory
 * DataStore class directly via std::make_shared<DataStore<T>>(...) — the
 * resolver has nothing meaningful to resolve against without a DataStructure
 * and a DataPath.
 *
 * @tparam T Primitive type (int8, float32, uint64, etc.)
 * @param dataStructure The DataStructure the array will live in (needed for format resolution)
 * @param arrayPath The DataPath where the array will be inserted (needed for parent-walk resolution)
 * @param tupleShape The tuple dimensions (e.g., {100, 200, 300} for a 3D volume)
 * @param componentShape The component dimensions (e.g., {3} for a 3-component vector)
 * @param mode PREFLIGHT returns an EmptyDataStore; EXECUTE allocates real storage
 * @return Shared pointer to the created AbstractDataStore
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateDataStore([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const DataPath& arrayPath, const ShapeType& tupleShape,
                                                      const ShapeType& componentShape, IDataAction::Mode mode = IDataAction::Mode::Execute)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape, std::string{});
  }
  case IDataAction::Mode::Execute: {
#ifdef SIMPLNX_USE_OOC
    // OOC compiled in: ask the OOC resolver which backing format this array
    // should use. The resolver consults user preferences (OOC thresholds,
    // forced OOC mode), the data type, the byte size, and walks the array's
    // parent objects for geometry context. When it selects the OOC format we
    // construct the chunked store directly; any other result (in-memory /
    // user-preferred) falls through to the built-in core factory.
    const uint64 requiredBytes = CalculateDataSize<T>(tupleShape, componentShape);
    const std::string resolvedFormat = SimplnxOoc::resolveFormat(dataStructure, arrayPath, GetDataType<T>(), requiredBytes);
    if(resolvedFormat == SimplnxOoc::k_OocFormatName)
    {
      return SimplnxOoc::createChunkedStore<T>(tupleShape, componentShape);
    }
    return GetIOCollection().createDataStoreWithType<T>(resolvedFormat, tupleShape, componentShape);
#else
    // OOC not compiled in: there is no resolver and no OOC format. Every array
    // is in-core; pass an empty format string to the core factory.
    return GetIOCollection().createDataStoreWithType<T>("", tupleShape, componentShape);
#endif
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

/**
 * @brief Creates a ListStore whose format is resolved through the IOCollection's
 * registered format resolver.
 *
 * This is the standard way to allocate a ListStore (the backing store for
 * NeighborList) that will live inside a DataStructure. Like CreateDataStore,
 * the resolver consults user preferences (OOC thresholds, forced OOC mode),
 * the array's data type, its size, and walks the array's parent objects.
 * Callers automatically get OOC-backed storage when the OOC plugin is loaded
 * and the array exceeds the configured threshold.
 *
 * In Preflight mode, returns an EmptyListStore that records shape metadata
 * without allocating any storage. In Execute mode, calls the resolver and
 * forwards to createListStoreWithType() to allocate the real backing store.
 *
 * Sizing note: NeighborList is variable-length (each tuple is a list whose
 * size isn't known at creation time). We pass numTuples * sizeof(T) as a
 * lower-bound estimate to the resolver, matching the convention used in
 * SimplnxOoc's DataStoreImportStrategy. The geometry-walk and user-preference
 * checks dominate the resolver's decision for NeighborLists in practice;
 * the size threshold only matters when the user has selected an OOC format
 * but no force-OOC flag and the data is below the size cutoff.
 *
 * For ListStores that are NOT going to live inside a DataStructure (e.g.,
 * raw HDF5 reader output), construct the in-memory ListStore class directly
 * via std::make_shared<ListStore<T>>(tupleShape).
 *
 * @tparam T Primitive type of the list elements
 * @param dataStructure The DataStructure the list will live in (needed for format resolution)
 * @param arrayPath The DataPath where the list will be inserted (needed for parent-walk resolution)
 * @param tupleShape The tuple dimensions
 * @param mode PREFLIGHT returns an EmptyListStore; EXECUTE allocates real storage
 * @return Shared pointer to the created AbstractListStore
 */
template <class T>
std::shared_ptr<AbstractListStore<T>> CreateListStore([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const DataPath& arrayPath, const ShapeType& tupleShape,
                                                      IDataAction::Mode mode = IDataAction::Mode::Execute)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyListStore<T>>(tupleShape);
  }
  case IDataAction::Mode::Execute: {
#ifdef SIMPLNX_USE_OOC
    // OOC compiled in: resolve the backing format for this list. NeighborList
    // is variable-length, so we pass numTuples * sizeof(T) as a lower-bound
    // size estimate (matches SimplnxOoc's DataStoreImportStrategy convention).
    // The geometry-walk and user-preference checks dominate the resolver's
    // decision for NeighborLists in practice.
    const uint64 numTuples = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());
    const uint64 estimatedBytes = numTuples * sizeof(T);
    const std::string resolvedFormat = SimplnxOoc::resolveFormat(dataStructure, arrayPath, GetDataType<T>(), estimatedBytes);
    if(resolvedFormat == SimplnxOoc::k_OocFormatName)
    {
      return SimplnxOoc::createChunkedListStore<T>(tupleShape);
    }
    return GetIOCollection().createListStoreWithType<T>(resolvedFormat, tupleShape);
#else
    // OOC not compiled in: lists are always in-core.
    return GetIOCollection().createListStoreWithType<T>("", tupleShape);
#endif
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