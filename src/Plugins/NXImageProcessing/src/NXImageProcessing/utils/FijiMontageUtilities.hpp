#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"

#include <filesystem>
#include <string>
#include <vector>

/**
 * @file FijiMontageUtilities.hpp
 * @brief Framework-independent parsing for Fiji TileConfiguration files, shared by
 *        ImportFijiMontageFilter. A Fiji TileConfiguration[.registered].txt lists, after
 *        a `dim = N` line and a `# Define the image coordinates` marker, one tile per
 *        line in the form:  `<file>; ; (x, y[, z])`. This reader is ITK-free (lifted from
 *        the legacy ITKImportFijiMontage IOHandler::parseConfigFile) and knows nothing
 *        about DataStructure/ImageGeom.
 */
namespace nx::core::fiji
{
/** @brief One montage tile: source file, world origin, and the (prefix-decorated) geometry name. */
struct IMAGEPROCESSING_EXPORT FijiTile
{
  std::filesystem::path filePath;     ///< Absolute path to the tile image (resolved against the config dir).
  FloatVec3 origin{0.0f, 0.0f, 0.0f}; ///< Parsed tile origin (Z=0 for 2D configs).
  std::string imageName;              ///< `<prefix><file-stem>`; filled by AssignTileNames.
};

/**
 * @brief Parses a Fiji TileConfiguration file into tiles. Tile file paths are resolved
 *        relative to the config file's parent directory. Malformed tile lines are skipped
 *        with no hard failure; a header-less file or a file yielding zero tiles is an error.
 * @return the parsed tiles (imageName not yet assigned) or a -359xx error.
 */
Result<std::vector<FijiTile>> IMAGEPROCESSING_EXPORT ParseTileConfiguration(const std::filesystem::path& configPath);

/** @brief Sets each tile's imageName to `<prefix><file-stem>`. */
void IMAGEPROCESSING_EXPORT AssignTileNames(std::vector<FijiTile>& tiles, const std::string& prefix);

/**
 * @brief Rebases every tile origin so the montage's minimum corner lands on @p userOrigin.
 *        newOrigin = origin - (minCorner - userOrigin). Matches the legacy change-origin path.
 */
void IMAGEPROCESSING_EXPORT RebaseOrigins(std::vector<FijiTile>& tiles, const FloatVec3& userOrigin);

} // namespace nx::core::fiji
