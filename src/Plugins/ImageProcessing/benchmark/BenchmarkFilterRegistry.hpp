#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ip_bench
{
using namespace nx::core;

/**
 * @brief Inputs available to a benchmark case's optional custom input-preparation callback.
 */
struct BenchmarkInputContext
{
  std::filesystem::path realInputPath;
  DataPath geometryPath;
  std::string cellAttributeMatrixName;
  std::string inputArrayName;
  usize targetVoxels = 0;
  usize minZSlices = 0;
  bool force2D = false;
  DataStorageMode storeMode = DataStorageMode::Adaptive;
};

/**
 * @brief One benchmark case: a new ITK-free ImageProcessing filter, its legacy ITK counterpart (if one is
 *        registered), the real input image its golden test uses, and an argument preset that mirrors that test.
 */
struct BenchmarkFilterSpec
{
  std::string name;                  ///< Short display name (e.g. "Abs").
  Uuid newFilterUuid;                ///< UUID of the new ITK-free filter (from FilterTraits<...>::uuid).
  std::optional<Uuid> legacyItkUuid; ///< UUID of the legacy ITKImageProcessing filter, or nullopt if none/disabled.
  std::string realInputFile;         ///< Input file name resolved via ip_golden::InputPath (its golden test's input).

  /// Populates @p args for a run: sets the geometry path, input array path, output array name, and any filter-
  /// specific parameters (kernel radii, connectivity, etc.) matching the filter's golden/parity test.
  std::function<void(Arguments& args, const DataPath& geom, const DataPath& inputArray, const std::string& outName)> setArgs;

  /// Optional case-specific setup for inputs that need more than one tiled source array or an untimed preprocessing
  /// filter. Returns the array that the benchmarked filter should receive as its primary input. When empty, the
  /// runner uses BuildTiledInput with realInputFile exactly as before.
  std::function<Result<DataPath>(DataStructure& dataStructure, const BenchmarkInputContext& context)> prepareInput;

  /// True only for filters whose setArgs callback uses the common "projection_dimension" key. When true, the
  /// runner may replace that preset with SweepOptions::projectionAxis; every other filter ignores the option.
  bool supportsProjectionAxisOverride = false;
};

/**
 * @brief The benchmark registry: every registered ImageProcessing filter (the original 13-filter representative
 *        subset first, then the full-plugin expansion). Built once and cached. Consumed by `--filters all`.
 */
const std::vector<BenchmarkFilterSpec>& GetBenchmarkFilterRegistry();

} // namespace ip_bench
