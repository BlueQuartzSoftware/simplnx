#pragma once

#include "complex/Common/Result.hpp"

#include "nlohmann/json.hpp"
#include <fmt/format.h>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace complex
{
// Description:
// Reads the RGB presets using a JSON string.
// -----------------------------------------------------------------------------
Result<nlohmann::json> readRGBPresets(const std::string& presetsJson)
{
  nlohmann::json allPresets;
  try
  {
    allPresets = nlohmann::json::parse(presetsJson);
  } catch(nlohmann::json::parse_error& exception)
  {
    // "Failed to parse presets file" error
    return MakeErrorResult<nlohmann::json>(-1000, fmt::format("Failed to parse presets json string: {}", exception.what()));
  }

  nlohmann::json rgbPresets;
  for(const auto& preset : allPresets)
  {
    if(preset.contains("ColorSpace") && preset["ColorSpace"] == "RGB")
    {
      rgbPresets.push_back(preset);
    }
  }

  return {rgbPresets};
}

// Description:
// Reads the RGB presets using the path to the presets JSON file.
// -----------------------------------------------------------------------------
Result<nlohmann::json> readRGBPresets(const fs::path& presetsFilePath)
{
  const std::ifstream iStream(presetsFilePath.string());
  std::stringstream buffer;
  buffer << iStream.rdbuf();
  return readRGBPresets(buffer.str());
}
} // namespace complex
