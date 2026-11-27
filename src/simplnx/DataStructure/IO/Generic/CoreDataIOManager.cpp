#include "CoreDataIOManager.hpp"

#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/ListStore.hpp"

namespace nx::core::generic
{
CoreDataIOManager::CoreDataIOManager()

{
  addCoreFactories();
  addDataStoreFnc();
  addListStoreFnc();
}

CoreDataIOManager::~CoreDataIOManager() noexcept = default;

std::string CoreDataIOManager::formatName() const
{
  // The reserved name distinguishes explicit resident storage from an empty automatic-format request.
  return std::string(Preferences::k_InMemoryFormat);
}

void CoreDataIOManager::addCoreFactories()
{
}

void CoreDataIOManager::addDataStoreFnc()
{
  // Resident stores ignore chunk shape and initialization policy but retain the common factory signature.
  DataStoreCreateFncType dataStoreFnc = [](nx::core::DataType numericType, const ShapeType& tupleShape, const ShapeType& componentShape, const std::optional<ShapeType>& chunkShape,
                                       DataStoreInitializationMode initializationMode) {
    static_cast<void>(chunkShape);
    static_cast<void>(initializationMode);
    std::unique_ptr<IDataStore> dataStore = nullptr;
    switch(numericType)
    {
    case DataType::int8:
      dataStore = std::make_unique<Int8DataStoreType>(tupleShape, componentShape, static_cast<int8>(0));
      break;
    case DataType::int16:
      dataStore = std::make_unique<Int16DataStoreType>(tupleShape, componentShape, static_cast<int16>(0));
      break;
    case DataType::int32:
      dataStore = std::make_unique<Int32DataStoreType>(tupleShape, componentShape, static_cast<int32>(0));
      break;
    case DataType::int64:
      dataStore = std::make_unique<Int64DataStoreType>(tupleShape, componentShape, static_cast<int64>(0));
      break;
    case DataType::uint8:
      dataStore = std::make_unique<UInt8DataStoreType>(tupleShape, componentShape, static_cast<uint8>(0));
      break;
    case DataType::uint16:
      dataStore = std::make_unique<UInt16DataStoreType>(tupleShape, componentShape, static_cast<uint16>(0));
      break;
    case DataType::uint32:
      dataStore = std::make_unique<UInt32DataStoreType>(tupleShape, componentShape, static_cast<uint32>(0));
      break;
    case DataType::uint64:
      dataStore = std::make_unique<UInt64DataStoreType>(tupleShape, componentShape, static_cast<uint64>(0));
      break;
    case DataType::float32:
      dataStore = std::make_unique<Float32DataStoreType>(tupleShape, componentShape, 0.0f);
      break;
    case DataType::float64:
      dataStore = std::make_unique<Float64DataStoreType>(tupleShape, componentShape, 0.0);
      break;
    case DataType::boolean:
      dataStore = std::make_unique<BoolDataStoreType>(tupleShape, componentShape, false);
      break;
    }
    return dataStore;
  };
  addDataStoreCreationFnc(formatName(), dataStoreFnc);
}

void CoreDataIOManager::addListStoreFnc()
{
  ListStoreCreateFncType listStoreFnc = [](nx::core::DataType numericType, const ShapeType& tupleShape) {
    std::unique_ptr<IListStore> listStore = nullptr;
    switch(numericType)
    {
    case DataType::int8:
      listStore = std::make_unique<Int8ListStoreType>(tupleShape);
      break;
    case DataType::int16:
      listStore = std::make_unique<Int16ListStoreType>(tupleShape);
      break;
    case DataType::int32:
      listStore = std::make_unique<Int32ListStoreType>(tupleShape);
      break;
    case DataType::int64:
      listStore = std::make_unique<Int64ListStoreType>(tupleShape);
      break;
    case DataType::uint8:
      listStore = std::make_unique<UInt8ListStoreType>(tupleShape);
      break;
    case DataType::uint16:
      listStore = std::make_unique<UInt16ListStoreType>(tupleShape);
      break;
    case DataType::uint32:
      listStore = std::make_unique<UInt32ListStoreType>(tupleShape);
      break;
    case DataType::uint64:
      listStore = std::make_unique<UInt64ListStoreType>(tupleShape);
      break;
    case DataType::float32:
      listStore = std::make_unique<Float32ListStoreType>(tupleShape);
      break;
    case DataType::float64:
      listStore = std::make_unique<Float64ListStoreType>(tupleShape);
      break;
    case DataType::boolean:
      // The core ListStore implementation does not support Boolean values.
      listStore = nullptr;
      break;
    }
    return listStore;
  };
  addListStoreCreationFnc(formatName(), listStoreFnc);
}
} // namespace nx::core::generic
