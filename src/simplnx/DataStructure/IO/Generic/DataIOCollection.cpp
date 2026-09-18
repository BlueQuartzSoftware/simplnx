#include "DataIOCollection.hpp"

#include <cassert>

#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractStringStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/CoreDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"

#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"
#include <algorithm>

namespace nx::core
{
DataIOCollection::DataIOCollection()
{
  // Only the built-in CoreDataIOManager can use the reserved in-memory format name.
  auto coreManager = std::make_shared<nx::core::Generic::CoreDataIOManager>();
  m_ManagerMap[coreManager->formatName()] = coreManager;

  (void)addIOManager(std::make_shared<nx::core::HDF5::DataIOManager>());

  // Optional managers register through plugins so core remains storage-format neutral.
}
DataIOCollection::~DataIOCollection() noexcept = default;

Result<> DataIOCollection::addIOManager(std::shared_ptr<IDataIOManager> manager)
{
  if(manager == nullptr)
  {
    return MakeErrorResult(-6010, "Cannot register a null IDataIOManager");
  }

  const std::string& name = manager->formatName();
  // Plugins cannot replace the built-in manager for the reserved in-memory name.
  if(name == Preferences::k_InMemoryFormat)
  {
    return MakeErrorResult(-6011, fmt::format("Cannot register an I/O manager with the reserved format name '{}'", std::string(Preferences::k_InMemoryFormat)));
  }

  m_ManagerMap[name] = manager;

  // Import finalization chooses one manager, so registration permits at most one finalizer.
#ifndef NDEBUG
  {
    int finalizerCount = 0;
    for(const auto& [ioType, ioManager] : m_ManagerMap)
    {
      if(ioManager->finalizesImport())
      {
        ++finalizerCount;
      }
    }
    assert(finalizerCount <= 1 && "DataIOCollection: more than one IO manager reports finalizesImport()==true");
  }
#endif

  return {};
}

std::shared_ptr<IDataIOManager> DataIOCollection::getManager(const std::string& formatName) const
{
  if(m_ManagerMap.find(formatName) == m_ManagerMap.end())
  {
    return nullptr;
  }

  return m_ManagerMap.at(formatName);
}

bool DataIOCollection::hasExternalSortCapability() const
{
  return std::any_of(m_ManagerMap.cbegin(), m_ManagerMap.cend(), [](const auto& entry) { return entry.second->supportsExternalSort(); });
}

Result<std::unique_ptr<IExternalSort>> DataIOCollection::createExternalSort(const ExternalSortConfig& config) const
{
  for(const auto& [formatName, ioManager] : m_ManagerMap)
  {
    if(ioManager->supportsExternalSort())
    {
      return ioManager->createExternalSort(config);
    }
  }
  return MakeErrorResult<std::unique_ptr<IExternalSort>>(-6013, "No registered I/O manager provides external sorting");
}

bool DataIOCollection::hasTemporaryRecordStoreCapability() const
{
  return std::any_of(m_ManagerMap.cbegin(), m_ManagerMap.cend(), [](const auto& entry) { return entry.second->supportsTemporaryRecordStore(); });
}

Result<std::unique_ptr<ITemporaryRecordStore>> DataIOCollection::createTemporaryRecordStore(const TemporaryRecordStoreConfig& config) const
{
  for(const auto& [formatName, ioManager] : m_ManagerMap)
  {
    if(ioManager->supportsTemporaryRecordStore())
    {
      return ioManager->createTemporaryRecordStore(config);
    }
  }
  return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-6015, "No registered I/O manager provides temporary record storage");
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
std::unique_ptr<IDataStore> DataIOCollection::createDataStore(const std::string& type, DataType dataType, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasDataStoreCreationFnc(type))
    {
      return ioManager->dataStoreCreationFnc(type)(dataType, tupleShape, componentShape, {});
    }
  }

  // Unknown formats use the built-in in-memory store. The constructor always
  // registers its reserved manager before callers can create stores.
  const auto& coreManager = m_ManagerMap.at(std::string(Preferences::k_InMemoryFormat));
  return coreManager->dataStoreCreationFnc(coreManager->formatName())(dataType, tupleShape, componentShape, {});
}

std::unique_ptr<IListStore> DataIOCollection::createListStore(const std::string& type, DataType dataType, const ShapeType& tupleShape) const
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasListStoreCreationFnc(type))
    {
      return ioManager->listStoreCreationFnc(type)(dataType, tupleShape);
    }
  }

  // Unknown formats use the built-in in-memory list store.
  const auto& coreManager = m_ManagerMap.at(std::string(Preferences::k_InMemoryFormat));
  return coreManager->listStoreCreationFnc(coreManager->formatName())(dataType, tupleShape);
}

