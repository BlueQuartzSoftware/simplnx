#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include "fmt/format.h"

namespace nx::core::HDF5
{
DataStructureReader::DataStructureReader(DataIOManager* factoryManager)
: m_IOManager(factoryManager)
{
}
DataStructureReader::~DataStructureReader() noexcept = default;

Result<DataStructure> DataStructureReader::ReadFile(const std::filesystem::path& path, bool useEmptyDataStores)
{
  const nx::core::HDF5::FileIO fileReader = HDF5::FileIO::ReadFile(path);
  return ReadFile(fileReader);
}
Result<DataStructure> DataStructureReader::ReadFile(const nx::core::HDF5::FileIO& fileReader, bool useEmptyDataStores)
{
  DataStructureReader dataStructureReader;
  auto groupReader = fileReader.openGroup(Constants::k_DataStructureTag);
  return dataStructureReader.readGroup(groupReader, useEmptyDataStores);
}

Result<DataStructure> DataStructureReader::readGroup(const nx::core::HDF5::GroupIO& groupReader, bool useEmptyDataStores)
{
  clearDataStructure();

  if(!groupReader.isValid())
  {
    std::string ss = fmt::format("Failed to open top-level DataStructure group");
    return MakeErrorResult<DataStructure>(-1, ss);
  }

  auto idResult = groupReader.readScalarAttribute<DataObject::IdType>(Constants::k_NextIdTag);
  if(idResult.invalid())
  {
    return ConvertInvalidResult<DataStructure>(std::move(idResult));
  }
  DataObject::IdType objectId = std::move(idResult.value());

  m_CurrentStructure = DataStructure();
  m_CurrentStructure.setNextId(objectId);
  Result<> result = HDF5::ReadDataMap(*this, m_CurrentStructure.getRootGroup(), groupReader, {}, useEmptyDataStores);
  if(result.invalid())
  {
    auto& error = result.errors()[0];
    return MakeErrorResult<DataStructure>(error.code, error.message);
  }
  return {m_CurrentStructure};
}

Result<> DataStructureReader::readObjectFromGroup(const nx::core::HDF5::GroupIO& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStores)
{
  std::shared_ptr<IDataIO> factory = nullptr;
  DataObject::IdType objectId = 0;

  // Get nx::core::HDF5::IDataFactory and check DataObject ID
  {
    bool isGroup = parentGroup.isGroup(objectName);

    if(isGroup)
    {
      auto childObj = parentGroup.openGroup(objectName);

      // Return 0 if object is marked as not importable.
      int32 importable = 0;
      auto importableResult = childObj.readScalarAttribute<int32>(Constants::k_ImportableTag);
      if(importableResult.invalid())
      {
        return ConvertResult(std::move(importableResult));
      }
      importable = std::move(importableResult.value());

      if(importable == 0)
      {
        return {};
      }

      // Check if data has already been read
      auto idResult = childObj.readScalarAttribute<DataObject::IdType>(Constants::k_ObjectIdTag);
      if(idResult.invalid())
      {
        return ConvertResult(std::move(idResult));
      }
      objectId = std::move(idResult.value());

      if(getDataStructure().containsData(objectId) && parentId.has_value())
      {
        getDataStructure().setAdditionalParent(objectId, parentId.value());
        return {};
      }

      // Get DataObject type for factory
      auto attrResult = childObj.readStringAttribute(Constants::k_ObjectTypeTag);
      if(attrResult.invalid())
      {
        return ConvertResult(std::move(attrResult));
      }
      std::string typeName = std::move(attrResult.value());

      factory = getDataFactory(typeName);
    }
    else
    {
      auto childObj = parentGroup.openDataset(objectName);

      // Return 0 if object is marked as not importable.
      auto importableResult = childObj.readScalarAttribute<int32>(Constants::k_ImportableTag);
      int32 importable = 0;
      if(importableResult.valid())
      {
        importable = std::move(importableResult.value());
      }

      if(importable == 0)
      {
        return {};
      }

      // Check if data has already been read
      auto objectIdResult = childObj.readScalarAttribute<DataObject::IdType>(Constants::k_ObjectIdTag);
      if(objectIdResult.valid())
      {
        objectId = std::move(objectIdResult.value());
      }

      if(getDataStructure().containsData(objectId) && parentId.has_value())
      {
        getDataStructure().setAdditionalParent(objectId, parentId.value());
        return {};
      }

      // Get DataObject type for factory
      auto typeNameResult = childObj.readStringAttribute(Constants::k_ObjectTypeTag);
      if(typeNameResult.invalid())
      {
        return ConvertResult(std::move(typeNameResult));
      }
      std::string typeName = std::move(typeNameResult.value());

      factory = getDataFactory(typeName);
    }
  }

  // Return an error if the factory could not be found.
  if(factory == nullptr)
  {
    std::string ss = fmt::format("Could not find the corresponding data factory for '{}' under parent path '{}'", objectName, parentGroup.getObjectPath());
    return MakeErrorResult<>(-3, ss);
  }

  // Read DataObject from Factory
  {
    auto errorCode = factory->readData(*this, parentGroup, objectName, objectId, parentId, useEmptyDataStores);
    if(errorCode.invalid())
    {
      return errorCode;
    }
  }

  return {};
}

DataStructure& DataStructureReader::getDataStructure()
{
  return m_CurrentStructure;
}

void DataStructureReader::clearDataStructure()
{
  m_CurrentStructure = DataStructure();
}

std::shared_ptr<DataIOManager> DataStructureReader::getDataReader() const
{
  if(m_IOManager != nullptr)
  {
    return m_IOManager;
  }

  return Application::GetOrCreateInstance()->getIOManagerAs<DataIOManager>("HDF5");
}

std::shared_ptr<IDataIO> DataStructureReader::getDataFactory(typename IDataIOManager::factory_id_type typeName) const
{
  return getDataReader()->getFactoryAs<IDataIO>(typeName);
}

} // namespace nx::core::HDF5
