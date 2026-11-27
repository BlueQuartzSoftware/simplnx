#pragma once

#include "BenchmarkFilterRegistry.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ip_bench
{
using namespace nx::core;

/**
 * @brief Median + min wall-clock timing of a callable across repeated executions.
 */
struct Timing
{
  double median_ms = 0.0;
  double min_ms = 0.0;
  int repeats = 0;
};

/**
 * @brief Runs @p warmup un-timed iterations of @p exec, then @p repeats timed iterations, and returns the median +
 *        min wall-clock milliseconds across the timed iterations (steady_clock).
 *
 * Only @p exec is timed -- input building, preflight pre-checks, and DataStructure setup all happen outside. Because
 * RunTimed has no separate per-iteration reset hook, a caller that must clear a prior run's output folds that (cheap)
 * cleanup into @p exec itself; the heavy filter execute() dominates the measurement. If @p exec throws, the exception
 * propagates out of RunTimed (the caller isolates the failure).
 */
Timing RunTimed(const std::function<void()>& exec, int warmup, int repeats);

/**
 * @brief Knobs shared across a benchmark sweep.
 */
struct SweepOptions
{
  usize target_voxels = 256000000ULL;    ///< Lower bound on the tiled voxel count.
  usize min_z_slices = 256;              ///< Lower bound on the tiled Z extent.
  bool force_2d = false;                 ///< When true, tile source slice zero into a Z=1 destination.
  int warmup = 1;                        ///< Un-timed iterations before timing.
  int repeats = 3;                       ///< Timed iterations per (filter, config).
  bool ooc_requested = false;            ///< True when the CLI requested --mode ooc.
  bool ooc_only = false;                 ///< Skip InCore timings when only the OOC result is needed.
  int64 ooc_threshold = 1024;            ///< large_data_size threshold applied with ForceOutOfCore.
  std::optional<uint32> projection_axis; ///< Optional axis override applied only to flagged projection specs.
};

/**
 * @brief One CSV row: the two per-config median timings (nullopt = the config did not run) plus notes.
 */
struct BenchmarkRow
{
  std::string filter;                  ///< Filter display name.
  std::string type;                    ///< Input array DataType, or "" if the input could not be built.
  std::string in_dims;                 ///< Tiled input dimensions "XxYxZ".
  uint64 in_voxels = 0;                ///< Tiled input voxel count.
  std::optional<double> new_incore_ms; ///< In-core median ms.
  std::optional<double> new_ooc_ms;    ///< Out-of-core median ms (nullopt = skipped).
  std::string notes;                   ///< Per-config skip reasons / error messages.
};

/**
 * @brief Runs the two configs (new-in-core, new-OOC) for one filter spec.
 *
 * Never throws: any per-config preflight/execute failure is caught and recorded in the row's notes so the remaining
 * config (and the rest of the sweep) still runs. The in-core tiled input is built ONCE; the OOC config builds its own
 * disk-backed tiled input once. Within a config the input is NOT rebuilt per repeat -- each timed iteration only
 * clears the prior run's output object(s) back to the freshly tiled baseline before executing.
 */
BenchmarkRow RunFilterBenchmark(const BenchmarkFilterSpec& spec, const SweepOptions& opts);

/**
 * @brief Runs RunFilterBenchmark for every spec (per-filter error isolation) and returns the rows. Prints a one-line
 *        console result as each filter finishes.
 */
std::vector<BenchmarkRow> RunSweep(const std::vector<const BenchmarkFilterSpec*>& specs, const SweepOptions& opts);

/**
 * @brief Writes @p rows to @p path as CSV: a header plus one row per filter. The ratio column is recomputed from the
 *        recorded milliseconds (ooc_vs_incore = new_ooc/new_incore).
 */
Result<> WriteCsv(const std::string& path, const std::vector<BenchmarkRow>& rows);

/**
 * @brief Prints the optimization-priority summary: filters sorted by ooc_vs_incore (worst first).
 */
void PrintSummary(const std::vector<BenchmarkRow>& rows);

/**
 * @brief Merge mode: joins an in-core CSV and an OOC CSV on filter+type, takes new_incore_ms from the first and
 *        new_ooc_ms from the second, recomputes the ratio column, and writes the merged CSV to @p outPath. No filters
 *        are run.
 */
Result<> MergeCsv(const std::string& incoreCsv, const std::string& oocCsv, const std::string& outPath);

} // namespace ip_bench
