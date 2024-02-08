#include "ColorPresetsUtilities.hpp"

#include "simplnx/util/ColorTable.hpp"

#include <fmt/format.h>

namespace nx::core::ColorTableUtilities
{
// -----------------------------------------------------------------------------
Result<nlohmann::json> LoadAllRGBPresets()
{
  nlohmann::json allPresets;
  try
  {
    // insert additional presets here
    allPresets.update(ColorTable::k_ColorTableJson);
  } catch(nlohmann::json::type_error& exception)
  {
    // "Failed to parse presets file" error
    return MakeErrorResult<nlohmann::json>(-1000, fmt::format("Failed to combine presets json string: {}", exception.what()));
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
} // namespace nx::core
