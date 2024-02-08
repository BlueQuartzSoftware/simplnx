#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

namespace nx::core::ColorTableUtilities
{
/**
 * @brief ReadRGBPresets This method will read RGB presets from a given presets JSON string
 * @param presetsJson The presets JSON string
 * @return The json object result
 */
SIMPLNX_EXPORT Result<nlohmann::json> LoadAllRGBPresets();
} // namespace nx::core
