#include "ZarrStructureReader.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrDataFactoryManager.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrIDataFactory.hpp"

#include "FileVec/collection/Group.hpp"

using namespace complex;

Zarr::DataStructureReader::DataStructureReader(DataFactoryManager* h5FactoryManager)
: m_FactoryManager(h5FactoryManager)
{
}

Zarr::DataStructureReader::~DataStructureReader() = default;

complex::DataStructure Zarr::DataStructureReader::readGroup(const FileVec::Group& groupReader, Zarr::ErrorType& errorCode, bool preflight)
{
  clearDataStructure();

  auto rootGroupReader = groupReader.findGroup(Constants::k_DataStructureTag);
  if(rootGroupReader == nullptr)
  {
    errorCode = -1;
    return {};
  }

  nlohmann::json idAttribute = rootGroupReader->attributes()[Constants::k_NextIdTag];
  if(idAttribute.empty())
  {
    errorCode = -2;
    return {};
  }

  m_CurrentStructure = DataStructure();
  m_CurrentStructure.setNextId(idAttribute.get<DataObject::IdType>());
  errorCode = m_CurrentStructure.getRootGroup().readZarGroup(*this, *rootGroupReader, {}, preflight);
  return std::move(m_CurrentStructure);
}

Zarr::ErrorType Zarr::DataStructureReader::readObjectFromGroup(const FileVec::Group& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  Zarr::IDataFactory* factory = nullptr;

  // Get Zarr::IDataFactory and check DataObject ID
  {
    auto* childPtr = parentGroup.find(objectName)->get();
    if(childPtr == nullptr)
    {
      return -78;
    }

    const auto& childObj = *childPtr;

    // Return 0 if object is marked as not importable.
    nlohmann::json importAttribute = childObj.attributes()[complex::Constants::k_ImportableTag];
    if(!importAttribute.empty())
    {
      const auto importable = importAttribute.get<int32>();
      if(importable == 0)
      {
        return 0;
      }
    }

    // Check if data has already been read
    nlohmann::json idAttribute = childObj.attributes()[complex::Constants::k_ObjectIdTag];
    if(idAttribute.empty())
    {
      return -2;
    }
    DataObject::IdType objectId = idAttribute.get<DataObject::IdType>();
    if(getDataStructure().containsData(objectId))
    {
      getDataStructure().setAdditionalParent(objectId, parentId.value());
      return 0;
    }

    // Get DataObject type for factory
    nlohmann::json typeAttribute = childObj.attributes()[complex::Constants::k_ObjectTypeTag];
    if(typeAttribute.empty())
    {
      return -1;
    }
    const std::string typeName = typeAttribute.get<std::string>();

    factory = getDataFactory(typeName);
  }

  // Return an error if the factory could not be found.
  if(factory == nullptr)
  {
    return -3;
  }

  // Read DataObject from Factory
  const auto childGroupPtr = parentGroup.find(objectName);
  const ErrorType errorCode = factory->read(*this, parentGroup, *childGroupPtr->get(), parentId, preflight);
  return errorCode;
}

DataStructure& Zarr::DataStructureReader::getDataStructure()
{
  return m_CurrentStructure;
}

void Zarr::DataStructureReader::clearDataStructure()
{
  m_CurrentStructure = DataStructure();
}

Zarr::DataFactoryManager* Zarr::DataStructureReader::getDataReader() const
{
  if(m_FactoryManager != nullptr)
  {
    return m_FactoryManager;
  }

  return Application::Instance()->getZarrFactoryManager();
}

Zarr::IDataFactory* Zarr::DataStructureReader::getDataFactory(const std::string& typeName) const
{
  return getDataReader()->getFactory(typeName);
}
