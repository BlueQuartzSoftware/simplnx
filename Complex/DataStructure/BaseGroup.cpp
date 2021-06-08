#include "BaseGroup.h"

#include <exception>

using namespace Complex;

BaseGroup::BaseGroup(DataStructure* ds, const std::string& name)
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

DataMap BaseGroup::getDataMap() const
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
  if(nullptr == obj)
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
  if(nullptr == obj)
  {
    return false;
  }
  return m_DataMap.remove(obj->getId());
}
bool BaseGroup::remove(const std::string& name)
{
  for(auto& iter = m_DataMap.begin(); iter != m_DataMap.end(); iter++)
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
