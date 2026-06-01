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
 * @brief The Application class serves as the core of the framework. The
 * Application instance provides access to the FilterList, PipelineBuilder,
 * and REST server. The Application handles loading available plugins into the
 * FilterList so that they can be used by the rest of the codebase.
 * When the Application is deleted, plugins are released and memory is cleaned
 * up. Pipelines or DataStructures are not cleaned up unless they are owned by
 * the REST server, but plugin-specific information or calculations will be
 * made unavailable.
 */
class SIMPLNX_EXPORT Application
{
public:
  using name_type_map = std::map<std::string, DataObject::Type>;

  /**
   * @brief Destroys the Application. If the destroyed Application matches the
   * current Application::Instance(), the instance pointer is set to nullptr.
   */
  ~Application();

  /**
   * @brief Returns a pointer to the current Application. This pointer is
   * replaced when a new nx::core::Application is created, replacing the
   * previous value. If the current Application is destroyed, this method will
   * return nullptr until a new Application is created.
   * @return Shared pointer to the current Application instance, or nullptr if none exists
   */
  static std::shared_ptr<Application> Instance();

  /**
   * @brief Gets the current Application instance, or creates a new one if none exists.
   * @return Shared pointer to the Application instance
   */
  static std::shared_ptr<Application> GetOrCreateInstance();

  /**
   * @brief Deletes the current Application instance and sets it to nullptr.
   */
  static void DeleteInstance();

  /**
   * @brief Finds and loads plugins in the target directory.
   *
   * Plugins are found by using the file extension of "".
   * @param pluginDir
   * @return Result<> indicating success or failure. Accumulates errors from individual plugin loads.
   */
  Result<> loadPlugins(const std::filesystem::path& pluginDir, bool verbose = false);

  /**
   * @brief Returns a pointer to the Application's FilterList.
   *
   * This pointer is owned by the Application and will remain valid for as long
   * as the Application exists.
   * @return FilterList*
   */
  FilterList* getFilterList() const;

  /**
   * @brief Convenience method to return the loaded plugins.
   * @return Unordered set of pointers to all loaded plugins
   */
  std::unordered_set<AbstractPlugin*> getPluginList() const;

  /**
   * @brief Returns the loaded plugin with the given uuid.
   * Returns nullptr if no match.
   * @param uuid The unique identifier of the plugin to retrieve
   * @return Pointer to the plugin if found, nullptr otherwise
   */
  const AbstractPlugin* getPlugin(const Uuid& uuid) const;

  /**
   * @brief Returns a pointer to the application preferences.
   * The application should be in charge of saving or loading values.
   * @return Pointer to the Preferences object
   */
  Preferences* getPreferences();

  /**
   * @brief Saves user preferences to the default filepath.
   * This method does not save default values.
   * @return Result<> indicating success or failure
   */
  Result<> savePreferences();

  /**
   * @brief Loads user preferences from the default filepath.
   * @return Result<> indicating success or failure (warnings if file doesn't exist)
   */
  Result<> loadPreferences();

  /**
   * @brief Returns a pointer to the JsonPipelineBuilder. It is the caller's
   * responsibility to delete the pointer when they are done with it.
   * @return JsonPipelineBuilder*
   */
  JsonPipelineBuilder* getPipelineBuilder() const;

  /**
   * @brief Returns the collection of data I/O managers.
   *
   * The returned reference is non-owning; the DataIOCollection is owned by
   * the Application singleton and lives for the entire process lifetime.
   * Callers should not attempt to extend its lifetime.
   *
   * @return Reference to the DataIOCollection owned by the Application.
   */
  DataIOCollection& getIOCollection() const;

  /**
   * @brief Returns the I/O manager for the specified format.
   * @param formatName The name of the data format
   * @return Shared pointer to the IDataIOManager for the specified format
   */
  std::shared_ptr<IDataIOManager> getIOManager(const std::string& formatName) const;

  /**
   * @brief Returns the I/O manager for the specified format, cast to the specified type.
   * @tparam T The type to cast the I/O manager to
   * @param formatName The name of the data format
   * @return Shared pointer to the I/O manager cast to type T, or nullptr if cast fails
   */
  template <typename T>
  std::shared_ptr<T> getIOManagerAs(const std::string& formatName) const
  {
    return std::dynamic_pointer_cast<T>(getIOManager(formatName));
  }

