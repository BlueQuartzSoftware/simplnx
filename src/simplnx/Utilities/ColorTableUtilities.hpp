#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace nx::core::ColorTableUtilities
{

/**
 * @brief This holds a ColorPreset object that can be read/written to JSON
 */
struct SIMPLNX_EXPORT ColorPreset
{
  std::string Name;
  std::string ColorSpace;
  std::string Source;
  std::string License;
  std::string Creator;

  std::vector<float32> NanColor;
  std::vector<float32> RGBPoints;
  std::vector<float32> IndexedColors;

  nlohmann::json Annotations;
  bool DefaultMap = false;

  bool Invalid = true;

  explicit ColorPreset(const nlohmann::json& a);
  ColorPreset();

  nlohmann::json toJson() const;

  std::string toJsonString(int index = -1) const;
};

using ColorPresetsVectorType = std::vector<ColorPreset>;

/**
 * @brief Converts the default JSON color table data into the internal representation as
 * a `ColorPreset`
 * @param rgbPreset Load RGB Presets
 * @param indexedPreset Load Indexed Color Preset
 * @return
 */
SIMPLNX_EXPORT Result<ColorPresetsVectorType> CacheAllPresets(bool rgbPreset, bool indexedPreset);

/**
 * @brief LoadRGBPresets This method will combine RGB json presets from the ColorTable.hpp JSON string
 * @return The json object result
 */
SIMPLNX_EXPORT Result<nlohmann::json> LoadAllRGBPresets();

/**
 * @brief This method will combine Indexed Color json presets from the ColorTable.hpp JSON string
 * @return
 */
SIMPLNX_EXPORT Result<nlohmann::json> LoadAllIndexedPresets();

/**
 * @brief Load all color preset objects into a JSON representation
 * @return
 */
SIMPLNX_EXPORT Result<nlohmann::json> LoadAllPresets();

/**
 * @brief ExtractControlPoints This method will create a 2-D array of control points based on the name of the preset
 * @param presetName this is a string that corresponds to a "name" of a json object
 * @return a result object holding errors and a vector<float64> that can be empty.
 */
SIMPLNX_EXPORT Result<std::vector<float32>> ExtractControlPoints(const std::string& presetName);

/**
 * @brief GetDefaultRGBPresetName This method will look for RGB json presets from the ColorTable.hpp JSON and return the first 'name' string
 * @return The string name of the first RGB preset or empty string if none found
 */
SIMPLNX_EXPORT std::string GetDefaultRGBPresetName();

/**
 * @brief IsValidPreset Returns true if the preset has the "RGBPoints" and "ColorSpace" keys
 * @param preset The preset to check in JSON form
 * @return
 */
SIMPLNX_EXPORT bool IsValidPreset(const nlohmann::json& preset);

/**
 * @brief IsValidPreset Returns true if the preset has the "RGBPoints" and "ColorSpace" keys
 * @param preset The preset to check as a ColorPreset object
 * @return
 */
SIMPLNX_EXPORT bool IsValidPreset(const ColorPreset& preset);

/**
 * @brief IsValidPreset Returns true if the preset has the "IndexedColors" keys
 * @param preset The preset to check in JSON form
 * @return
 */
SIMPLNX_EXPORT bool IsValidIndexedPreset(const nlohmann::json& preset);

/**
 * @brief IsValidPreset Returns true if the preset has the "IndexedColors" keys
 * @param preset The preset to check as a ColorPreset object
 * @return
 */
SIMPLNX_EXPORT bool IsValidIndexedPreset(const ColorPreset& preset);

} // namespace nx::core::ColorTableUtilities
