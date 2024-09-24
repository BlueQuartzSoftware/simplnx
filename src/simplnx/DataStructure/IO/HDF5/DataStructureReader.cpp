#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/IO/HDF5/BaseGroupIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"

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
  auto groupReaderResult = fileReader.openGroup(Constants::k_DataStructureTag);
  if(groupReaderResult.invalid())
  {
    return ConvertInvalidResult<DataStructure>(std::move(groupReaderResult));
  }
  auto groupReader = std::move(groupReaderResult.value());

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

  DataObject::IdType objectId;
  groupReader.readAttribute(Constants::k_NextIdTag, objectId);

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

    if (isGroup)
    {
      auto childObjResult = parentGroup.openGroup(objectName);
      if(childObjResult.invalid())
      {
        return ConvertResult(std::move(childObjResult));
      }
      auto childObj = std::move(childObjResult.value());

      // Return 0 if object is marked as not importable.
      int32 importable = 0;
      childObj.readAttribute(Constants::k_ImportableTag, importable);
      if(importable == 0)
      {
        return {};
      }

      // Check if data has already been read
      childObj.readAttribute(Constants::k_ObjectIdTag, objectId);
      if(getDataStructure().containsData(objectId))
      {
        getDataStructure().setAdditionalParent(objectId, parentId.value());
        return {};
      }

      // Get DataObject type for factory
      std::string typeName;
      childObj.readAttribute(Constants::k_ObjectTypeTag, typeName);

      factory = getDataFactory(typeName);
    }
    else
    {
      auto childObjResult = parentGroup.openDataset(objectName);
      if(childObjResult.invalid())
      {
        return ConvertResult(std::move(childObjResult));
      }
      auto childObj = std::move(childObjResult.value());

      // Return 0 if object is marked as not importable.
      int32 importable = 0;
      childObj.readAttribute(Constants::k_ImportableTag, importable);
      if(importable == 0)
      {
        return {};
      }

      // Check if data has already been read
      childObj.readAttribute(Constants::k_ObjectIdTag, objectId);
      if(getDataStructure().containsData(objectId))
      {
        getDataStructure().setAdditionalParent(objectId, parentId.value());
        return {};
      }

      // Get DataObject type for factory
      std::string typeName;
      childObj.readAttribute(Constants::k_ObjectTypeTag, typeName);

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
