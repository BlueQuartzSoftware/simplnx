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
 * @enum DataStorageMode
 * @brief Selects storage intent for new data arrays.
 *
 * The core library stores intent instead of a concrete out-of-core format. An
 * OOC-enabled build resolves compatible modes to its registered storage format.
 * The persisted integer values must remain stable.
 */
enum class DataStorageMode : int
{
  Adaptive,      ///< Selects storage from the array-size threshold.
  ForceInCore,   ///< Requests in-core storage regardless of array size.
  ForceOutOfCore ///< Requests out-of-core storage regardless of array size.
};

/**
 * @class Preferences
 * @brief Stores application and plugin preference values.
 *
 * The class merges explicit values with defaults and persists explicit values.
 * It represents storage intent without requiring an out-of-core implementation.
 */
class SIMPLNX_EXPORT Preferences
{
  friend class AbstractPlugin;

public:
  static inline constexpr int32 k_FailedToCreateDirectory_Code = -585;
  static inline constexpr int32 k_FileDoesNotExist_Code = -586;
  static inline constexpr int32 k_FileCouldNotOpen_Code = -587;
  static inline constexpr int32 k_JsonParseError_Code = -588;

  static inline constexpr StringLiteral k_LargeDataSize_Key = "large_data_size";

  // Migration reads this legacy key. The core does not write it.
  static inline constexpr StringLiteral k_PreferredLargeDataFormat_Key = "large_data_format";

  // Migration maps this legacy format value to ForceInCore.
  static inline constexpr StringLiteral k_InMemoryFormat = "Simplnx-Default-In-Memory";

  static inline constexpr StringLiteral k_LargeDataStructureSize_Key = "large_datastructure_size";

  // The preferences file stores Large Data Size under this key, in GB with two decimals.
  // Memory keeps the value in bytes under k_LargeDataSize_Key.
  static constexpr StringLiteral k_LargeDataSizeGB_Key = "large_data_size_gb";

  // The preferences file stores Large Data Structure Size under this key, in GB with two decimals.
  // Memory keeps the value in bytes under k_LargeDataStructureSize_Key.
  static constexpr StringLiteral k_LargeDataStructureSizeGB_Key = "large_datastructure_size_gb";

  // One GB of an out-of-core size threshold is 1024^3 bytes.
  static constexpr uint64 k_BytesPerGiB = 1073741824;

  // The largest threshold in hundredths of a GB (17179869183.99 GB) whose byte count fits in uint64.
  static constexpr uint64 k_MaxOocSizeHundredths = 1717986918399;

  // Migration reads this legacy key. The core does not write it.
  static inline constexpr StringLiteral k_ForceOocData_Key = "force_ooc_data";

  // The persisted value is the DataStorageMode integer.
  static inline constexpr StringLiteral k_DataStorageMode_Key = "data_storage_mode";

  static inline constexpr nx::core::StringLiteral k_OoCTempDirectory_ID = "ooc_temp_directory";

  static inline constexpr StringLiteral k_AutoRangeComputation_Key = "auto_range_computation";

  static inline constexpr StringLiteral k_CacheMemoryBudgetBytes_Key = "cache_memory_budget_bytes";

  // Migration reads this legacy key. The core does not write it.
  static inline constexpr StringLiteral k_LegacyMemoryBudgetBytes_Key = "memory_budget_bytes";

  /**
   * @brief Returns the default per-user preferences path.
   * @param applicationName Application name used in the path.
   * @return Platform-specific preferences.json path.
   *
   * macOS uses Library/Preferences, Windows uses AppData/Local, and other
   * platforms use .config below the home directory.
   */
  static std::filesystem::path DefaultFilePath(const std::string& applicationName);

  /**
   * @brief Validates an out-of-core size threshold without changing preferences.
   * @param name Large Data Size or Large Data Structure Size preference key.
   * @param value Input JSON.
   *
   * @return Exact nonnegative integer in bytes, or a contextual error.
   */
  static Result<uint64> ValidateOocSizeValue(const std::string& name, const nlohmann::json& value);

