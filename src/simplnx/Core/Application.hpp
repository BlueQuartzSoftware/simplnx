#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/simplnx_export.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nx::core
{
class DataIOCollection;
class IDataIOManager;
class JsonPipelineBuilder;

/**
 * @class Application
 * @brief Owns the process-wide filter, preference, and data-I/O services.
 *
 * The global instance owns loaded plugins and their registrations. Data
 * structures and pipelines remain caller-owned. The global instance operations
 * are not synchronized.
 */
class SIMPLNX_EXPORT Application
{
public:
  using name_type_map = std::map<std::string, DataObject::Type>;

  /**
   * @brief Saves preferences and clears the global instance pointer.
   *
   * The destructor logs a preference-write error because it cannot return it.
   */
  ~Application();

  /**
   * @brief Returns the global application instance without creating one.
   * @return Shared instance, or nullptr when no instance exists.
   *
   * The global instance API is not thread-safe.
   */
  static std::shared_ptr<Application> Instance();

  /**
   * @brief Returns the global application instance, creating it when needed.
   * @return Shared global instance.
   *
   * The global instance API is not thread-safe.
   */
  static std::shared_ptr<Application> GetOrCreateInstance();

  /**
   * @brief Saves preferences and releases the global application instance.
   *
   * The method logs a preference-write error because it cannot return it.
   */
  static void DeleteInstance();

  /**
   * @brief Finds and loads plugins in the target directory.
   * @param pluginDir Directory to scan for plugin libraries.
   * @param verbose True to write load progress to standard output.
   * @return Errors for an invalid directory or failed plugin loads.
   *
   * The method selects the build-matching .simplnx library suffix. It continues
   * after an individual plugin failure and returns the accumulated errors.
   */
  Result<> loadPlugins(const std::filesystem::path& pluginDir, bool verbose = false);

  /**
   * @brief Returns the owned filter registry.
   * @return FilterList owned by this application.
   *
   * The pointer remains valid until this application is destroyed.
   */
  FilterList* getFilterList() const;

  /**
   * @brief Returns the loaded plugin objects.
   * @return Non-owning pointers to loaded plugins.
   *
   * The pointers remain valid until the application releases the plugins.
   */
  std::unordered_set<AbstractPlugin*> getPluginList() const;

  /**
   * @brief Finds a loaded plugin by UUID.
   * @param uuid Plugin UUID to find.
   * @return Non-owning plugin pointer, or nullptr when no plugin matches.
   */
  const AbstractPlugin* getPlugin(const Uuid& uuid) const;

  /**
   * @brief Returns the owned preference store.
   * @return Preferences owned by this application.
   *
   * The pointer remains valid until this application is destroyed.
   */
  Preferences* getPreferences();

  /**
   * @brief Saves explicit preferences to the default file path.
   * @return Error when preferences are unavailable or cannot be saved.
   *
   * Default preference values are not serialized.
   */
  Result<> savePreferences();

  /**
   * @brief Loads preferences from the default file path.
   * @return Error when the file cannot load or parse.
   */
  Result<> loadPreferences();

  /**
   * @brief Returns no JSON pipeline builder.
   * @return nullptr.
   */
  JsonPipelineBuilder* getPipelineBuilder() const;

  /**
   * @brief Returns the collection of data I/O managers.
   *
   * @return DataIOCollection owned by this application.
   *
   * The reference remains valid until this application is destroyed.
   */
  DataIOCollection& getIOCollection() const;

  /**
   * @brief Finds a data-I/O manager by format name.
   * @param formatName Registered data format name.
   * @return Shared manager, or nullptr when no manager matches.
   */
  std::shared_ptr<IDataIOManager> getIOManager(const std::string& formatName) const;

  template <typename T>
  std::shared_ptr<T> getIOManagerAs(const std::string& formatName) const
  {
    return std::dynamic_pointer_cast<T>(getIOManager(formatName));
  }

  std::filesystem::path getCurrentPath() const;

  std::filesystem::path getCurrentDir() const;

  /**
   * @brief Maps a legacy SIMPL filter UUID to a simplnx filter UUID.
   * @param simplUuid Legacy filter UUID.
   * @return Mapped simplnx UUID, or std::nullopt when no mapping exists.
   */
  std::optional<Uuid> getSimplnxUuid(const Uuid& simplUuid);

  /**
   * @brief Maps a simplnx filter UUID to legacy SIMPL filter UUIDs.
   * @param simplnxUuid Simplnx filter UUID.
   * @return Legacy UUIDs that map to simplnxUuid.
   */
  std::vector<Uuid> getSimplUuid(const Uuid& simplnxUuid);

  void addDataType(DataObject::Type type, const std::string& name);

  DataObject::Type getDataType(const std::string& name) const;

  std::vector<std::string> getDataStoreFormats() const;

  /**
   * @brief Returns registered data-store format display names.
   *
   * The collection includes Automatic and In Memory entries. Registered I/O
   * managers append their display names.
   * @return Format-name and display-name pairs.
   */
  std::vector<std::pair<std::string, std::string>> getDataStoreFormatDisplayNames() const;

protected:
  Application();

  /**
   * @brief Constructs an application for a legacy command-line interface.
   * @param argc Ignored command-line argument count.
   * @param argv Ignored command-line arguments.
   */
  Application(int argc, char** argv);

  /**
   * @brief Initializes preferences, executable path, and type mappings.
   * @return Preference-load warnings or an executable-path error.
   */
  Result<> initialize();

private:
  void initDefaultDataTypes();

  /**
   * @brief Loads one plugin and registers its filter and I/O services.
   * @param path Plugin library path.
   * @param verbose True to write load progress to standard output.
   * @return Error when loading, UUID mapping, or I/O registration fails.
   *
   * Legacy UUIDs remain unique so reverse mapping stays unambiguous.
   */
  Result<> loadPlugin(const std::filesystem::path& path, bool verbose = false);

  static std::shared_ptr<Application> s_Instance;

  std::unique_ptr<nx::core::FilterList> m_FilterList;
  std::filesystem::path m_CurrentPath = "";
  std::vector<Uuid> m_Simpl_Uuids;   // Legacy UUIDs are unique. Indices match m_Simplnx_Uuids.
  std::vector<Uuid> m_Simplnx_Uuids; // Values can repeat. Indices match m_Simpl_Uuids.
  std::shared_ptr<DataIOCollection> m_DataIOCollection;
  name_type_map m_NamedTypesMap;
  std::unique_ptr<Preferences> m_Preferences = nullptr;
};
} // namespace nx::core
