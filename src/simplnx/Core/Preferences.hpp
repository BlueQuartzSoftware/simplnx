#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace nx::core
{
class AbstractPlugin;

/**
 * @class Preferences
 * @brief Manages application and plugin-specific preferences with support for default values and file persistence.
 * Handles both global application preferences and plugin-specific settings, including out-of-core data management.
 */
class SIMPLNX_EXPORT Preferences
{
  friend class AbstractPlugin;

public:
  /// @name Preference Keys
  /// JSON keys used to store and retrieve preference values. These keys appear
  /// in the serialized preferences.json file and are used internally by the
  /// getter/setter methods below.
  /// @{

  /// Byte-size threshold above which a single DataArray is considered "large"
  /// and may be written to an OOC-capable format instead of in-memory storage.
  static inline constexpr StringLiteral k_LargeDataSize_Key = "large_data_size";

  /// Name of the preferred storage format for large DataArrays (e.g., "HDF5-OOC").
  /// An empty string means "not yet configured by the user or the compiled-in default".
  static inline constexpr StringLiteral k_PreferredLargeDataFormat_Key = "large_data_format";

  /// Sentinel value for k_PreferredLargeDataFormat_Key that explicitly requests
  /// in-memory storage. This is distinct from an empty string, which means
  /// "not configured" and falls back to the compiled-in default.
  static inline constexpr StringLiteral k_InMemoryFormat = "Simplnx-Default-In-Memory";

  /// Byte-size threshold for the entire DataStructure. When total memory usage
  /// approaches this value, the application may switch to OOC storage for new arrays.
  /// The default is computed dynamically by updateMemoryDefaults() based on system RAM.
  static inline constexpr StringLiteral k_LargeDataStructureSize_Key = "large_datastructure_size";

  /// Boolean flag that, when true, forces all new DataArrays to use OOC storage
  /// regardless of their size. Only takes effect when an OOC format is active.
  static inline constexpr StringLiteral k_ForceOocData_Key = "force_ooc_data";

  /// Filesystem path to the directory where OOC temporary files (chunk stores,
  /// backing HDF5 files) are created during filter execution.
  static inline constexpr nx::core::StringLiteral k_OoCTempDirectory_ID = "ooc_temp_directory";

  /// Boolean flag that controls whether the DREAM3D-NX visualization layer
  /// computes exact min/max for every visible array on file open. Applies to
  /// both in-memory and out-of-core arrays. Default: false.
  static inline constexpr StringLiteral k_AutoRangeComputation_Key = "auto_range_computation";

  /// Total memory budget in bytes shared across all caching subsystems
  /// (chunk cache, stride cache, partition cache). The memory budget manager
  /// distributes this budget via global LRU eviction. When unset, defaults to
  /// MemoryBudgetManager::defaultBudgetBytes() (50% of system RAM, clamped to a
  /// minimum of 1 GB).
  static inline constexpr StringLiteral k_MemoryBudgetBytes_Key = "memory_budget_bytes";

  /// @}

  /**
   * @brief Returns the default file path for storing preferences based on the application name.
   * @param applicationName The name of the application
   * @return Default filesystem path for the preferences file
   */
  static std::filesystem::path DefaultFilePath(const std::string& applicationName);

  /**
   * @brief Default constructor initializes preferences with default values.
   */
  Preferences();

  /**
   * @brief Destructor cleans up preferences resources.
   */
  ~Preferences() noexcept;

  /**
   * @brief Checks if a preference with the given name exists.
   * @param name The name of the preference to check
   * @return True if the preference exists, false otherwise
   */
  bool contains(const std::string& name) const;

  /**
   * @brief Removes a top-level preference by key. No-op if the key is absent.
   * @param name The name of the preference to remove
   */
  void removeValue(std::string_view name);

  /**
   * @brief Checks if a plugin-specific preference exists.
   * @param pluginName The name of the plugin
   * @param name The name of the preference within the plugin
   * @return True if the plugin preference exists, false otherwise
   */
  bool pluginContains(const std::string& pluginName, const std::string& name) const;

  /**
   * @brief Checks if a default value exists for a plugin-specific preference.
   * @param pluginName The name of the plugin
   * @param name The name of the preference within the plugin
   * @return True if a default value exists for the plugin preference, false otherwise
   */
  bool pluginContainsDefault(const std::string& pluginName, const std::string& name) const;

  /**
   * @brief Retrieves the value of a preference as a JSON object.
   * @param name The name of the preference
   * @return The preference value as a JSON object
   */
  nlohmann::json value(const std::string& name) const;

  /**
   * @brief Retrieves the value of a preference and converts it to the specified type.
   * @tparam T The type to convert the preference value to
   * @param name The name of the preference
   * @return The preference value converted to type T
   */
  template <typename T>
  T valueAs(const std::string& name) const
  {
    return value(name).get<T>();
  }

  /**
   * @brief Retrieves the default value of a preference as a JSON object.
   * @param name The name of the preference
   * @return The default preference value as a JSON object
   */
  nlohmann::json defaultValue(const std::string& name) const;

  /**
   * @brief Retrieves the default value of a preference and converts it to the specified type.
   * @tparam T The type to convert the default value to
   * @param name The name of the preference
   * @return The default preference value converted to type T
   */
  template <typename T>
  T defaultValueAs(const std::string& name) const
  {
    return defaultValue(name).get<T>();
  }

  /**
   * @brief Sets the value of a preference.
   * @param name The name of the preference
   * @param value The new value as a JSON object
   */
  void setValue(const std::string& name, const nlohmann::json& value);

  /**
   * @brief Retrieves a plugin-specific preference value as a JSON object.
   * @param pluginName The name of the plugin
   * @param valueName The name of the preference within the plugin
   * @return The plugin preference value as a JSON object
   */
  nlohmann::json pluginValue(const std::string& pluginName, const std::string& valueName) const;

  /**
   * @brief Retrieves a plugin-specific preference value and converts it to the specified type.
   * @tparam T The type to convert the preference value to
   * @param pluginName The name of the plugin
   * @param valueName The name of the preference within the plugin
   * @return The plugin preference value converted to type T
   */
  template <typename T>
  T pluginValueAs(const std::string& pluginName, const std::string& valueName) const
  {
    return pluginValue(pluginName, valueName);
  }

  /**
   * @brief Retrieves the default value of a plugin-specific preference as a JSON object.
   * @param pluginName The name of the plugin
   * @param name The name of the preference within the plugin
   * @return The default plugin preference value as a JSON object
   */
  nlohmann::json defaultPluginValue(const std::string& pluginName, const std::string& name) const;

  /**
   * @brief Retrieves the default value of a plugin-specific preference and converts it to the specified type.
   * @tparam T The type to convert the default value to
   * @param pluginName The name of the plugin
   * @param name The name of the preference within the plugin
   * @return The default plugin preference value converted to type T
   */
  template <typename T>
  T defaultPluginValueAs(const std::string& pluginName, const std::string& name) const
  {
    return defaultPluginValue(pluginName, name).get<T>();
  }

  /**
   * @brief Sets a plugin-specific preference value.
   * @param pluginName The name of the plugin
   * @param valueName The name of the preference within the plugin
   * @param value The new value as a JSON object
   */
  void setPluginValue(const std::string& pluginName, const std::string& valueName, const nlohmann::json& value);

  /**
   * @brief Clears all preference values (does not affect default values).
   */
  void clear();

  /**
   * @brief Saves the current preferences to a file.
   * @param filepath The filesystem path where preferences will be saved
   * @return Result indicating success or failure of the save operation
   */
  Result<> saveToFile(const std::filesystem::path& filepath) const;

  /**
   * @brief Loads preferences from a file.
   * @param filepath The filesystem path from which preferences will be loaded
   * @return Result indicating success or failure of the load operation
   */
  Result<> loadFromFile(const std::filesystem::path& filepath);

  /**
   * @brief Gets the default format for large data storage.
   * @return String representing the default large data format
   */
  std::string defaultLargeDataFormat() const;

  /**
   * @brief Sets the default format for large data storage.
   * @param dataFormat The format to use as default for large data
   */
  void setDefaultLargeDataFormat(std::string dataFormat);

  /**
   * @brief Gets the current format for large data storage.
   * @return String representing the current large data format
   */
  std::string largeDataFormat() const;

  /**
   * @brief Sets the format for large data storage.
   * @param dataFormat The format to use for large data
   */
  void setLargeDataFormat(std::string dataFormat);

  /**
   * @brief Checks if out-of-core (OOC) data mode is being used.
   * @return True if OOC data mode is enabled, false otherwise
   */
  bool useOocData() const;

  /**
   * @brief Checks if out-of-core (OOC) data mode is forced.
   * @return True if OOC data mode is forced, false otherwise
   */
  bool forceOocData() const;

  /**
   * @brief Sets whether to force out-of-core (OOC) data mode.
   * @param forceOoc True to force OOC mode, false otherwise
   */
  void setForceOocData(bool forceOoc);

  /**
   * @brief Recomputes the default value for k_LargeDataStructureSize_Key based
   *        on the current system's total physical RAM.
   *
   * The target value is (totalRAM - 2 * k_LargeDataSize), which reserves
   * headroom for the OS and the application itself. If the reservation would
   * exceed total RAM (e.g., on a low-memory system), the fallback is totalRAM / 2.
   *
   * Called automatically during construction, after loadFromFile(), and after
   * clear(). Can also be called explicitly after changing k_LargeDataSize_Key.
   */
  void updateMemoryDefaults();

  /**
   * @brief Gets the size threshold for large data structures.
   * @return Size threshold in bytes for considering a data structure as large
   */
  uint64 largeDataStructureSize() const;

  /**
   * @brief Gets the temporary directory path for out-of-core data.
   * @return String representing the OOC temporary directory path
   */
  std::string oocTempDirectory() const;

  /**
   * @brief Sets the temporary directory path for out-of-core data.
   * @param path The directory path to use for OOC temporary files
   */
  void setOocTempDirectory(const std::string& path);

  /**
   * @brief Whether the application should compute exact min/max for every
   *  visible array on file open. Off by default. When on, the DREAM3D-NX
   *  visualization layer dispatches background workers per array. Applies
   *  to both in-memory and out-of-core arrays.
   */
  bool autoRangeComputation() const;

  /**
   * @brief Sets the auto_range_computation preference. The DREAM3D-NX
   *  visualization layer observes the corresponding NXPreferences signal
   *  and kicks off scans for the already-loaded data source when the
   *  pref toggles to true.
   */
  void setAutoRangeComputation(bool enabled);

  /**
   * @brief Gets the total memory budget for all caching subsystems.
   *
   * The memory budget manager distributes this budget across the chunk
   * cache, stride cache, and partition cache using global LRU eviction. When
   * the combined memory usage of all caches exceeds this budget, the least
   * recently used entries are evicted to make room for new data.
   *
   * When the user has not saved an explicit budget preference, this falls
   * back to MemoryBudgetManager::defaultBudgetBytes() — 50% of system RAM,
   * clamped to a minimum of 1 GB.
   *
   * @note This reads from m_Values (user-set) directly, NOT from m_DefaultValues,
   *       because the default is computed at call time from system RAM.
   *
   * @return Budget in bytes (50% of system RAM if not explicitly set)
   */
  uint64 memoryBudgetBytes() const;

  /**
   * @brief Sets the total memory budget for all caching subsystems.
   *
   * The new budget takes effect immediately for subsequent cache eviction
   * decisions. Existing cached data that exceeds the new budget will be
   * evicted lazily as new cache entries are requested.
   *
   * @param bytes Budget in bytes. Must be > 0; passing 0 would effectively
   *        disable caching.
   */
  void setMemoryBudgetBytes(uint64 bytes);

protected:
  /**
   * @brief Initializes all default preference values for the application.
   */
  void setDefaultValues();

  /**
   * @brief Adds a default value for a plugin-specific preference.
   * @param pluginName The name of the plugin
   * @param valueName The name of the preference within the plugin
   * @param value The default value as a JSON object
   */
  void addDefaultValues(std::string pluginName, std::string valueName, const nlohmann::json& value);

  /**
   * @brief Recomputes the cached m_UseOoc flag based on the current value of
   *        k_PreferredLargeDataFormat_Key.
   *
   * OOC mode is considered active when the resolved format string is non-empty
   * and is not the sentinel value k_InMemoryFormat. This method is called after
   * any operation that could change the format: construction, loadFromFile(),
   * setLargeDataFormat(), and setDefaultLargeDataFormat().
   */
  void checkUseOoc();

private:
  nlohmann::json m_DefaultValues;
  nlohmann::json m_Values;
  bool m_UseOoc = false;
};
} // namespace nx::core
