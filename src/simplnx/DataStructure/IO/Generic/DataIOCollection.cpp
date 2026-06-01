#include "DataIOCollection.hpp"

#include "simplnx/Common/SimplnxConfig.hpp"
#ifdef SIMPLNX_USE_OOC
#include "SimplnxOoc/OocDataIOManager.hpp"
#endif

#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractStringStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/CoreDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"

namespace nx::core
{
DataIOCollection::DataIOCollection()
{
  // Register the built-in CoreDataIOManager directly into the map. The core
  // manager's formatName() is k_InMemoryFormat, which is a reserved name that
  // addIOManager() rejects for plugin registrations — so we bypass that
  // validation here by writing to m_ManagerMap directly. This is the only
  // place in the codebase permitted to register a manager under the reserved
  // in-memory format name.
  auto coreManager = std::make_shared<nx::core::Generic::CoreDataIOManager>();
  m_ManagerMap[coreManager->formatName()] = coreManager;

  // HDF5 format name is not reserved, so this cannot fail.
  (void)addIOManager(std::make_shared<nx::core::HDF5::DataIOManager>());

#ifdef SIMPLNX_USE_OOC
  // Register the out-of-core format ("HDF5-OOC"). Compile-time analog of the
  // former SimplnxOoc plugin's initialize(), which added the OocDataIOManager
  // via addIOManager when the plugin loaded. This makes getManager("HDF5-OOC")
  // resolve so the format is recognized as available (e.g., the unit-test
  // ExpectedStoreType() check); OOC stores themselves are created through the
  // StoreFactory entry points, not this manager.
  SimplnxOoc::registerIOManager(*this);
#endif
}
DataIOCollection::~DataIOCollection() noexcept = default;

Result<> DataIOCollection::addIOManager(std::shared_ptr<IDataIOManager> manager)
{
  if(manager == nullptr)
  {
    return MakeErrorResult(-6010, "Cannot register a null IDataIOManager");
  }

  const std::string& name = manager->formatName();
  // k_InMemoryFormat is reserved for the built-in CoreDataIOManager, which is
  // registered directly by the constructor. Any other attempt to register
  // under this name (e.g., from a plugin) is rejected.
  if(name == Preferences::k_InMemoryFormat)
  {
    return MakeErrorResult(-6011, fmt::format("Cannot register an I/O manager with the reserved format name '{}'", std::string(Preferences::k_InMemoryFormat)));
  }

  m_ManagerMap[name] = manager;
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

  // Fallback: no registered manager claimed @p type, so default to in-memory
  // storage. The built-in CoreDataIOManager is always registered under
  // k_InMemoryFormat by our constructor, so .at() is guaranteed to succeed.
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

  // Fallback: see createDataStore for rationale. Core manager is always present.
  const auto& coreManager = m_ManagerMap.at(std::string(Preferences::k_InMemoryFormat));
  return coreManager->listStoreCreationFnc(coreManager->formatName())(dataType, tupleShape);
}

// ---------------------------------------------------------------------------
// StringStore creation
// ---------------------------------------------------------------------------

bool DataIOCollection::hasStringStoreCreationFnc(const std::string& type) const
{
  // Search all registered IO managers for one that provides a StringStore
  // factory for the requested format name.
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
  // Find the first IO manager that has a StringStore factory for the requested
  // format and delegate construction.
  for(const auto& [ioType, ioManager] : m_ManagerMap)
  {
    if(ioManager->hasStringStoreCreationFnc(type))
    {
      auto fnc = ioManager->stringStoreCreationFnc(type);
      return fnc(tupleShape);
    }
  }
  // No IO manager supports this format for string stores.
  return nullptr;
}

// ---------------------------------------------------------------------------
// Post-pipeline store finalization
// ---------------------------------------------------------------------------

void DataIOCollection::finalizeStores(DataStructure& dataStructure)
{
#ifdef SIMPLNX_USE_OOC
  // Transition OOC stores from write mode to read-only after pipeline execution
  // (close HDF5 write handles, reopen as read handles). No-op for in-core stores.
  SimplnxOoc::finalizeStores(dataStructure);
#endif
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
  // Always include the two built-in entries first
  result.emplace_back("", "Automatic");
  result.emplace_back(std::string(Preferences::k_InMemoryFormat), "In Memory");
  // Append any additionally registered display names
  for(const auto& [formatName, displayName] : m_FormatDisplayNames)
  {
    result.emplace_back(formatName, displayName);
  }
  return result;
}

std::string DataIOCollection::generateManagerListString() const
{
  // Build one row per registered manager. Each row pairs the display name
  // with a comma-separated list of store-type capabilities. We collect first,
  // then format the output as a padded table so columns align.
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
    // The manager's own formatName() is the key under which it registers its
    // factories (convention enforced across all known managers). Query with
    // that key to determine what this manager can create.
    const std::string fn = manager->formatName();

    // Resolve a friendly display name. The core in-memory manager has a fixed
    // label; other managers may have a registered display name;
    // otherwise fall back to the raw format identifier.
    std::string displayName;
    if(fn == Preferences::k_InMemoryFormat)
    {
      displayName = "In Memory";
    }
    else
    {
      auto it = m_FormatDisplayNames.find(fn);
      displayName = (it != m_FormatDisplayNames.end()) ? it->second : fn;
    }

    // Collect the capability labels in a stable order so rows read consistently.
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

  // Assemble the padded output. The leading newline keeps the table from
  // butting up against the caller's error-message prefix.
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
