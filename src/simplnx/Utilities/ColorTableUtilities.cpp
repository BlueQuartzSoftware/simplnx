#include "ColorTableUtilities.hpp"

#include "simplnx/util/ColorTable.hpp"

#include <fmt/format.h>

using namespace nx::core;

Result<nlohmann::json> ColorTableUtilities::LoadAllRGBPresets()
{
  nlohmann::json rgbPresets;
  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if(preset.contains("ColorSpace") && preset["ColorSpace"] == "RGB")
    {
      rgbPresets.push_back(preset);
    }
  }

  return {rgbPresets};
}

Result<std::vector<float32>> ColorTableUtilities::ExtractContolPoints(const std::string& presetName)
{
  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if(preset.contains("Name") && preset["ColorSpace"] == presetName)
    {
      if(preset.contains("RGBPoints"))
      {
        // Migrate colorControlPoints values from JsonArray to 2D array.
        return {{preset["RGBPoints"].get<std::vector<float32>>()}};
      }
    }
  }

  return MakeErrorResult<std::vector<float32>>(-36782, fmt::format("Unable to find the object for name {} in the JSON Table", presetName));
}

std::string ColorTableUtilities::GetDefaultRGBPresetName()
{
  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if(preset.contains("ColorSpace") && preset["ColorSpace"] == "RGB")
    {
      return preset["Name"];
    }
  }

  return {};
}
