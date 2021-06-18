#include "DataObject.hpp"

#include <algorithm>
#include <exception>

#include "Complex/DataStructure/DataStructure.hpp"
#include "Complex/DataStructure/BaseGroup.hpp"
#include "Complex/DataStructure/Metadata.hpp"

using namespace complex;

DataObject::IdType DataObject::generateId(const std::optional<IdType>& opId)
{
  static IdType id = 0;
  if(opId && opId > id)
  {
    id = opId.value();
  }
  return id++;
}

DataObject::DataObject(DataStructure* ds, const std::string& name)
: m_Name(name)
, m_DataStructure(ds)
, m_Id(generateId())
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
, m_DataStructure(std::move(other.m_DataStructure))
, m_Id(std::move(other.m_Id))
, m_H5Id(std::move(other.m_H5Id))
{
}

DataObject::~DataObject()
{
  getDataStructure()->dataDeleted(getId(), getName());
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
  for(BaseGroup* parent : m_ParentList)
  {
    if(parent->contains(name))
    {
      return false;
    }
  }
  return true;
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
  if(m_ParentList.size() == 0)
  {
    return {DataPath({getName()})};
  }

  std::vector<DataPath> paths;
  for(auto parent : m_ParentList)
  {
    auto parentPaths = parent->getDataPaths();
    for(auto dataPath : parentPaths)
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
