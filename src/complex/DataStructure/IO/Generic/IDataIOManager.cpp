#include "IDataIOManager.hpp"

namespace complex
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
} // namespace complex