  /**
   * @brief Returns a filepath pointing to the current executable.
   * @return std::filesystem::path
   */
  std::filesystem::path getCurrentPath() const;

  /**
   * @brief Returns a filepath pointing to the current executable's parent directory.
   * @return std::filesystem::path
   */
  std::filesystem::path getCurrentDir() const;

  /**
   * @brief Returns the Simplnx filter UUID [v4] from the SIMPL filter UUID [v5].
   * @param simplUuid The SIMPL filter UUID to convert
   * @return Optional Simplnx UUID if a mapping exists, std::nullopt otherwise
   */
  std::optional<Uuid> getSimplnxUuid(const Uuid& simplUuid);

  /**
   * @brief Returns the SIMPL filter UUID(s) [v5] from the Simplnx filter UUID [v4].
   * @param simplnxUuid The Simplnx filter UUID to convert
   * @return Vector of SIMPL UUIDs that map to the given Simplnx UUID (may be empty)
   */
  std::vector<Uuid> getSimplUuid(const Uuid& simplnxUuid);

  /**
   * @brief Registers a data type with a name for lookup.
   * @param type The DataObject type enumeration value
   * @param name The string name to associate with the type
   */
  void addDataType(DataObject::Type type, const std::string& name);

  /**
   * @brief Retrieves the DataObject type associated with a given name.
   * @param name The string name of the data type
   * @return The DataObject type enumeration value
   */
  DataObject::Type getDataType(const std::string& name) const;

  /**
   * @brief Returns a list of all available data store format names.
   * @return Vector of strings representing the available data store formats
   */
  std::vector<std::string> getDataStoreFormats() const;

  /**
   * @brief Returns all known format display names as (formatName, displayName) pairs.
   *
   * Delegates to DataIOCollection::getFormatDisplayNames(). The list always
   * includes ("", "Automatic") and (k_InMemoryFormat, "In Memory"). In an
   * OOC-enabled build the compiled-in HDF5-OOC entry is also present (seeded
   * when the DataIOCollection is constructed). Any entries registered by
   * loaded plugins are appended as well.
   *
   * @return Vector of (formatName, displayName) pairs
   */
  std::vector<std::pair<std::string, std::string>> getDataStoreFormatDisplayNames() const;

protected:
  /**
   * @brief Constructs an Application using default values and replaces the
   * current Instance pointer.
   */
  Application();

  /**
   * @brief Constructs an Application accepting a set of command line arguments.
   *
   * The current Application instance is replaced with the constructed Application.
   * @param argc Number of command line arguments
   * @param argv Array of command line argument strings
   */
  Application(int argc, char** argv);

  /**
   * @brief Initializes the application components and sets up default configurations.
   * @return Result indicating success or failure of initialization
   */
  Result<> initialize();

private:
  /**
   * @brief Initializes the default data type mappings.
   */
  void initDefaultDataTypes();

  /**
   * @brief Loads the plugin at the specified filepath and updates the
   * FilterList with the new IFilters.
   * @param path Filesystem path to the plugin library
   * @param verbose If true, outputs verbose loading information
   * @return Result indicating success or failure of plugin loading
   */
  Result<> loadPlugin(const std::filesystem::path& path, bool verbose = false);

  //////////////////
  // Static Variable
  static std::shared_ptr<Application> s_Instance;

  ////////////
  // Variables
  std::unique_ptr<nx::core::FilterList> m_FilterList;
  std::filesystem::path m_CurrentPath = "";
  std::vector<Uuid> m_Simpl_Uuids;   // no duplicates; index must match m_Simplnx_Uuids
  std::vector<Uuid> m_Simplnx_Uuids; // duplicate allowed conditionally; index must match m_Simpl_Uuids
  std::shared_ptr<DataIOCollection> m_DataIOCollection;
  name_type_map m_NamedTypesMap;
  std::unique_ptr<Preferences> m_Preferences = nullptr;
};
} // namespace nx::core
