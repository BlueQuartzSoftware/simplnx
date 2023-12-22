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
  const nx::core::HDF5::FileReader fileReader(path);
  return ReadFile(fileReader);
}
Result<DataStructure> DataStructureReader::ReadFile(const nx::core::HDF5::FileReader& fileReader, bool useEmptyDataStores)
{
  DataStructureReader dataStructureReader;
  auto groupReader = fileReader.openGroup(Constants::k_DataStructureTag);
  return dataStructureReader.readGroup(groupReader, useEmptyDataStores);
}

Result<DataStructure> DataStructureReader::readGroup(const nx::core::HDF5::GroupReader& groupReader, bool useEmptyDataStores)
{
  clearDataStructure();

  if(!groupReader.isValid())
  {
    std::string ss = fmt::format("Failed to open top-level DataStructure group");
    return MakeErrorResult<DataStructure>(-1, ss);
  }

  auto idAttribute = groupReader.getAttribute(Constants::k_NextIdTag);
  if(!idAttribute.isValid())
  {
    std::string ss = fmt::format("Failed to access DataStructure {} attribute", Constants::k_NextIdTag);
    return MakeErrorResult<DataStructure>(-2, ss);
  }

  m_CurrentStructure = DataStructure();
  m_CurrentStructure.setNextId(idAttribute.readAsValue<DataObject::IdType>());
  Result<> result = HDF5::ReadDataMap(*this, m_CurrentStructure.getRootGroup(), groupReader, {}, useEmptyDataStores);
  if(result.invalid())
  {
    auto& error = result.errors()[0];
    return MakeErrorResult<DataStructure>(error.code, error.message);
  }
  return {m_CurrentStructure};
}

Result<> DataStructureReader::readObjectFromGroup(const nx::core::HDF5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId,
                                                  bool useEmptyDataStores)
{
  std::shared_ptr<IDataIO> factory = nullptr;
  DataObject::IdType objectId = 0;

  // Get nx::core::HDF5::IDataFactory and check DataObject ID
  {
    auto childObj = parentGroup.openObject(objectName);

    // Return 0 if object is marked as not importable.
    auto importAttribute = childObj.getAttribute(Constants::k_ImportableTag);
    if(importAttribute.isValid())
    {
      const auto importable = importAttribute.readAsValue<int32>();
      if(importable == 0)
      {
        return {};
      }
    }

    // Check if data has already been read
    auto idAttribute = childObj.getAttribute(Constants::k_ObjectIdTag);
    if(!idAttribute.isValid())
    {
      // AttributeMatrix Data
      return {};
    }
    objectId = idAttribute.readAsValue<DataObject::IdType>();
    if(getDataStructure().containsData(objectId))
    {
      getDataStructure().setAdditionalParent(objectId, parentId.value());
      return {};
    }

    // Get DataObject type for factory
    auto typeAttribute = childObj.getAttribute(Constants::k_ObjectTypeTag);
    if(!typeAttribute.isValid())
    {
      std::string ss = "Could not read ObjectType attribute";
      return MakeErrorResult<>(-1, ss);
    }
    const std::string typeName = typeAttribute.readAsString();

    factory = getDataFactory(typeName);
  }

  // Return an error if the factory could not be found.
  if(factory == nullptr)
  {
    std::string ss = fmt::format("Could not find the corresponding data factory");
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
