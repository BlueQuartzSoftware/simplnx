#include "DataIOCollection.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IO/Generic/CoreDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"

namespace nx::core
{
DataIOCollection::DataIOCollection()
{
  addIOManager(std::make_shared<nx::core::Generic::CoreDataIOManager>());
  addIOManager(std::make_shared<nx::core::HDF5::DataIOManager>());
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

bool DataIOCollection::hasDataStoreCreationFunction(const std::string& type) const
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasDataStoreCreationFnc(type))
    {
      return true;
    }
  }
  return false;
}
std::unique_ptr<IDataStore> DataIOCollection::createDataStore(const std::string& type, DataType dataType, const typename IDataStore::ShapeType& tupleShape,
                                                              const typename IDataStore::ShapeType& componentShape)
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasDataStoreCreationFnc(type))
    {
      return ioManager->dataStoreCreationFnc(type)(dataType, tupleShape, componentShape, {});
    }
  }

  return nullptr;
}

std::string DataIOCollection::checkStoreDataFormat(uint64 dataSize, std::string dataFormat) const
{
  if(!dataFormat.empty())
  {
    return dataFormat;
  }
  const Preferences* preferences = Application::GetOrCreateInstance()->getPreferences();
  const uint64 largeDataSize = preferences->valueAs<uint64>(Preferences::k_LargeDataSize_Key);
  const std::string largeDataFormat = preferences->valueAs<std::string>(Preferences::k_PreferredLargeDataFormat_Key);
  if(dataSize > largeDataSize && hasDataStoreCreationFunction(largeDataFormat))
  {
    dataFormat = largeDataFormat;
  }
  return dataFormat;
}

std::vector<std::string> DataIOCollection::getFormatNames() const
{
  std::vector<std::string> keyNames;
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasDataStoreCreationFnc(ioType))
    {
      keyNames.push_back(ioType);
    }
  }

  return keyNames;
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
} // namespace nx::core
