#include "IDataIOManager.hpp"

namespace nx::core
{
IDataIOManager::IDataIOManager() = default;
IDataIOManager::~IDataIOManager() noexcept = default;

IDataIOManager::factory_collection IDataIOManager::getFactories() const
{
  return m_FactoryCollection;
}

IDataIOManager::factory_ptr IDataIOManager::getFactory(factory_id_type typeName) const
{
  if(m_FactoryCollection.find(typeName) == m_FactoryCollection.end())
  {
    return nullptr;
  }
  return m_FactoryCollection.at(typeName);
}

IDataIOManager::DataStoreCreationMap IDataIOManager::getDataStoreCreationFunctions()
{
  return m_DataStoreCreationMap;
}

bool IDataIOManager::hasDataStoreCreationFnc(const std::string& type) const
{
  return m_DataStoreCreationMap.find(type) != m_DataStoreCreationMap.end();
}

IDataIOManager::DataStoreCreateFnc IDataIOManager::dataStoreCreationFnc(const std::string& type) const
{
  return m_DataStoreCreationMap.at(type);
}

void IDataIOManager::addDataStoreCreationFnc(const std::string& type, DataStoreCreateFnc creationFnc)
{
  m_DataStoreCreationMap[type] = creationFnc;
}
} // namespace nx::core
