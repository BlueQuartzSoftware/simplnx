#include "AbstractDataIOManager.hpp"

namespace nx::core
{
AbstractDataIOManager::AbstractDataIOManager() = default;
AbstractDataIOManager::~AbstractDataIOManager() noexcept = default;

AbstractDataIOManager::factory_collection AbstractDataIOManager::getFactories() const
{
  return m_FactoryCollection;
}

AbstractDataIOManager::factory_ptr AbstractDataIOManager::getFactory(factory_id_type typeName) const
{
  if(m_FactoryCollection.find(typeName) == m_FactoryCollection.end())
  {
    return nullptr;
  }
  return m_FactoryCollection.at(typeName);
}

AbstractDataIOManager::DataStoreCreationMap AbstractDataIOManager::getDataStoreCreationFunctions()
{
  return m_DataStoreCreationMap;
}

bool AbstractDataIOManager::hasDataStoreCreationFnc(const std::string& type) const
{
  return m_DataStoreCreationMap.find(type) != m_DataStoreCreationMap.end();
}

AbstractDataIOManager::DataStoreCreateFnc AbstractDataIOManager::dataStoreCreationFnc(const std::string& type) const
{
  return m_DataStoreCreationMap.at(type);
}

void AbstractDataIOManager::addDataStoreCreationFnc(const std::string& type, DataStoreCreateFnc creationFnc)
{
  m_DataStoreCreationMap[type] = creationFnc;
}

bool AbstractDataIOManager::hasListStoreCreationFnc(const std::string& type) const
{
  return m_ListStoreCreationMap.find(type) != m_ListStoreCreationMap.end();
}

AbstractDataIOManager::ListStoreCreateFnc AbstractDataIOManager::listStoreCreationFnc(const std::string& type) const
{
  return m_ListStoreCreationMap.at(type);
}

void AbstractDataIOManager::addListStoreCreationFnc(const std::string& type, ListStoreCreateFnc creationFnc)
{
  m_ListStoreCreationMap[type] = creationFnc;
}
} // namespace nx::core
