#include "ColorPresetsUtilities.hpp"

#include <fmt/format.h>
#include <fstream>
#include <sstream>

namespace nx::core
{
// -----------------------------------------------------------------------------
Result<nlohmann::json> ReadRGBPresets(const std::string& presetsJson)
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

// -----------------------------------------------------------------------------
Result<nlohmann::json> ReadRGBPresets(const fs::path& presetsFilePath)
{
  const std::ifstream iStream(presetsFilePath.string());
  std::stringstream buffer;
  buffer << iStream.rdbuf();
  return ReadRGBPresets(buffer.str());
}
} // namespace nx::core
