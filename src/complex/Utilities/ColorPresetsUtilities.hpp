#pragma once

#include "complex/Common/Result.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace complex
{
/**
 * @brief ReadRGBPresets This method will read RGB presets from a given presets JSON string
 * @param presetsJson The presets JSON string
 * @return The json object result
 */
COMPLEX_EXPORT Result<nlohmann::json> ReadRGBPresets(const std::string& presetsJson);

/**
 * @brief ReadRGBPresets This method will read RGB presets from a given presets JSON file
 * @param presetsJson The presets JSON file path
 * @return The json object result
 */
COMPLEX_EXPORT Result<nlohmann::json> ReadRGBPresets(const fs::path& presetsFilePath);
} // namespace complex
