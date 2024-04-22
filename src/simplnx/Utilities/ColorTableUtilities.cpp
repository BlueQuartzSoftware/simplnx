#include "ColorTableUtilities.hpp"

#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/util/ColorTable.hpp"

#include <fmt/format.h>

#include <iostream>

using namespace nx::core;

bool ColorTableUtilities::IsValidPreset(const nlohmann::json& preset)
{
  bool hasRgbPoints = preset.contains("RGBPoints");
  bool rgbPointsIsArray = (hasRgbPoints && preset["RGBPoints"].is_array() ? true : false);
  bool hasColorSpace = preset.contains("ColorSpace");
  std::string colorSpaceValue = (hasColorSpace ? preset["ColorSpace"].get<std::string>() : "");

  if(rgbPointsIsArray && (hasColorSpace && (colorSpaceValue == "RGB" || colorSpaceValue == "Diverging" || colorSpaceValue == "Lab" || colorSpaceValue == "CIELAB")))
  {
    return true;
  }
  return false;
}

Result<nlohmann::json> ColorTableUtilities::LoadAllRGBPresets()
{
  nlohmann::json rgbPresets;

  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if(IsValidPreset(preset))
    {
      rgbPresets.push_back(preset);
    }
  }

  // Sort the presets by name
  std::sort(rgbPresets.begin(), rgbPresets.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
    return nx::core::StringUtilities::toLower(a["Name"].get<std::string>()) < nx::core::StringUtilities::toLower(b["Name"].get<std::string>());
  });

  return {rgbPresets};
}

Result<std::vector<float32>> ColorTableUtilities::ExtractControlPoints(const std::string& presetName)
{
  if(presetName.empty())
  {
    return MakeErrorResult<std::vector<float32>>(-36781, fmt::format("{}({}): Function {}: Search argument is empty!", __FILE__, __LINE__, "ColorTableUtilities::ExtractControlPoints"));
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
                                                                     "ColorTableUtilities::ExtractControlPoints", presetName));
  }

  return MakeErrorResult<std::vector<float32>>(
      -36783, fmt::format("{}({}): Function {}: Unable to find the object for name '{}' in the JSON Table", __FILE__, __LINE__, "ColorTableUtilities::ExtractControlPoints", presetName));
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
