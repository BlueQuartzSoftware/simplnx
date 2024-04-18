#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace nx::core::ColorTableUtilities
{
/**
 * @brief LoadRGBPresets This method will combine RGB json presets from the ColorTable.hpp JSON string
 * @return The json object result
 */
SIMPLNX_EXPORT Result<nlohmann::json> LoadAllRGBPresets();

/**
 * @brief ExtractControlPoints This method will create a 2-D array of control points based on a the name of the preset
 * @param presetName this is a string that corresponds to a "name" of a json object
 * @return a result object holding errors and a vector<float64> that can be empty.
 */
SIMPLNX_EXPORT Result<std::vector<float32>> ExtractControlPoints(const std::string& presetName);

/**
 * @brief GetDefaultRGBPresetName This method will look for RGB json presets from the ColorTable.hpp JSON and return the first 'name' string
 * @return The string name of the first RGB preset or empty string if none found
 */
SIMPLNX_EXPORT std::string GetDefaultRGBPresetName();
} // namespace nx::core::ColorTableUtilities
