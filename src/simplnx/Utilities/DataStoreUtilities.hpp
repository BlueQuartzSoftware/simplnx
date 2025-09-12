#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Filter/Output.hpp"

#include <fmt/format.h>

namespace nx::core::DataStoreUtilities
{
/**
 * @brief Sets the dataFormat string to the large data format from the prefs
 * if forceOocData is true.
 * @param dataFormat
 */
SIMPLNX_EXPORT void TryForceLargeDataFormatFromPrefs(std::string& dataFormat);

/**
 * @brief Returns the application's DataIOCollection.
 * @return
 */
SIMPLNX_EXPORT std::shared_ptr<DataIOCollection> GetIOCollection();

template <class T>
uint64 CalculateDataSize(const IDataStore::ShapeType& tupleShape, const IDataStore::ShapeType& componentShape)
{
  uint64 numValues = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());
  uint64 numComponents = std::accumulate(componentShape.begin(), componentShape.end(), 1ULL, std::multiplies<>());
  return numValues * numComponents * sizeof(T);
}

/**
 * @brief Creates a DataStore with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param tupleShape The Tuple Dimensions
 * @param componentShape The component dimensions
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateDataStore(const typename IDataStore::ShapeType& tupleShape, const typename IDataStore::ShapeType& componentShape, IDataAction::Mode mode,
                                                      std::string dataFormat = "")
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape, dataFormat);
  }
  case IDataAction::Mode::Execute: {
    uint64 dataSize = CalculateDataSize<T>(tupleShape, componentShape);
    TryForceLargeDataFormatFromPrefs(dataFormat);
    auto ioCollection = GetIOCollection();
    ioCollection->checkStoreDataFormat(dataSize, dataFormat);
    return ioCollection->createDataStoreWithType<T>(dataFormat, tupleShape, componentShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

template <class T>
std::shared_ptr<AbstractListStore<T>> CreateListStore(const IListStore::ShapeType& tupleShape, IDataAction::Mode mode = IDataAction::Mode::Execute, std::string dataFormat = "")
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyListStore<T>>(tupleShape);
  }
  case IDataAction::Mode::Execute: {
    uint64 dataSize = CalculateDataSize<T>(tupleShape, {10});
    TryForceLargeDataFormatFromPrefs(dataFormat);
    auto ioCollection = GetIOCollection();
    ioCollection->checkStoreDataFormat(dataSize, dataFormat);
    return ioCollection->createListStoreWithType<T>(dataFormat, tupleShape);
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

  auto ioCollection = GetIOCollection();
  std::shared_ptr<AbstractDataStore<T>> newStore = ioCollection->createDataStoreWithType<T>(dataFormat, dataStore.getTupleShape(), dataStore.getComponentShape());
  if(newStore == nullptr)
  {
    return nullptr;
  }

  newStore->copy(dataStore);
  return newStore;
}
} // namespace nx::core::DataStoreUtilities