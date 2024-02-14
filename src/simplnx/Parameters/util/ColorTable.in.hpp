#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>


namespace nx::core
{
constexpr StringLiteral k_DefaultColorTableJsonStr = R"~(@COLOR_TABLE_JSON@)~";
static const nlohmann::json k_DefaultColorTableJson = R"~(@COLOR_TABLE_JSON@)~"_json;

class ColorTable
{
public:
  static constexpr usize k_ControlPointCompSize = 4;

  ColorTable() = default;
  ~ColorTable() = default;
  
  /**
 * @brief LoadRGBPresets This method will combine RGB json presets from the ColorTable.hpp JSON string
 * @return The json object result
   */
  Result<nlohmann::json> LoadAllRGBPresets() const
  {
    nlohmann::json rgbPresets;
    for(const auto& preset : k_DefaultColorTableJson)
    {
      if(preset.contains("ColorSpace") && preset["ColorSpace"] == "RGB")
      {
        rgbPresets.push_back(preset);
      }
    }

    return {rgbPresets};
  }

  /**
  * @brief ExtractContolPoints This method will create a 2-D array of control points based on a the name of the preset
  * @param presetName this is a string that corresponds to a "name" of a json object
  * @return a result object holding errors and a vector<float64> that can be empty.
  */
  Result<std::vector<float32>> ExtractContolPoints(const std::string& presetName) const
  {
    for(const auto& preset : k_DefaultColorTableJson)
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
};
}