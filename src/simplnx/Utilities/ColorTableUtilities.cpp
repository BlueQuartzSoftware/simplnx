#include "ColorTableUtilities.hpp"

#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/util/ColorTable.hpp"

#include <fmt/format.h>

using namespace nx::core;

/* *****************************************************************************
 *
 * ColorTableUtilities::ColorPreset Implementation
 *
 ******************************************************************************/

ColorTableUtilities::ColorPreset::ColorPreset() = default;

ColorTableUtilities::ColorPreset::ColorPreset(const nlohmann::json& a)
{
  if(a.contains("Name") && a["Name"].is_string())
  {
    this->Name = a["Name"].get<std::string>();
  }
  if(a.contains("ColorSpace") && a["ColorSpace"].is_string())
  {
    this->ColorSpace = a["ColorSpace"].get<std::string>();
  }
  if(a.contains("Source") && a["Source"].is_string())
  {
    this->Source = a["Source"].get<std::string>();
  }
  if(a.contains("Licence") && a["Licence"].is_string())
  {
    this->License = a["Licence"].get<std::string>();
  }
  if(a.contains("Creator") && a["Creator"].is_string())
  {
    this->Creator = a["Creator"].get<std::string>();
  }

  if(a.contains("NanColor") && a["NanColor"].is_array())
  {
    this->NanColor = a["NanColor"].get<std::vector<float>>();
  }
  if(a.contains("RGBPoints") && a["RGBPoints"].is_array())
  {
    this->RGBPoints = a["RGBPoints"].get<std::vector<float>>();
  }
  if(a.contains("IndexedColors") && a["IndexedColors"].is_array())
  {
    this->IndexedColors = a["IndexedColors"].get<std::vector<float>>();
  }

  if(a.contains("DefaultMap") && a["DefaultMap"].is_boolean())
  {
    this->DefaultMap = a["DefaultMap"].get<bool>();
  }

  if(a.contains("Annotations"))
  {
    this->Annotations = a["Annotations"].get<nlohmann::json>();
  }
  this->Invalid = false;
}

nlohmann::json ColorTableUtilities::ColorPreset::toJson() const
{
  nlohmann::json json;
  json["Name"] = Name;
  json["ColorSpace"] = ColorSpace;
  json["Source"] = Source;
  json["License"] = License;
  json["Creator"] = Creator;
  json["NanColor"] = NanColor;
  json["RGBPoints"] = RGBPoints;
  json["IndexedColors"] = IndexedColors;
  json["DefaultMap"] = DefaultMap;
  json["Annotations"] = Annotations;
  return json;
}

std::string ColorTableUtilities::ColorPreset::toJsonString(int indent) const
{
  nlohmann::json json = toJson();
  return json.dump(indent);
}

/* *****************************************************************************
 *
 * Begin ColorTableUtilities Functions
 *
 ******************************************************************************/

bool ColorTableUtilities::IsValidPreset(const ColorPreset& preset)
{
  if(!preset.RGBPoints.empty() && ((preset.ColorSpace == "RGB" || preset.ColorSpace == "Diverging" || preset.ColorSpace == "Lab" || preset.ColorSpace == "CIELAB")))
  {
    return true;
  }
  return false;
}

bool ColorTableUtilities::IsValidIndexedPreset(const ColorPreset& preset)
{
  return !preset.IndexedColors.empty();
}

bool ColorTableUtilities::IsValidPreset(const nlohmann::json& preset)
{
  const bool hasRgbPoints = preset.contains("RGBPoints");
  const bool rgbPointsIsArray = (hasRgbPoints && preset["RGBPoints"].is_array() ? true : false);
  const bool hasColorSpace = preset.contains("ColorSpace");
  const std::string colorSpaceValue = (hasColorSpace ? preset["ColorSpace"].get<std::string>() : "");

  if(rgbPointsIsArray && (hasColorSpace && (colorSpaceValue == "RGB" || colorSpaceValue == "Diverging" || colorSpaceValue == "Lab" || colorSpaceValue == "CIELAB")))
  {
    return true;
  }
  return false;
}

bool ColorTableUtilities::IsValidIndexedPreset(const nlohmann::json& preset)
{
  const bool hasIndexedColors = preset.contains("IndexedColors");
  const bool indexedColorsIsArray = hasIndexedColors && preset["IndexedColors"].is_array();
  return indexedColorsIsArray;
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
  std::sort(rgbPresets.begin(), rgbPresets.end(),
            [](const nlohmann::json& a, const nlohmann::json& b) { return StringUtilities::toLower(a["Name"].get<std::string>()) < StringUtilities::toLower(b["Name"].get<std::string>()); });

  return {rgbPresets};
}

Result<nlohmann::json> ColorTableUtilities::LoadAllIndexedPresets()
{
  nlohmann::json indexedPresets;

  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if(IsValidIndexedPreset(preset))
    {
      indexedPresets.push_back(preset);
    }
  }

  // Sort the presets by name
  std::sort(indexedPresets.begin(), indexedPresets.end(),
            [](const nlohmann::json& a, const nlohmann::json& b) { return StringUtilities::toLower(a["Name"].get<std::string>()) < StringUtilities::toLower(b["Name"].get<std::string>()); });

  return {indexedPresets};
}

Result<ColorTableUtilities::ColorPresetsVectorType> ColorTableUtilities::CacheAllPresets(bool rgbPreset, bool indexedPreset)
{
  ColorPresetsVectorType presets;
  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if((indexedPreset && IsValidIndexedPreset(preset)) || (rgbPreset && IsValidPreset(preset)))
    {
      presets.emplace_back(preset);
    }
  }

  // Sort the presets by name
  std::sort(presets.begin(), presets.end(), [](const ColorPreset& a, const ColorPreset& b) { return StringUtilities::toLower(a.Name) < StringUtilities::toLower(b.Name); });

  return {presets};
}

Result<nlohmann::json> ColorTableUtilities::LoadAllPresets()
{
  nlohmann::json presets;

  for(const auto& preset : ColorTable::k_DefaultColorTableJson)
  {
    if(IsValidIndexedPreset(preset) || IsValidPreset(preset))
    {
      presets.push_back(preset);
    }
  }

  // Sort the presets by name
  std::sort(presets.begin(), presets.end(),
            [](const nlohmann::json& a, const nlohmann::json& b) { return StringUtilities::toLower(a["Name"].get<std::string>()) < StringUtilities::toLower(b["Name"].get<std::string>()); });

  return {presets};
}

Result<std::vector<float32>> ColorTableUtilities::ExtractControlPoints(const std::string& presetName)
{
  if(presetName.empty())
  {
    return MakeErrorResult<std::vector<float32>>(-36781, "ColorTableUtilities::ExtractControlPoints: Search argument is empty!");
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
    return MakeErrorResult<std::vector<float32>>(-36782, fmt::format("ColorTableUtilities::ExtractControlPoints: Found the object for name '{}' in the JSON Table, but no 'RGBPoints' found",
                                                                     presetName));
  }

  return MakeErrorResult<std::vector<float32>>(
      -36783, fmt::format("ColorTableUtilities::ExtractControlPoints: Unable to find the object for name '{}' in the JSON Table", presetName));
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
