#include "DataObject.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Metadata.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

#include <algorithm>
#include <exception>

using namespace complex;

DataObject::DataObject(DataStructure& ds, std::string name)
: m_Name(std::move(name))
, m_DataStructure(&ds)
, m_Id(ds.generateId())
, m_H5Id(-1)
{
}

DataObject::DataObject(const DataObject& other)
: m_Name(other.m_Name)
, m_DataStructure(other.m_DataStructure)
, m_Id(other.m_Id)
, m_H5Id(other.m_H5Id)
{
}

DataObject::DataObject(DataObject&& other) noexcept
: m_Name(std::move(other.m_Name))
, m_DataStructure(other.m_DataStructure)
, m_Id(other.m_Id)
, m_H5Id(other.m_H5Id)
{
}

DataObject::~DataObject()
{
  getDataStructure()->dataDeleted(getId(), getName());
}

bool DataObject::AttemptToAddObject(DataStructure& ds, const std::shared_ptr<DataObject>& data, const std::optional<IdType>& parentId)
{
  return ds.finishAddingObject(data, parentId);
}

DataObject::IdType DataObject::getId() const
{
  return m_Id;
}

DataStructure* DataObject::getDataStructure()
{
  return m_DataStructure;
}

const DataStructure* DataObject::getDataStructure() const
{
  return m_DataStructure;
}

void DataObject::setDataStructure(DataStructure* ds)
{
  m_DataStructure = ds;
}

std::string DataObject::getName() const
{
  return m_Name;
}

bool DataObject::canRename(const std::string& name) const
{
  return !std::any_of(m_ParentList.cbegin(), m_ParentList.cend(), [name](BaseGroup* parent) { return parent->contains(name); });
}

bool DataObject::rename(const std::string& name)
{
  if(!canRename(name))
  {
    return false;
  }

  m_Name = name;
  return true;
}

DataObject::ParentCollectionType DataObject::getParents() const
{
  return m_ParentList;
}

Metadata* DataObject::getMetadata() const
{
  return m_Metadata.get();
}

void DataObject::addParent(BaseGroup* parent)
{
  m_ParentList.push_back(parent);
}

void DataObject::removeParent(BaseGroup* parent)
{
  m_ParentList.remove(parent);
}

void DataObject::replaceParent(BaseGroup* oldParent, BaseGroup* newParent)
{
  std::replace(m_ParentList.begin(), m_ParentList.end(), oldParent, newParent);
}

std::vector<DataPath> DataObject::getDataPaths() const
{
  if(m_ParentList.empty())
  {
    return {DataPath({getName()})};
  }

  std::vector<DataPath> paths;
  for(auto* parent : m_ParentList)
  {
    auto parentPaths = parent->getDataPaths();
    for(auto& dataPath : parentPaths)
    {
      paths.push_back(dataPath.createChildPath(m_Name));
    }
  }
  return paths;
}

bool DataObject::hasH5Id() const
{
  return -1 != m_H5Id;
}

H5::IdType DataObject::getH5Id() const
{
  return m_H5Id;
}

H5::ErrorType DataObject::writeH5ObjectAttributes(H5::DataStructureWriter& dataStructureWriter, H5::ObjectWriter& objectWriter) const
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addH5Writer(objectWriter, getId());

  auto typeAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ObjectTypeTag);
  auto error = typeAttributeWriter.writeString(getTypeName());
  if(error < 0)
  {
    return error;
  }

  auto idAttributeWriter = objectWriter.createAttribute(complex::Constants::k_ObjectIdTag);
  error = idAttributeWriter.writeValue(getId());

  return error;
}
