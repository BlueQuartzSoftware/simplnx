#include "BaseGroup.hpp"

#include <exception>

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

using namespace complex;

BaseGroup::BaseGroup(DataStructure& ds, const std::string& name)
: DataObject(ds, name)
{
}

BaseGroup::BaseGroup(const BaseGroup& other)
: DataObject(other)
, m_DataMap(other.m_DataMap)
{
}

BaseGroup::BaseGroup(BaseGroup&& other) noexcept
: DataObject(std::move(other))
, m_DataMap(std::move(other.m_DataMap))
{
  auto keys = m_DataMap.getKeys();
  for(auto& key : keys)
  {
    m_DataMap[key]->replaceParent(&other, this);
  }
}

BaseGroup::~BaseGroup() = default;

const DataMap& BaseGroup::getDataMap() const
{
  return m_DataMap;
}

usize BaseGroup::getSize() const
{
  return m_DataMap.getSize();
}

DataMap& BaseGroup::getDataMap()
{
  return m_DataMap;
}

bool BaseGroup::contains(const std::string& name) const
{
  return m_DataMap.contains(name);
}

bool BaseGroup::contains(const DataObject* obj) const
{
  return m_DataMap.contains(obj);
}

DataObject* BaseGroup::operator[](const std::string& name)
{
  return m_DataMap[name];
}

const DataObject* BaseGroup::operator[](const std::string& name) const
{
  return m_DataMap[name];
}

bool BaseGroup::canInsert(const DataObject* obj) const
{
  if(obj == nullptr)
  {
    return false;
  }
  if(contains(obj))
  {
    return false;
  }
  return true;
}

void BaseGroup::setDataStructure(DataStructure* ds)
{
  DataObject::setDataStructure(ds);
  m_DataMap.setDataStructure(ds);
}

BaseGroup::Iterator BaseGroup::find(const std::string& name)
{
  return m_DataMap.find(name);
}

BaseGroup::ConstIterator BaseGroup::find(const std::string& name) const
{
  return m_DataMap.find(name);
}

bool BaseGroup::insert(const std::weak_ptr<DataObject>& obj)
{
  auto ptr = obj.lock();
  if(!canInsert(ptr.get()))
  {
    return false;
  }
  if(m_DataMap.insert(ptr))
  {
    ptr->addParent(this);
    return true;
  }
  return false;
}
bool BaseGroup::remove(DataObject* obj)
{
  if(obj == nullptr)
  {
    return false;
  }
  return m_DataMap.remove(obj->getId());
}
bool BaseGroup::remove(const std::string& name)
{
  for(auto iter = m_DataMap.begin(); iter != m_DataMap.end(); iter++)
  {
    if((*iter).second->getName() == name)
    {
      (*iter).second->removeParent(this);
      m_DataMap.erase(iter);
      return true;
    }
  }
  return false;
}
BaseGroup::Iterator BaseGroup::begin()
{
  return m_DataMap.begin();
}
BaseGroup::Iterator BaseGroup::end()
{
  return m_DataMap.end();
}
BaseGroup::ConstIterator BaseGroup::begin() const
{
  return m_DataMap.begin();
}
BaseGroup::ConstIterator BaseGroup::end() const
{
  return m_DataMap.end();
}

H5::ErrorType BaseGroup::readHdf5(const H5::GroupReader& groupReader)
{
  return m_DataMap.readH5Group(*getDataStructure(), groupReader, getId());
}

H5::ErrorType BaseGroup::writeHdf5(H5::GroupWriter& parentGroupWriter) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  writeHdf5DataType(groupWriter);

  return m_DataMap.writeH5Group(groupWriter);
}
