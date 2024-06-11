#include "simplnx/DataStructure/DataMap.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include <algorithm>

using namespace nx::core;

DataMap::DataMap() = default;
DataMap::DataMap(const DataMap& other)
: m_Map(other.m_Map)
{
}

DataMap::DataMap(DataMap&& other) noexcept
: m_Map(std::move(other.m_Map))
{
}

DataMap::~DataMap() = default;

DataMap DataMap::deepCopy(const DataPath& parentCopyPath) const
{
  DataMap dataMap(*this);

  const auto keys = getKeys();
  for(auto& key : keys)
  {
    const auto& dataObj = m_Map.at(key);
    const auto copy = dataObj->deepCopy(parentCopyPath.createChildPath(dataObj->getName()));
    dataMap.m_Map[key] = copy;
  }
  return dataMap;
}

usize DataMap::getSize() const
{
  return m_Map.size();
}

bool DataMap::empty() const
{
  return m_Map.empty();
}

bool DataMap::insert(const std::shared_ptr<DataObject>& obj)
{
  if(obj == nullptr)
  {
    return false;
  }

  m_Map[obj->getId()] = obj;
  return true;
}

bool DataMap::remove(DataObject* obj)
{
  if(obj == nullptr)
  {
    return false;
  }
  return remove(obj->getId());
}

bool DataMap::remove(IdType identifier)
{
  auto iter = m_Map.find(identifier);
  bool found = (iter != m_Map.end());
  erase(iter);
  return found;
}

bool DataMap::erase(const Iterator& iter)
{
  if(iter == end())
  {
    return false;
  }
  m_Map.erase(iter);
  return true;
}

void DataMap::clear()
{
  m_Map.clear();
}

std::vector<DataMap::IdType> DataMap::getKeys() const
{
  std::vector<IdType> keys(m_Map.size());
  usize index = 0;
  for(auto& pair : m_Map)
  {
    keys[index++] = pair.first;
  }
  return keys;
}

std::vector<DataMap::IdType> DataMap::getAllKeys() const
{
  std::vector<IdType> keys;
  for(auto& pair : m_Map)
  {
    keys.push_back(pair.first);
    if(const BaseGroup* group = dynamic_cast<BaseGroup*>(pair.second.get()))
    {
      auto childKeys = group->getDataMap().getAllKeys();
      keys.insert(keys.end(), childKeys.begin(), childKeys.end());
    }
  }
  return keys;
}

std::map<DataMap::IdType, std::weak_ptr<DataObject>> DataMap::getAllItems() const
{
  std::map<IdType, std::weak_ptr<DataObject>> map;
  for(auto& pair : m_Map)
  {
    map[pair.first] = pair.second;
    if(const BaseGroup* group = dynamic_cast<BaseGroup*>(pair.second.get()))
    {
      map.merge(group->getDataMap().getAllItems());
    }
  }
  return map;
}

std::vector<std::string> DataMap::getNames() const
{
  std::vector<std::string> names;
  for(const auto& [identifier, data] : m_Map)
  {
    names.push_back(data->getName());
  }
  std::sort(names.begin(), names.end());
  return names;
}

bool DataMap::contains(const std::string& name) const
{
  for(auto& iter : m_Map)
  {
    if (iter.second == nullptr)
    {
      continue;
    }
    if(iter.second->getName() == name)
    {
      return true;
    }
  }
  return false;
}

bool DataMap::contains(const DataObject* obj) const
{
  if(obj == nullptr)
  {
    return false;
  }
  return contains(obj->getId());
}

bool DataMap::contains(IdType identifier) const
{
  return m_Map.find(identifier) != m_Map.end();
}

DataObject* DataMap::operator[](IdType key)
{
  if(contains(key))
  {
    return m_Map.at(key).get();
  }
  return nullptr;
}

const DataObject* DataMap::operator[](IdType key) const
{
  if(contains(key))
  {
    return m_Map.at(key).get();
  }
  return nullptr;
}

DataObject* DataMap::operator[](const std::string& name)
{
  for(auto& iter : m_Map)
  {
    if(iter.second->getName() == name)
    {
      return iter.second.get();
    }
  }
  return nullptr;
}

const DataObject* DataMap::operator[](const std::string& name) const
{
  for(auto& iter : m_Map)
  {
    if(iter.second->getName() == name)
    {
      return iter.second.get();
    }
  }
  return nullptr;
}

DataObject& DataMap::at(const std::string& name)
{
  DataObject* object = (*this)[name];
  if(object == nullptr)
  {
    throw std::invalid_argument(fmt::format("DataMap::at: unable to find object with name '{}'", name));
  }
  return *object;
}

const DataObject& DataMap::at(const std::string& name) const
{
  const DataObject* object = (*this)[name];
  if(object == nullptr)
  {
    throw std::invalid_argument(fmt::format("DataMap::at: unable to find object with name '{}'", name));
  }
  return *object;
}

DataMap::Iterator DataMap::find(IdType identifier)
{
  return m_Map.find(identifier);
}

DataMap::ConstIterator DataMap::find(IdType identifier) const
{
  return m_Map.find(identifier);
}

DataMap::Iterator DataMap::find(const std::string& name)
{
  for(auto iter = begin(); iter != end(); iter++)
  {
    if((*iter).second->getName() == name)
    {
      return iter;
    }
  }
  return end();
}

DataMap::ConstIterator DataMap::find(const std::string& name) const
{
  for(auto iter = begin(); iter != end(); iter++)
  {
    if((*iter).second->getName() == name)
    {
      return iter;
    }
  }
  return end();
}

void DataMap::setDataStructure(DataStructure* dataStr)
{
  if(dataStr == nullptr)
  {
    return;
  }

  for(auto& [key, ptr] : *this)
  {
    // Replace shared_ptr with the corresponding object from the target DataStructure
    auto shareData = dataStr->getSharedData(key);
    if(shareData == nullptr)
    {
      continue;
    }
    shareData->setDataStructure(dataStr);
    m_Map[key] = shareData;
    dataStr->setData(key, shareData);
  }
}

DataMap::Iterator DataMap::begin()
{
  return m_Map.begin();
}
DataMap::Iterator DataMap::end()
{
  return m_Map.end();
}
DataMap::ConstIterator DataMap::begin() const
{
  return m_Map.begin();
}
DataMap::ConstIterator DataMap::end() const
{
  return m_Map.end();
}

DataMap& DataMap::operator=(const DataMap& rhs)
{
  m_Map = rhs.m_Map;
  auto keys = rhs.getKeys();
  for(auto& key : keys)
  {
    DataObject* copy = rhs.m_Map.at(key)->shallowCopy();
    m_Map[key] = std::shared_ptr<DataObject>(copy);
  }

  return *this;
}

DataMap& DataMap::operator=(DataMap&& rhs) noexcept
{
  m_Map = std::move(rhs.m_Map);
  return *this;
}

void DataMap::updateIds(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  using UpdatedValueType = std::pair<IdType, std::shared_ptr<DataObject>>;
  std::list<UpdatedValueType> movedValues;

  for(const auto& updatedId : updatedIds)
  {
    if(!contains(updatedId.first))
    {
      continue;
    }

    // Copy and remove updated values
    auto ptr = m_Map[updatedId.first];
    movedValues.emplace_back(UpdatedValueType{updatedId.first, ptr});
    m_Map.erase(updatedId.first);
  }

  // Re-assign updated values
  for(const UpdatedValueType& updatedValue : movedValues)
  {
    m_Map[updatedValue.first] = updatedValue.second;
  }
}
