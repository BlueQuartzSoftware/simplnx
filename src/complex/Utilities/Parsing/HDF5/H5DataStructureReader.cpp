#include "H5DataStructureReader.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

using namespace complex;

H5::DataStructureReader::DataStructureReader(H5DataReader* h5DataReader)
: m_DataReader(h5DataReader)
{
}

H5::DataStructureReader::~DataStructureReader() = default;

complex::DataStructure H5::DataStructureReader::readH5Group(const H5::GroupReader& groupReader, H5::ErrorType& errorCode)
{
  clearDataStructure();

  auto rootGroupReader = groupReader.openGroup(Constants::k_DataStructureTag);
  if(!rootGroupReader.isValid())
  {
    errorCode = -1;
    return {};
  }

  auto idAttribute = rootGroupReader.getAttribute(Constants::k_NextIdTag);
  if(!idAttribute.isValid())
  {
    errorCode = -2;
    return {};
  }

  m_CurrentStructure = DataStructure();
  m_CurrentStructure.setNextId(idAttribute.readAsValue<DataObject::IdType>());
  errorCode = m_CurrentStructure.getRootGroup().readH5Group(*this, rootGroupReader);
  return m_CurrentStructure;
}

H5::ErrorType H5::DataStructureReader::readObjectFromGroup(const H5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId)
{
  IH5DataFactory* factory = nullptr;

  // Get IH5DataFactory and check DataObject ID
  {
    auto childObj = parentGroup.openObject(objectName);
    
    // Check if data has already been read
    auto idAttribute = childObj.getAttribute(complex::Constants::k_ObjectIdTag);
    if(!idAttribute.isValid())
    {
      return -2;
    }
    DataObject::IdType objectId = idAttribute.readAsValue<DataObject::IdType>();
    if(getDataStructure().containsData(objectId))
    {
      getDataStructure().setAdditionalParent(objectId, parentId.value());
      return 0;
    }

    // Get DataObject type for factory
    auto typeAttribute = childObj.getAttribute(complex::Constants::k_ObjectTypeTag);
    if(!typeAttribute.isValid())
    {
      return -1;
    }
    std::string typeName = typeAttribute.readAsString();

    factory = getDataFactory(typeName);
  }

  // Return an error if the factory could not be found.
  if(factory == nullptr)
  {
    return -3;
  }

  // Read DataObject from Factory
  if(parentGroup.isGroup(objectName))
  {
    auto childGroup = parentGroup.openGroup(objectName);
    auto errorCode = factory->readDataStructureGroup(*this, childGroup, parentId);
    if(errorCode < 0)
    {
      return errorCode;
    }
  }
  else if(parentGroup.isDataset(objectName))
  {
    auto childDataset = parentGroup.openDataset(objectName);
    auto errorCode = factory->readDataStructureDataset(*this, childDataset, parentId);
    {
      return errorCode;
    }
  }
  return 0;
}

DataStructure& H5::DataStructureReader::getDataStructure()
{
  return m_CurrentStructure;
}

void H5::DataStructureReader::clearDataStructure()
{
  m_CurrentStructure = DataStructure();
}

H5DataReader* H5::DataStructureReader::getDataReader() const
{
  if(m_DataReader != nullptr)
  {
    return m_DataReader;
  }

  return Application::Instance()->getDataStructureReader();
}

IH5DataFactory* H5::DataStructureReader::getDataFactory(const std::string& typeName) const
{
  return getDataReader()->getFactory(typeName);
}
