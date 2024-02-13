#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/util/ColorTable.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <string>

namespace nx::core
{
template<bool SearchCustom = false>
struct ColorTableUtilities
{
  /**
 * @brief ReadRGBPresets This method will combine RGB json presets from the ColorTable.hpp JSON string
 * @return The json object result
   */
  SIMPLNX_EXPORT Result<nlohmann::json> LoadAllRGBPresets()
  {
    nlohmann::json allPresets;
    try
    {
      // insert additional presets here
      if constexpr()
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

  /**
 * @brief ReadRGBPresets This method will combine RGB json presets from the ColorTable.hpp JSON string
 * @return The json object result
   */
  SIMPLNX_EXPORT Result<std::vector<float64>> ExtractContolPoints(const std::string& presetName);
};

using DefaultColorTableUtilities = ColorTableUtilities<true>;
using CustomColorTableUtilities = ColorTableUtilities<true>;
} // namespace nx::core
