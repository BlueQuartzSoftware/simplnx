#pragma once

#include "ReconstructionFilterTestUtils.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace watershed_marker_test
{
/**
 * @brief Creates scalar grayscale and marker arrays on one image grid using the active storage preference.
 * @tparam TInput Grayscale element type.
 * @tparam TMarker Integer marker element type.
 * @param dataStructure Receives the image geometry, its cell data, and both arrays.
 * @param dimX Number of columns.
 * @param dimY Number of rows per slice.
 * @param dimZ Number of slices.
 * @param gray One grayscale value per cell in flat order, with X varying fastest, then Y, then Z.
 * @param markers One marker ID per cell in the same order as the grayscale values.
 * @return Pair containing the grayscale path first and the marker path second.
 * @note Geometry dimensions use X, Y, Z order. Array tuple dimensions use Z, Y, X order.
 */
template <class TInput, class TMarker>
std::pair<nx::core::DataPath, nx::core::DataPath> BuildGrayscaleAndMarker(nx::core::DataStructure& dataStructure, nx::core::usize dimX, nx::core::usize dimY, nx::core::usize dimZ,
                                                                          const std::vector<TInput>& gray, const std::vector<TMarker>& markers)
{
  using namespace nx::core;
  REQUIRE((markers.size() == gray.size()));
  const auto inputPath = recon_test::BuildImageFromPattern<TInput>(dataStructure, dimX, dimY, dimZ, gray);
  const auto cellPath = inputPath.getParent();
  const auto markerPath = cellPath.createChildPath("Marker");
  const auto cellId = dataStructure.getId(cellPath);
  REQUIRE(cellId.has_value());
  auto store = data_store_utilities::CreateDataStore<TMarker>(dataStructure, markerPath, ShapeType{dimZ, dimY, dimX}, ShapeType{1});
  REQUIRE((store != nullptr));
  REQUIRE((DataArray<TMarker>::Create(dataStructure, markerPath.getTargetName(), store, *cellId) != nullptr));
  constexpr usize k_ChunkValues = 65536;
  for(usize start = 0; start < markers.size();)
  {
    const usize count = std::min(k_ChunkValues, markers.size() - start);
    const auto result = store->copyFromBuffer(start, nonstd::span<const TMarker>(markers.data() + start, count));
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) The project assertion macro uses a do-while wrapper.
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    start += count;
  }
  return {inputPath, markerPath};
}

/**
 * @brief Builds Watershed From Markers arguments for the selected inputs and output.
 * @param inputPath Grayscale array in the image geometry's cell data.
 * @param markerPath Integer marker array on the same grid.
 * @param markWatershedLine Enables zero-valued watershed lines.
 * @param fullyConnected Includes diagonal neighbors when enabled.
 * @param outputName Name of the output array beside the grayscale input.
 * @return Arguments for preflight or execution. The conversion policy is omitted so tests can exercise the default or set it explicitly.
 */
nx::core::Arguments CreateArguments(const nx::core::DataPath& inputPath, const nx::core::DataPath& markerPath, bool markWatershedLine, bool fullyConnected, const std::string& outputName = "Output");
} // namespace watershed_marker_test
