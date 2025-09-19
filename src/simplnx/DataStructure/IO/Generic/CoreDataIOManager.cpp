#include "CoreDataIOManager.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/ListStore.hpp"

namespace nx::core::Generic
{
CoreDataIOManager::CoreDataIOManager()
: IDataIOManager()
{
  addCoreFactories();
  addDataStoreFnc();
  addListStoreFnc();
}

CoreDataIOManager::~CoreDataIOManager() noexcept = default;

std::string CoreDataIOManager::formatName() const
{
  return "";
}

void CoreDataIOManager::addCoreFactories()
{
}

void CoreDataIOManager::addDataStoreFnc()
{
  DataStoreCreateFnc dataStoreFnc = [](nx::core::DataType numericType, const ShapeType& tupleShape, const ShapeType& componentShape, const std::optional<ShapeType>& chunkShape) {
    std::unique_ptr<IDataStore> dataStore = nullptr;
    switch(numericType)
    {
    case DataType::int8:
      dataStore = std::make_unique<Int8DataStore>(tupleShape, componentShape, static_cast<int8>(0));
      break;
    case DataType::int16:
      dataStore = std::make_unique<Int16DataStore>(tupleShape, componentShape, static_cast<int16>(0));
      break;
    case DataType::int32:
      dataStore = std::make_unique<Int32DataStore>(tupleShape, componentShape, static_cast<int32>(0));
      break;
    case DataType::int64:
      dataStore = std::make_unique<Int64DataStore>(tupleShape, componentShape, static_cast<int64>(0));
      break;
    case DataType::uint8:
      dataStore = std::make_unique<UInt8DataStore>(tupleShape, componentShape, static_cast<uint8>(0));
      break;
    case DataType::uint16:
      dataStore = std::make_unique<UInt16DataStore>(tupleShape, componentShape, static_cast<uint16>(0));
      break;
    case DataType::uint32:
      dataStore = std::make_unique<UInt32DataStore>(tupleShape, componentShape, static_cast<uint32>(0));
      break;
    case DataType::uint64:
      dataStore = std::make_unique<UInt64DataStore>(tupleShape, componentShape, static_cast<uint64>(0));
      break;
    case DataType::float32:
      dataStore = std::make_unique<Float32DataStore>(tupleShape, componentShape, 0.0f);
      break;
    case DataType::float64:
      dataStore = std::make_unique<Float64DataStore>(tupleShape, componentShape, 0.0);
      break;
    case DataType::boolean:
      dataStore = std::make_unique<BoolDataStore>(tupleShape, componentShape, false);
      break;
    }
    return dataStore;
  };
  addDataStoreCreationFnc(formatName(), dataStoreFnc);
}

void CoreDataIOManager::addListStoreFnc()
{
  ListStoreCreateFnc listStoreFnc = [](nx::core::DataType numericType, const ShapeType& tupleShape) {
    std::unique_ptr<IListStore> listStore = nullptr;
    switch(numericType)
    {
    case DataType::int8:
      listStore = std::make_unique<Int8ListStore>(tupleShape);
      break;
    case DataType::int16:
      listStore = std::make_unique<Int16ListStore>(tupleShape);
      break;
    case DataType::int32:
      listStore = std::make_unique<Int32ListStore>(tupleShape);
      break;
    case DataType::int64:
      listStore = std::make_unique<Int64ListStore>(tupleShape);
      break;
    case DataType::uint8:
      listStore = std::make_unique<UInt8ListStore>(tupleShape);
      break;
    case DataType::uint16:
      listStore = std::make_unique<UInt16ListStore>(tupleShape);
      break;
    case DataType::uint32:
      listStore = std::make_unique<UInt32ListStore>(tupleShape);
      break;
    case DataType::uint64:
      listStore = std::make_unique<UInt64ListStore>(tupleShape);
      break;
    case DataType::float32:
      listStore = std::make_unique<Float32ListStore>(tupleShape);
      break;
    case DataType::float64:
      listStore = std::make_unique<Float64ListStore>(tupleShape);
      break;
    case DataType::boolean:
      listStore = nullptr;
      break;
    }
    return listStore;
  };
  addListStoreCreationFnc(formatName(), listStoreFnc);
}
} // namespace nx::core::Generic