  /**
   * @brief Converts an out-of-core size threshold from hundredths of a GB to bytes.
   * @param hundredths Threshold in hundredths of a GB (1 GB = 1024^3 bytes).
   * @return Nearest byte count, with a half byte rounded up.
   *
   * The preferences file and the Preferences dialog store a threshold as a multiple of 0.01 GB.
   * File load, file save and the dialog use this function and OocSizeBytesToHundredths() so that
   * they apply one rounding rule. A value above k_MaxOocSizeHundredths converts as k_MaxOocSizeHundredths.
   * The calculation does not overflow.
   */
  static constexpr uint64 OocSizeHundredthsToBytes(uint64 hundredths) noexcept
  {
    const uint64 bounded = hundredths < k_MaxOocSizeHundredths ? hundredths : k_MaxOocSizeHundredths;
    return (bounded / 100) * k_BytesPerGiB + ((bounded % 100) * k_BytesPerGiB + 50) / 100;
  }

  /**
   * @brief Converts an out-of-core size threshold from bytes to hundredths of a GB.
   * @param bytes Threshold in bytes.
   * @return Nearest number of hundredths of a GB (1 GB = 1024^3 bytes), with a half hundredth rounded up.
   *
   * The result is at most k_MaxOocSizeHundredths. A nonzero byte count that rounds to zero
   * becomes 1 (0.01 GB), so a small positive threshold never becomes zero. For every value h
   * from 0 to k_MaxOocSizeHundredths, OocSizeBytesToHundredths(OocSizeHundredthsToBytes(h)) is h.
   * The calculation does not overflow.
   */
  static constexpr uint64 OocSizeBytesToHundredths(uint64 bytes) noexcept
  {
    const uint64 wholeGiB = bytes / k_BytesPerGiB;
    const uint64 remainderBytes = bytes % k_BytesPerGiB;
    uint64 hundredths = wholeGiB * 100 + (remainderBytes * 100 + k_BytesPerGiB / 2) / k_BytesPerGiB;
    if(hundredths > k_MaxOocSizeHundredths)
    {
      hundredths = k_MaxOocSizeHundredths;
    }
    if(bytes > 0 && hundredths == 0)
    {
      hundredths = 1;
    }
    return hundredths;
  }

  Preferences();

  ~Preferences() noexcept;

  /**
   * @brief Tests whether an explicit top-level preference exists.
   * @param name Preference key to test.
   * @return True when m_Values contains name.
   *
   * Default values do not make this method return true.
   */
  bool contains(const std::string& name) const;

  void removeValue(std::string_view name);

  bool pluginContains(const std::string& pluginName, const std::string& name) const;

  bool pluginContainsDefault(const std::string& pluginName, const std::string& name) const;

  /**
   * @brief Returns an explicit preference or its default value.
   * @param name Preference key to read.
   * @return Explicit value, default value, or empty JSON when no value exists.
   */
  nlohmann::json value(const std::string& name) const;

  template <typename T>
  T valueAs(const std::string& name) const
  {
    return value(name).get<T>();
  }

  nlohmann::json defaultValue(const std::string& name) const;

  template <typename T>
  T defaultValueAs(const std::string& name) const
  {
    return defaultValue(name).get<T>();
  }

  /**
   * @brief Stores an explicit top-level preference value.
   * @param name Preference key to update.
   * @param value New JSON value.
   *
   * Both out-of-core size thresholds accept unsigned JSON integers and
   * nonnegative signed JSON integers. Changing Large Data Size recomputes
   * memory defaults.
   * Invalid thresholds throw
   * std::invalid_argument before mutation.
   */
  void setValue(const std::string& name, const nlohmann::json& value);

  /**
   * @brief Returns an explicit plugin preference or its default value.
   * @param pluginName Plugin preference group.
   * @param valueName Preference key within pluginName.
   * @return Explicit value, default value, or empty JSON when no value exists.
   */
  nlohmann::json pluginValue(const std::string& pluginName, const std::string& valueName) const;

  template <typename T>
  T pluginValueAs(const std::string& pluginName, const std::string& valueName) const
  {
    return pluginValue(pluginName, valueName);
  }

  /**
   * @brief Returns a default value from the plugin-default object.
   * @param pluginName Ignored by the current implementation.
   * @param name Key to read directly from the plugin-default object.
   * @return Matching JSON value, or empty JSON when name is absent.
   *
   * The implementation does not index the default object by pluginName.
   */
  nlohmann::json defaultPluginValue(const std::string& pluginName, const std::string& name) const;