bool DataIOCollection::hasStringStoreCreationFnc(const std::string& type) const
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasStringStoreCreationFnc(type))
    {
      return true;
    }
  }
  return false;
}

std::unique_ptr<AbstractStringStore> DataIOCollection::createStringStore(const std::string& type, const ShapeType& tupleShape)
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasStringStoreCreationFnc(type))
    {
      auto fnc = ioManager->stringStoreCreationFnc(type);
      return fnc(tupleShape);
    }
  }
  return nullptr;
}

void DataIOCollection::finalizeStores(DataStructure& dataStructure)
{
  // Each manager finalizes only the stores it owns after pipeline execution.
  for(const auto& [ioType, manager] : m_ManagerMap)
  {
    manager->onFinalizeStores(dataStructure);
  }
}

bool DataIOCollection::anyManagerFinalizesImport() const
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->finalizesImport())
    {
      return true;
    }
  }
  return false;
}

std::optional<Result<>> DataIOCollection::onImportFinalize(DataStructure& dataStructure, const std::vector<DataPath>& paths, const nx::core::HDF5::FileIO& fileReader)
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->finalizesImport())
    {
      return ioManager->onImportFinalize(dataStructure, paths, fileReader);
    }
  }
  return std::nullopt;
}

std::optional<Result<>> DataIOCollection::onRecoveryWrite(nx::core::HDF5::DataStructureWriter& writer, const DataObject* dataObject, nx::core::HDF5::GroupIO& parentGroup)
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    auto overrideResult = ioManager->onRecoveryWrite(writer, dataObject, parentGroup);
    if(overrideResult.has_value())
    {
      return overrideResult;
    }
  }
  return std::nullopt;
}

void DataIOCollection::setBaseDirectory(const std::filesystem::path& path)
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    ioManager->setBaseDirectory(path);
  }
}

void DataIOCollection::shutdownManagers()
{
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    ioManager->shutdownManager();
  }
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

void DataIOCollection::registerFormatDisplayName(const std::string& formatName, const std::string& displayName)
{
  m_FormatDisplayNames[formatName] = displayName;
}

std::vector<std::pair<std::string, std::string>> DataIOCollection::getFormatDisplayNames() const
{
  std::vector<std::pair<std::string, std::string>> result;
  // Automatic and explicit in-memory labels remain first for stable UI selection.
  result.emplace_back("", k_AutomaticDisplayName);
  result.emplace_back(std::string(Preferences::k_InMemoryFormat), k_InMemoryDisplayName);
  for(const auto& [formatName, displayName] : m_FormatDisplayNames)
  {
    result.emplace_back(formatName, displayName);
  }
  return result;
}

std::string DataIOCollection::generateManagerListString() const
{
  // Collect rows before formatting so labels align in the error-message table.
  struct Row
  {
    std::string displayName;
    std::string capabilityList;
  };
  std::vector<Row> rows;
  rows.reserve(m_ManagerMap.size());

  usize maxNameWidth = 0;
  for(const auto& [managerKey, manager] : m_ManagerMap)
  {
    // A manager registers its own factories under its format name.
    const std::string fn = manager->formatName();

    // Prefer a registered label and fall back to the format identifier.
    std::string displayName;
    if(fn == Preferences::k_InMemoryFormat)
    {
      displayName = k_InMemoryDisplayName;
    }
    else
    {
      auto it = m_FormatDisplayNames.find(fn);
      displayName = (it != m_FormatDisplayNames.end()) ? it->second : fn;
    }

    // Keep capability labels in a stable order for readable errors.
    std::vector<std::string> capabilities;
    if(manager->hasDataStoreCreationFnc(fn))
    {
      capabilities.emplace_back("DataStore");
    }
    if(manager->hasListStoreCreationFnc(fn))
    {
      capabilities.emplace_back("ListStore");
    }
    if(manager->hasStringStoreCreationFnc(fn))
    {
      capabilities.emplace_back("StringStore");
    }

    std::string capList;
    for(usize i = 0; i < capabilities.size(); ++i)
    {
      if(i > 0)
      {
        capList += ", ";
      }
      capList += capabilities[i];
    }
    if(capList.empty())
    {
      capList = "(no factories registered)";
    }

    if(displayName.size() > maxNameWidth)
    {
      maxNameWidth = displayName.size();
    }
    rows.push_back({std::move(displayName), std::move(capList)});
  }

  // A leading newline separates the table from its caller's error prefix.
  std::string result = "Registered IO managers and their capabilities:";
  for(const auto& row : rows)
  {
    result += fmt::format("\n  {:<{}} : {}", row.displayName, maxNameWidth, row.capabilityList);
  }
  return result;
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
