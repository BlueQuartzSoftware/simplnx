#include "complex/DataStructure/DataMap.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

using namespace complex;

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

usize DataMap::getSize() const
{
  return m_Map.size();
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
  auto keys = rhs.getKeys();
  for(auto& key : keys)
  {
    DataObject* copy = rhs.m_Map.at(key)->deepCopy();
    m_Map[key] = std::shared_ptr<DataObject>(copy);
  }

  return *this;
}

DataMap& DataMap::operator=(DataMap&& rhs) noexcept
{
  m_Map = std::move(rhs.m_Map);
  return *this;
}

H5::ErrorType DataMap::readH5Group(DataStructure& ds, const H5::GroupReader& h5Group, const std::optional<DataObject::IdType>& dsParentId)
{
  auto count = h5Group.getNumChildren();
  auto childrenNames = h5Group.getChildNames();
  for(const auto& childName : childrenNames)
  {
    IH5DataFactory* factory = nullptr;

    // Get IH5DataFactory
    {
      auto childObj = h5Group.openObject(childName);
      auto typeAttribute = childObj.getAttribute(complex::Constants::k_ObjectTypeTag);
      std::string typeName = typeAttribute.readAsString();

      factory = Application::Instance()->getDataStructureReader()->getFactory(typeName);
    }

    // Return an error if the factory could not be found.
    if(factory == nullptr)
    {
      return -1;
    }

    // Read DataObject from Factory
    if(h5Group.isGroup(childName))
    {
      auto childGroup = h5Group.openGroup(childName);
      if(factory->readDataStructureGroup(ds, childGroup, dsParentId) < 0)
      {
        throw std::runtime_error("Error reading DataMap from HDF5");
      }
    }
    else if(h5Group.isDataset(childName))
    {
      auto childDataset = h5Group.openDataset(childName);
      if(factory->readDataStructureDataset(ds, childDataset, dsParentId) < 0)
      {
        throw std::runtime_error("Error reading DataMap from HDF5");
      }
    }
  }
  return 0;
}

H5::ErrorType DataMap::writeH5Group(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& groupWriter) const
{
  for(const auto& [id, dataObject] : *this)
  {
    herr_t err = dataStructureWriter.writeDataObject(dataObject.get(), groupWriter);
    if(err < 0)
    {
      std::cout << "Error writing object (" << dataObject->getName() << ") in DataMap to HDF5 " << std::endl;
      return err;
    }
  }
  return 0;
}
