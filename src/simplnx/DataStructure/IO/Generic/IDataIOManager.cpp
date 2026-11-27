#include "IDataIOManager.hpp"

#include <utility>

#include "simplnx/DataStructure/AbstractStringStore.hpp"

namespace nx::core
{
IDataIOManager::IDataIOManager() = default;
IDataIOManager::~IDataIOManager() noexcept = default;

IDataIOManager::FactoryCollectionType IDataIOManager::getFactories() const
{
  return m_FactoryCollection;
}

IDataIOManager::FactoryPtrType IDataIOManager::getFactory(const FactoryIdType& typeName) const
{
  if(m_FactoryCollection.find(typeName) == m_FactoryCollection.end())
  {
    return nullptr;
  }
  return m_FactoryCollection.at(typeName);
}

IDataIOManager::DataStoreCreationMapType IDataIOManager::getDataStoreCreationFunctions()
{
  return m_DataStoreCreationMap;
}

bool IDataIOManager::hasDataStoreCreationFnc(const std::string& type) const
{
  return m_DataStoreCreationMap.find(type) != m_DataStoreCreationMap.end();
}

IDataIOManager::DataStoreCreateFncType IDataIOManager::dataStoreCreationFnc(const std::string& type) const
{
  return m_DataStoreCreationMap.at(type);
}

void IDataIOManager::addDataStoreCreationFnc(const std::string& type, DataStoreCreateFncType creationFnc)
{
  m_DataStoreCreationMap[type] = std::move(creationFnc);
}

bool IDataIOManager::hasListStoreCreationFnc(const std::string& type) const
{
  return m_ListStoreCreationMap.find(type) != m_ListStoreCreationMap.end();
}

IDataIOManager::ListStoreCreateFncType IDataIOManager::listStoreCreationFnc(const std::string& type) const
{
  return m_ListStoreCreationMap.at(type);
}

void IDataIOManager::addListStoreCreationFnc(const std::string& type, ListStoreCreateFncType creationFnc)
{
  m_ListStoreCreationMap[type] = std::move(creationFnc);
}

bool IDataIOManager::hasStringStoreCreationFnc(const std::string& type) const
{
  return m_StringStoreCreationMap.find(type) != m_StringStoreCreationMap.cend();
}

IDataIOManager::StringStoreCreateFncType IDataIOManager::stringStoreCreationFnc(const std::string& type) const
{
  auto iter = m_StringStoreCreationMap.find(type);
  if(iter == m_StringStoreCreationMap.cend())
  {
    return nullptr;
  }
  return iter->second;
}

void IDataIOManager::addStringStoreCreationFnc(const std::string& type, StringStoreCreateFncType creationFnc)
{
  m_StringStoreCreationMap[type] = std::move(creationFnc);
}
} // namespace nx::core
