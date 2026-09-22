#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Preferences.hpp" // nx::core::DataStorageMode
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

#include <filesystem>
#include <functional>
#include <string>

namespace ip_bench
{
using namespace nx::core;

using SourceArrayValidator = std::function<Result<>(const IDataArray& sourceArray, const std::filesystem::path& sourcePath)>;

/**
 * @brief Builds a large, tile-replicated input from a filter's real input image, for benchmarking.
 *
 * The real input at @p realInput is read once through the ITK-free reader into a scratch DataStructure to recover
 * its dimensions, element type, and component count. A destination ImageGeom plus matching cell DataArray are
 * created in @p ds and filled by tiling the source. Normally tile factors grow Z first, then XY, so the destination
 * reaches at least @p targetVoxels and @p minZSlices. The explicit force2D mode instead selects source slice zero and
 * grows only XY.
 *
 * The fill is streamed slice-by-slice, so only one source and one destination slice are materialized.
 *
 * The requested mode is temporarily applied through Preferences. The canonical DataStructure resolver chooses the
 * format for the actual destination path, type, and byte count. The result is verified against @p storeMode.
 *
 * Deterministic; preserves source element type and component interleave.
 *
 * @param ds DataStructure in which to create the destination geometry and array.
 * @param realInput Input image path routed through the ITK-free reader.
 * @param geomPath Destination ImageGeom path.
 * @param cellAmName Destination cell AttributeMatrix name.
 * @param arrayName Destination cell DataArray name.
 * @param targetVoxels Lower bound on destination voxel count.
 * @param minZSlices Lower bound on destination Z extent.
 * @param storeMode Required storage mode for the destination array.
 * @param[out] outArrayPath Created destination array path.
 * @param force2D When true, uses only source slice zero and sets destination Z to one while tiling X/Y to targetVoxels.
 * @param sourceValidator Optional validator run on the small in-core source before replication.
 * @return Any read, validation, size, allocation, or fill error.
 */
Result<> BuildTiledInput(DataStructure& ds, const std::filesystem::path& realInput, const DataPath& geomPath, const std::string& cellAmName, const std::string& arrayName, usize targetVoxels,
                         usize minZSlices, DataStorageMode storeMode, DataPath& outArrayPath, bool force2D = false, const SourceArrayValidator& sourceValidator = {});

/**
 * @brief Adds a tiled array to an ImageGeom previously created by BuildTiledInput.
 *
 * The source image must tile exactly into the existing geometry in each dimension.
 *
 * This lets benchmark cases add a distinct mask, marker, or other companion array without materializing the
 * destination volume in memory. The optional validator runs on the small in-core source before replication.
 */
Result<> AddTiledArray(DataStructure& ds, const std::filesystem::path& realInput, const DataPath& geomPath, const std::string& cellAmName, const std::string& arrayName, DataStorageMode storeMode,
                       DataPath& outArrayPath, const SourceArrayValidator& sourceValidator = {});

} // namespace ip_bench
