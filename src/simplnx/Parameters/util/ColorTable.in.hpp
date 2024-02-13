#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>


namespace nx::core
{
SIMPLNX_EXPORT constexpr StringLiteral k_DefaultColorTableJsonStr = R"~(@COLOR_TABLE_JSON@)~";

class ColorTable
{
  ColorTable()
  {
    if(m_AllColorTablesJson.empty())
    {
      m_AllColorTablesJson = nlohmann::json(k_DefaultColorTableJsonStr);
    }
  }
  ~ColorTable() = default;
  
  /**
 * @brief ReadRGBPresets This method will combine RGB json presets from the ColorTable.hpp JSON string
 * @return The json object result
   */
  Result<nlohmann::json> LoadAllRGBPresets() const
  {
    nlohmann::json rgbPresets;
    for(const auto& preset : m_AllColorTablesJson)
    {
      if(preset.contains("ColorSpace") && preset["ColorSpace"] == "RGB")
      {
        rgbPresets.push_back(preset);
      }
    }

    return {rgbPresets};
  }

  /**
  * @brief ReadRGBPresets This method will combine RGB json presets from the ColorTable.hpp JSON string
  * @return The json object result
  */
  Result<std::vector<float64>> ExtractContolPoints(const std::string& presetName) const
  {
    for(const auto& preset : m_AllColorTablesJson)
    {
      if(preset.contains("Name") && preset["ColorSpace"] == presetName)
      {
        if(preset.contains("RGBPoints"))
        {
          // Migrate colorControlPoints values from JsonArray to 2D array.
          return {{preset["RGBPoints"].get<std::vector<float64>>()}};
        }
      }
    }


    return MakeErrorResult<std::vector<float64>>(-36782, fmt::format("Unable to find the object for name {} in the JSON Table", presetName));
  }

private:
  static const nlohmann::json m_AllColorTablesJson;
};
}