#include "DataIOCollection.hpp"

#include "complex/DataStructure/IO/HDF5/DataIOManager.hpp"

namespace complex
{
DataIOCollection::DataIOCollection()
{
  addIOManager(std::make_shared<HDF5::DataIOManager>());
}
DataIOCollection::~DataIOCollection() noexcept = default;

void DataIOCollection::addIOManager(std::shared_ptr<IDataIOManager> manager)
{
  if(manager == nullptr)
  {
    return;
  }

  m_ManagerMap[manager->formatName()] = manager;
}

std::shared_ptr<IDataIOManager> DataIOCollection::getManager(const std::string& formatName) const
{
  if(m_ManagerMap.find(formatName) == m_ManagerMap.end())
  {
    return nullptr;
  }

  return m_ManagerMap.at(formatName);
}

DataIOCollection::iterator DataIOCollection::begin()
{
  return m_ManagerMap.begin();
}
DataIOCollection::iterator DataIOCollection::end()
{
  return m_ManagerMap.end();
}

DataIOCollection::const_iterator DataIOCollection::begin() const
{
  return m_ManagerMap.begin();
}
DataIOCollection::const_iterator DataIOCollection::end() const
{
  return m_ManagerMap.end();
}
} // namespace complex
