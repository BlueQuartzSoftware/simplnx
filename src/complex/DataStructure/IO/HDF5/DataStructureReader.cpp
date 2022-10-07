#include "complex/DataStructure/IO/HDF5/DataStructureReader.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/IO/HDF5/BaseGroupIO.hpp"
#include "complex/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"
#include "complex/DataStructure/IO/HDF5/IOUtilities.hpp"

#include "fmt/format.h"

namespace complex
{
namespace HDF5
{
DataStructureReader::DataStructureReader(DataIOManager* factoryManager)
: m_IOManager(factoryManager)
{
}
DataStructureReader::~DataStructureReader() noexcept = default;

Result<DataStructure> DataStructureReader::readFile(const std::filesystem::path& path)
{
  const H5::FileReader fileReader(path);
  return readFile(fileReader);
}
Result<DataStructure> DataStructureReader::readFile(const H5::FileReader& fileReader)
{
  return readGroup(fileReader);
}

Result<DataStructure> DataStructureReader::readGroup(const H5::GroupReader& groupReader, bool useEmptyDataStores)
{
  clearDataStructure();

  auto rootGroupReader = groupReader.openGroup(Constants::k_DataStructureTag);
  if(!rootGroupReader.isValid())
  {
    std::string ss = fmt::format("Failed to open top-level DataStructure group");
    return MakeErrorResult<DataStructure>(-1, ss);
  }

  auto idAttribute = rootGroupReader.getAttribute(Constants::k_NextIdTag);
  if(!idAttribute.isValid())
  {
    std::string ss = fmt::format("Failed to access DataStructure {} attribute", Constants::k_NextIdTag);
    return MakeErrorResult<DataStructure>(-2, ss);
  }

  m_CurrentStructure = DataStructure();
  m_CurrentStructure.setNextId(idAttribute.readAsValue<DataObject::IdType>());
  Result<> result = HDF5::ReadDataMap(*this, m_CurrentStructure.getRootGroup(), rootGroupReader, {}, useEmptyDataStores);
  if(result.invalid())
  {
    auto& error = result.errors()[0];
    return MakeErrorResult<DataStructure>(error.code, error.message);
  }
  return {m_CurrentStructure};
}

Result<> DataStructureReader::readObjectFromGroup(const H5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStores)
{
  std::shared_ptr<IDataIO> factory = nullptr;
  DataObject::IdType objectId = 0;

  // Get H5::IDataFactory and check DataObject ID
  {
    auto childObj = parentGroup.openObject(objectName);

    // Return 0 if object is marked as not importable.
    auto importAttribute = childObj.getAttribute(complex::Constants::k_ImportableTag);
    if(importAttribute.isValid())
    {
      const auto importable = importAttribute.readAsValue<int32>();
      if(importable == 0)
      {
        return {};
      }
    }

    // Check if data has already been read
    auto idAttribute = childObj.getAttribute(complex::Constants::k_ObjectIdTag);
    if(!idAttribute.isValid())
    {
      // AttributeMatrix Data
      return {};
      // std::string ss = "Could not read ObjectID attribute";
      // return MakeErrorResult(-2, ss);
    }
    objectId = idAttribute.readAsValue<DataObject::IdType>();
    if(getDataStructure().containsData(objectId))
    {
      getDataStructure().setAdditionalParent(objectId, parentId.value());
      return {};
    }

    // Get DataObject type for factory
    auto typeAttribute = childObj.getAttribute(complex::Constants::k_ObjectTypeTag);
    if(!typeAttribute.isValid())
    {
      std::string ss = "Could not read ObjectType attribute";
      return MakeErrorResult<>(-1, ss);
    }
    const std::string typeName = typeAttribute.readAsString();
    // const auto type = Application::Instance()->getDataType(typeName);

    factory = getDataFactory(typeName);
  }

  // Return an error if the factory could not be found.
  if(factory == nullptr)
  {
    std::string ss = fmt::format("Could not find the corresponding data factory");
    return MakeErrorResult<>(-3, ss);
  }

  // Read DataObject from Factory
  // if(parentGroup.isGroup(objectName))
  {
    // auto childGroup = parentGroup.openGroup(objectName);
    auto errorCode = factory->readData(*this, parentGroup, objectName, {objectId}, parentId, useEmptyDataStores);
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

  return Application::Instance()->getIOManagerAs<DataIOManager>("HDF5");
}

std::shared_ptr<IDataIO> DataStructureReader::getDataFactory(typename IDataIOManager::factory_id_type typeName) const
{
  return getDataReader()->getFactoryAs<IDataIO>(typeName);
}

} // namespace HDF5
} // namespace complex
