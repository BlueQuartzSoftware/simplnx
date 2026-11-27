#include "simplnx/Utilities/ColorTableUtilities.hpp"

#include <catch2/catch.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>

using namespace nx::core;

TEST_CASE("ColorTableUtilities::ColorPreset keeps the License text", "[simplnx][ColorTableUtilities]")
{
  SECTION("Built-in preset")
  {
    // The built-in preset list stores the license under the key "License".
    const Result<color_table_utilities::ColorPresetsVectorType> presetsResult = color_table_utilities::CacheAllPresets(true, false);
    REQUIRE(presetsResult.valid());
    const color_table_utilities::ColorPresetsVectorType& presetsRef = presetsResult.value();
    const auto viridisIter = std::find_if(presetsRef.cbegin(), presetsRef.cend(), [](const color_table_utilities::ColorPreset& preset) { return preset.name == "Viridis (matplotlib)"; });
    REQUIRE(viridisIter != presetsRef.cend());
    REQUIRE(viridisIter->license == "CC0");
  }

  SECTION("Write then read")
  {
    color_table_utilities::ColorPreset preset;
    preset.name = "Licensed";
    preset.license = "CC0";
    preset.rgb_points = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f};
    const color_table_utilities::ColorPreset reread(preset.toJson());
    REQUIRE(reread.license == "CC0");
  }
}