  template <typename T>
  T defaultPluginValueAs(const std::string& pluginName, const std::string& name) const
  {
    return defaultPluginValue(pluginName, name).get<T>();
  }

  void setPluginValue(const std::string& pluginName, const std::string& valueName, const nlohmann::json& value);

  /**
   * @brief Clears explicit preference values.
   *
   * The method recreates the plugin group and recomputes memory defaults.
   */
  void clear();

  /**
   * @brief Writes explicit preferences to a JSON file.
   * @param filepath Destination file path.
   * @return Error when the parent directory or output file cannot open.
   *
   * The method creates missing parent directories. Default values are not written.
   * The file stores each out-of-core size threshold in GB under k_LargeDataSizeGB_Key or
   * k_LargeDataStructureSizeGB_Key, as OocSizeBytesToHundredths(bytes) / 100. The byte keys are
   * not written. All other explicit values are written unchanged.
   */
  Result<> saveToFile(const std::filesystem::path& filepath) const;

  /**
   * @brief Reads explicit preferences from a JSON file.
   * @param filepath Source file path.
   * @return Error when the file is absent, cannot open, contains invalid JSON, or has a non-object root.
   * A valid result has one warning for each invalid out-of-core size threshold in the file.
   * That threshold then uses its default.
   *
   * For each out-of-core size threshold, a GB key takes precedence over the byte key that older
   * versions write. The method does not validate that byte key. A GB value must be a finite
   * JSON number from 0 to 17179869183.99. A legacy byte value must pass ValidateOocSizeValue().
   * The method stores a valid value in bytes on the 0.01-GB grid. A value above 0 that rounds
   * to 0 becomes 0.01 GB. The method then migrates legacy cache and storage keys and updates
   * memory defaults. The method does not write the file; the next saveToFile() call writes the GB keys.
   */
  Result<> loadFromFile(const std::filesystem::path& filepath);

  /**
   * @brief Reports whether storage intent permits out-of-core storage.
   *
   * Adaptive and ForceOutOfCore return true. This preference does not prove
   * that an out-of-core manager is registered.
   * @return True unless dataStorageMode() is DataStorageMode::ForceInCore.
   */
  bool useOocData() const;

  /**
   * @brief Returns the canonical tri-state storage preference.
   *
   * An explicit canonical value takes precedence. Legacy force-out-of-core and
   * format values preserve the equivalent intent. Unrecognized values and fresh
   * preferences use Adaptive.
   * @return Active storage mode.
   */
  DataStorageMode dataStorageMode() const;

  /**
   * @brief Stores the canonical tri-state storage preference.
   *
   * The method writes only the canonical key. Legacy keys remain unchanged to
   * keep concrete out-of-core formats outside simplnx core.
   * @param mode Storage mode to persist.
   */
  void setDataStorageMode(DataStorageMode mode);

  /**
   * @brief Recomputes the default whole-data-structure size threshold.
   *
   * The threshold reserves two single-array limits for the operating system and
   * application. Low-memory systems use half of physical RAM instead. The method rounds the
   * threshold to the 0.01-GB grid, so a saved and reloaded default keeps its exact byte count.
   */
  void updateMemoryDefaults();

  uint64 largeDataStructureSize() const;

  std::string oocTempDirectory() const;

  /**
   * @brief Stores the out-of-core temporary directory.
   * @param path Directory for session backing files.
   *
   * The method propagates path to registered data-I/O managers through the
   * global Application instance.
   */
  void setOocTempDirectory(const std::string& path);

  bool autoRangeComputation() const;

  void setAutoRangeComputation(bool enabled);

  uint64 cacheMemoryBudgetBytes() const;

  /**
   * @brief Stores the shared cache-memory budget preference.
   * @param bytes Requested cache budget in bytes.
   *
   * This method does not reconfigure CacheMemoryBudgetManager.
   */
  void setCacheMemoryBudgetBytes(uint64 bytes);

protected:
  void setDefaultValues();

  void addDefaultValues(std::string pluginName, std::string valueName, const nlohmann::json& value);

private:
  nlohmann::json m_DefaultValues;
  nlohmann::json m_Values;
};
} // namespace nx::core
