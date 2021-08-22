#include "complex/DataStructure/DataMap.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

using namespace complex;

DataMap::DataMap()
{
}

DataMap::DataMap(const DataMap& other)
: m_Map(other.m_Map)
{
  auto keys = other.getKeys();
  for(auto& key : keys)
  {
    DataObject* copy = other.m_Map.at(key)->deepCopy();
    m_Map[key] = std::shared_ptr<DataObject>(copy);
  }
}

DataMap::DataMap(DataMap&& other) noexcept
: m_Map(std::move(other.m_Map))
{
}

DataMap::~DataMap() = default;

DataMap DataMap::deepCopy() const
{
  DataMap dataMap(*this);

  auto keys = getKeys();
  for(auto& key : keys)
  {
    DataObject* copy = m_Map.at(key)->deepCopy();
    dataMap.m_Map[key] = std::shared_ptr<DataObject>(copy);
  }
  return dataMap;
}

bool DataMap::insert(const std::shared_ptr<DataObject>& obj)
{
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

bool DataMap::remove(IdType id)
{
  auto iter = m_Map.find(id);
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

std::vector<DataMap::IdType> DataMap::getKeys() const
{
  std::vector<IdType> keys(m_Map.size());
  size_t index = 0;
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

bool DataMap::contains(const std::string& name) const
{
  for(auto& iter : m_Map)
  {
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

bool DataMap::contains(IdType id) const
{
  return m_Map.find(id) != m_Map.end();
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

DataMap::Iterator DataMap::find(IdType id)
{
  return m_Map.find(id);
}

DataMap::ConstIterator DataMap::find(IdType id) const
{
  return m_Map.find(id);
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
  auto keys = getKeys();
  for(auto& key : keys)
  {
    auto shareData = dataStr->getSharedData(key);
    shareData->setDataStructure(dataStr);
    m_Map[key] = shareData;
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
  return *this;
}

DataMap& DataMap::operator=(DataMap&& rhs) noexcept
{
  m_Map = std::move(rhs.m_Map);
  return *this;
}

H5::ErrorType DataMap::readH5Group(DataStructure& ds, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  const std::string ObjectTypeTag = H5::Constants::DataObject::ObjectTypeTag;

  hsize_t count;
  H5Gget_num_objs(groupId, &count);
  for(hsize_t i = 0; i < count; i++)
  {
    const std::string name = H5::Reader::Generic::getNameAtIdx(groupId, i);
    hid_t objId = H5Gopen(groupId, name.c_str(), H5P_DEFAULT);
    std::string typeName;
    H5::Reader::Generic::readStringAttribute(groupId, name, ObjectTypeTag, typeName);
    //H5::Reader::Generic::readStringAttribute(objId, ObjectTypeTag, ObjectTypeTag, typeName);

    IH5DataFactory* factory = Application::Instance()->getDataStructureReader()->getFactory(typeName);
    if(factory->createFromHdf5(ds, objId, groupId, parentId) < 0)
    {
      throw std::runtime_error("Error reading DataMap from HDF5");
    }

    H5Gclose(objId);
  }
  return 0;
}

H5::ErrorType DataMap::writeH5Group(H5::IdType groupId) const
{
  for(const auto& iter : *this)
  {
    herr_t err = iter.second->writeHdf5(groupId);
    if(err < 0)
    {
      return err;
    }
  }
  return 0;
}
