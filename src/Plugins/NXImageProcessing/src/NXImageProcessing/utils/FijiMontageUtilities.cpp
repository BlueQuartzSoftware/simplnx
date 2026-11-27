#include "FijiMontageUtilities.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <fstream>
#include <limits>

namespace nx::core::fiji
{
Result<std::vector<FijiTile>> ParseTileConfiguration(const std::filesystem::path& configPath)
{
  std::ifstream inStream(configPath, std::ios_base::binary);
  if(!inStream.is_open())
  {
    return MakeErrorResult<std::vector<FijiTile>>(-35900, fmt::format("Unable to open the Fiji TileConfiguration file for reading: '{}'", configPath.string()));
  }

  usize lineNumber = 0;
  bool dimFound = false;
  bool dataFound = false;
  std::string line;
  while(std::getline(inStream, line))
  {
    ++lineNumber;
    line = string_utilities::Trimmed(line);
    if(string_utilities::StartsWith(line, "dim ="))
    {
      dimFound = true; // Fiji montages handled here are 2D; the value is informational.
    }
    if(string_utilities::StartsWith(line, "# Define the image coordinates"))
    {
      dataFound = true;
      break;
    }
  }

  if(!dimFound || !dataFound)
  {
    return MakeErrorResult<std::vector<FijiTile>>(
        -35901, fmt::format("'{}' is not a valid Fiji TileConfiguration file (missing the 'dim =' line and/or the '# Define the image coordinates' marker).", configPath.string()));
  }

  // Example tile line:  section_3_m01_ORG.tif; ; (0.0, 0.0)
  std::vector<FijiTile> tiles;
  while(std::getline(inStream, line))
  {
    ++lineNumber;
    line = string_utilities::Trimmed(line);
    if(line.empty())
    {
      continue;
    }
    std::vector<std::string> tokens = string_utilities::Split(line, ';');
    if(tokens.size() != 3)
    {
      continue; // malformed line (token count != 3): silently skipped, matching legacy leniency
    }

    std::string coords = string_utilities::Trimmed(tokens[2]);
    coords = string_utilities::Replace(coords, "(", "");
    coords = string_utilities::Replace(coords, ")", "");
    std::vector<std::string> coordTokens = string_utilities::Split(coords, ',');
    if(coordTokens.size() != 2)
    {
      continue; // only 2D configs are supported
    }

    FijiTile tile;
    tile.file_path = configPath.parent_path() / string_utilities::Trimmed(tokens[0]);
    try
    {
      const float32 x = std::stof(string_utilities::Trimmed(coordTokens[0]));
      const float32 y = std::stof(string_utilities::Trimmed(coordTokens[1]));
      tile.origin = FloatVec3Type(x, y, 0.0f);
    } catch(const std::exception&)
    {
      continue; // unparseable coordinate, skip
    }
    if(auto result = ImageGeom::ValidateOrigin(tile.origin); result.invalid())
    {
      auto& error = result.errors().front();
      error.message = fmt::format("Input file '{}' has invalid origin values at line {}. Error Message: {} Correct the stored tile coordinates.", configPath.string(), lineNumber, error.message);
      return ConvertInvalidResult<std::vector<FijiTile>>(std::move(result));
    }
    tiles.push_back(std::move(tile));
  }

  if(tiles.empty())
  {
    return MakeErrorResult<std::vector<FijiTile>>(-35903, fmt::format("The Fiji TileConfiguration file '{}' did not contain any valid tile entries.", configPath.string()));
  }

  return {std::move(tiles)};
}

void AssignTileNames(std::vector<FijiTile>& tiles, const std::string& prefix)
{
  for(auto& tile : tiles)
  {
    tile.image_name = prefix + tile.file_path.stem().string();
  }
}

void RebaseOrigins(std::vector<FijiTile>& tiles, const FloatVec3Type& userOrigin)
{
  FloatVec3Type minCorner{std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max(), 0.0f};
  for(const auto& tile : tiles)
  {
    minCorner[0] = std::min(tile.origin[0], minCorner[0]);
    minCorner[1] = std::min(tile.origin[1], minCorner[1]);
  }
  minCorner[2] = 0.0f; // 2D montage: the Z minimum is always 0 (hoisted out of the loop)
  const FloatVec3Type delta{minCorner[0] - userOrigin[0], minCorner[1] - userOrigin[1], minCorner[2] - userOrigin[2]};
  for(auto& tile : tiles)
  {
    tile.origin[0] -= delta[0];
    tile.origin[1] -= delta[1];
    tile.origin[2] -= delta[2];
  }
}

} // namespace nx::core::fiji
