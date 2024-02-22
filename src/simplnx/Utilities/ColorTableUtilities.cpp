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
  if(presetName.empty())
  {
    return MakeErrorResult<std::vector<float32>>(-36781, fmt::format("{}({}): Function {}: Search argument is empty!", __FILE__, __LINE__, "ColorTableUtilities::ExtractContolPoints"));
  }

  bool found = false;
  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if(preset.contains("Name") && preset["Name"] == presetName)
    {
      found = true;
      if(preset.contains("RGBPoints"))
      {
        // Migrate colorControlPoints values from JsonArray to array.
        return {{preset["RGBPoints"].get<std::vector<float32>>()}};
      }
    }
  }

  if(!found)
  {
    return MakeErrorResult<std::vector<float32>>(-36782, fmt::format("{}({}): Function {}: Found the object for name '{}' in the JSON Table, but no 'RGBPoints' found", __FILE__, __LINE__,
                                                                     "ColorTableUtilities::ExtractContolPoints", presetName));
  }

  return MakeErrorResult<std::vector<float32>>(
      -36783, fmt::format("{}({}): Function {}: Unable to find the object for name '{}' in the JSON Table", __FILE__, __LINE__, "ColorTableUtilities::ExtractContolPoints", presetName));
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
