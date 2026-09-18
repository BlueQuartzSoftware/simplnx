// ImageProcessingBenchmark -- a standalone developer tool that times ImageProcessing filters on large,
// tile-replicated real inputs. A few tiny CTest smoke invocations exercise its custom input preparations; this file
// provides the CLI, plugin loading, three-config timing sweep, CSV output, and merge mode.

#include "BenchmarkFilterRegistry.hpp"
#include "BenchmarkRunner.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#ifdef SIMPLNX_IMAGEPROCESSING_BENCH_HAS_OOC
// Only present in the OOC harness build (where the SimplnxOoc target exists). These wire up the disk-backed
// "HDF5-OOC" backend so `--mode ooc` produces real out-of-core stores; the standalone build compiles none of it.
#include "SimplnxOoc/OocDataIOManager.hpp"         // SimplnxOoc::registerIOManager / SimplnxOoc::shutdown
#include "SimplnxOoc/PreferenceFormatResolver.hpp" // SimplnxOoc::PreferenceFormatResolver
#include "simplnx/DataStructure/DataStructure.hpp" // DataStructure::setDefaultFormatResolver

#include <memory>
#endif

#include <fmt/format.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

using namespace nx::core;

namespace
{
struct CliOptions
{
  std::string mode = "incore";       // incore | ooc
  usize targetVoxels = 256000000ULL; // ~256M voxels
  usize zSlices = 256;
  bool force2D = false;
  std::optional<uint32> projectionAxis;
  int warmup = 1;
  int repeats = 3;
  bool oocOnly = false;
  bool newOnly = false;
  std::optional<uint64> cacheMemoryBudgetBytes;
  std::optional<uint64> workingMemoryBytes;
  std::string filters = "subset"; // subset | all | <name>[,<name>...]
  std::string outCsv;             // CSV output path (enables the full run + CSV write)
  bool requireNewResult = false;  // Exit nonzero if the selected new-filter configuration does not produce a timing.
  bool doMerge = false;           // --merge <incoreCsv> <oocCsv> : join two CSVs, no filters run
  std::string mergeIncoreCsv;
  std::string mergeOocCsv;
};

std::string FirstErrorMessage(const std::vector<Error>& errors)
{
  return errors.empty() ? "(no message)" : errors.front().message;
}

void PrintUsage()
{
  fmt::print("Usage: ImageProcessingBenchmark [options]\n"
             "  --mode {{incore|ooc}}     Storage mode for the tiled input (default: incore)\n"
             "  --target-voxels N        Lower bound on tiled voxel count (default: 256000000)\n"
             "  --z-slices N             Lower bound on tiled Z extent (default: 256)\n"
             "  --force-2d               Tile source slice zero into an exact Z=1 destination\n"
             "  --projection-axis N      Override axis for projection filters: 0=X, 1=Y, 2=Z\n"
             "  --warmup N               Un-timed iterations before timing (default: 1)\n"
             "  --repeats N              Timed executions per filter/config (default: 3)\n"
             "  --cache-memory-budget-bytes N  Cache budget for this process; working memory is limited to 25%\n"
             "  --working-memory-bytes N Override ImageProcessing's preferred working-memory request\n"
             "  --ooc-only               With --mode ooc, skip ImageProcessing and ITK InCore timing\n"
             "  --new-only               Run ImageProcessing InCore without the legacy ITK comparator\n"
             "  --filters {{subset|all|<name>[,<name>...]}}  Which filters to run (default: subset)\n"
             "  --require-new-result     Exit nonzero if a selected new-filter configuration fails or is skipped\n"
             "  --out <csv>              CSV output path (writes the full sweep + prints the summary)\n"
             "  --merge <incore.csv> <ooc.csv>   Join two CSVs on filter+type into --out (no filters run)\n");
}

// Returns false if a value was expected but missing.
bool NextValue(int argc, char** argv, int& i, std::string& out)
{
  if(i + 1 >= argc)
  {
    return false;
  }
  out = argv[++i];
  return true;
}
} // namespace

int main(int argc, char** argv)
{
  CliOptions opts;

  for(int i = 1; i < argc; ++i)
  {
    const std::string_view arg = argv[i];
    std::string value;
    try
    {
      if(arg == "--mode" && NextValue(argc, argv, i, value))
      {
        opts.mode = value;
      }
      else if(arg == "--target-voxels" && NextValue(argc, argv, i, value))
      {
        opts.targetVoxels = std::stoull(value);
      }
      else if(arg == "--z-slices" && NextValue(argc, argv, i, value))
      {
        opts.zSlices = std::stoull(value);
      }
      else if(arg == "--force-2d")
      {
        opts.force2D = true;
      }
      else if(arg == "--projection-axis")
      {
        if(!NextValue(argc, argv, i, value))
        {
          fmt::print("--projection-axis requires a value of 0, 1, or 2\n");
          return 1;
        }
        const uint64 parsedAxis = std::stoull(value);
        if(parsedAxis > 2)
        {
          fmt::print("--projection-axis must be 0, 1, or 2 (got '{}')\n", value);
          return 1;
        }
        opts.projectionAxis = static_cast<uint32>(parsedAxis);
      }
      else if(arg == "--warmup" && NextValue(argc, argv, i, value))
      {
        opts.warmup = std::stoi(value);
      }
      else if(arg == "--repeats" && NextValue(argc, argv, i, value))
      {
        opts.repeats = std::stoi(value);
      }
      else if(arg == "--cache-memory-budget-bytes")
      {
        if(!NextValue(argc, argv, i, value))
        {
          fmt::print("--cache-memory-budget-bytes requires a positive integer byte count. No value was provided.\n");
          return 1;
        }
        if(value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
        {
          throw std::invalid_argument(fmt::format("expected decimal digits only, but received '{}'", value));
        }
        const uint64 parsedBytes = std::stoull(value);
        if(parsedBytes == 0)
        {
          fmt::print("--cache-memory-budget-bytes requires a positive integer byte count. Received {}.\n", parsedBytes);
          return 1;
        }
        opts.cacheMemoryBudgetBytes = parsedBytes;
      }
      else if(arg == "--working-memory-bytes")
      {
        if(!NextValue(argc, argv, i, value))
        {
          fmt::print("--working-memory-bytes requires a positive integer byte count\n");
          return 1;
        }
        if(value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
        {
          throw std::invalid_argument("expected decimal digits only");
        }
        const uint64 parsedBytes = std::stoull(value);
        if(parsedBytes == 0)
        {
          fmt::print("--working-memory-bytes requires a positive integer byte count\n");
          return 1;
        }
        opts.workingMemoryBytes = parsedBytes;
      }
      else if(arg == "--ooc-only")
      {
        opts.oocOnly = true;
      }
      else if(arg == "--new-only")
      {
        opts.newOnly = true;
      }
      else if(arg == "--merge")
      {
        std::string second;
        if(!NextValue(argc, argv, i, value) || !NextValue(argc, argv, i, second))
        {
          fmt::print("--merge requires two CSV paths: --merge <incore.csv> <ooc.csv>\n");
          return 1;
        }
        opts.doMerge = true;
        opts.mergeIncoreCsv = value;
        opts.mergeOocCsv = second;
      }
      else if(arg == "--filters" && NextValue(argc, argv, i, value))
      {
        opts.filters = value;
      }
      else if(arg == "--require-new-result")
      {
        opts.requireNewResult = true;
      }
      else if(arg == "--out" && NextValue(argc, argv, i, value))
      {
        opts.outCsv = value;
      }
      else if(arg == "--help" || arg == "-h")
      {
        PrintUsage();
        return 0;
      }
      else
      {
        fmt::print("Unknown or malformed argument: {}\n", arg);
        PrintUsage();
        return 1;
      }
    } catch(const std::exception& ex)
    {
      fmt::print("Could not parse value for {}: {}\n", arg, ex.what());
      return 1;
    }
  }

  // --- Merge mode: pure CSV join (no plugins, no filter runs). Joins an in-core-build CSV and an OOC-build CSV on
  //     filter+type into --out, recomputing the ratio columns. ---
  if(opts.doMerge)
  {
    if(opts.outCsv.empty())
    {
      fmt::print("--merge requires --out <merged.csv>\n");
      return 1;
    }
    const Result<> mergeResult = ip_bench::MergeCsv(opts.mergeIncoreCsv, opts.mergeOocCsv, opts.outCsv);
    if(mergeResult.invalid())
    {
      fmt::print("Merge failed: {}\n", FirstErrorMessage(mergeResult.errors()));
      return 1;
    }
    fmt::print("Merged '{}' + '{}' -> '{}'\n", opts.mergeIncoreCsv, opts.mergeOocCsv, opts.outCsv);
    return 0;
  }

  if(opts.mode != "incore" && opts.mode != "ooc")
  {
    fmt::print("--mode must be 'incore' or 'ooc' (got '{}')\n", opts.mode);
    return 1;
  }
  if(opts.oocOnly && opts.mode != "ooc")
  {
    fmt::print("--ooc-only requires --mode ooc, but received mode '{}'.\n", opts.mode);
    return 1;
  }
  const DataStorageMode storeMode = (opts.mode == "ooc") ? DataStorageMode::ForceOutOfCore : DataStorageMode::ForceInCore;
  std::optional<ImageProcessing::ScopedWorkingMemoryTuningOverride> workingMemoryOverride;
  if(opts.workingMemoryBytes.has_value())
  {
    workingMemoryOverride.emplace(*opts.workingMemoryBytes);
  }

  // --- Load ALL plugins from the build's plugin directory (same dir as this exe). This resolves BOTH the new
  //     ImageProcessing filters and the legacy ITKImageProcessing filters through the FilterList, so Task 2's ITK
  //     timing column can be populated. ---
  auto app = Application::GetOrCreateInstance();
  auto& cacheBudgetManager = CacheMemoryBudgetManager::instance();
  const uint64 requestedCacheBudgetBytes = opts.cacheMemoryBudgetBytes.value_or(app->getPreferences()->cacheMemoryBudgetBytes());
  const bool cacheBudgetClamped = cacheBudgetManager.setBudgetBytes(requestedCacheBudgetBytes);
  if(cacheBudgetClamped)
  {
    fmt::print("WARNING: requested cache memory budget ({} bytes) exceeded this machine's safe limit and was reduced to {} bytes.\n", requestedCacheBudgetBytes, cacheBudgetManager.budgetBytes());
  }

  const Result<> loadResult = app->loadPlugins(SIMPLNX_BUILD_DIR, true);
  if(loadResult.invalid())
  {
    fmt::print("WARNING: one or more plugins failed to load from '{}': {}\n", std::string(SIMPLNX_BUILD_DIR), FirstErrorMessage(loadResult.errors()));
  }

#ifdef SIMPLNX_IMAGEPROCESSING_BENCH_HAS_OOC
  // Register the out-of-core backend + preference-driven resolver so `--mode ooc` produces disk-backed "HDF5-OOC"
  // stores. Mirrors SimplnxOoc's unit-test bootstrap: the ForceOutOfCore preference set below is INERT without BOTH
  // the registered manager AND the resolver that maps that preference to the "HDF5-OOC" format. Only compiled in the
  // OOC harness build; the standalone build has no backend, so its OOC column self-skips (as designed).
  SimplnxOoc::registerIOManager(app->getIOCollection());
  DataStructure::setDefaultFormatResolver(std::make_shared<SimplnxOoc::PreferenceFormatResolver>());
  fmt::print("OOC backend registered: yes\n");
#endif

  // Confirm a known legacy ITK filter UUID resolves (ITKAbsImageFilter). If not, Task 2's ITK column will be empty.
  const Uuid itkAbsUuid = Uuid::FromString("e9dd12bc-f7fa-4ba2-98b0-fec3326bf620").value();
  const bool itkLoaded = app->getFilterList()->createFilter(itkAbsUuid) != nullptr;
  fmt::print("ITK plugin loaded: {}\n", itkLoaded ? "yes" : "no");
  if(!itkLoaded)
  {
    fmt::print("WARNING: legacy ITKImageProcessing filters did not resolve; the ITK timing column will be empty.\n");
  }

  // --- Select filters. ---
  const auto& registry = ip_bench::GetBenchmarkFilterRegistry();
  std::vector<const ip_bench::BenchmarkFilterSpec*> selected;
  if(opts.filters == "subset" || opts.filters == "all")
  {
    for(const auto& spec : registry)
    {
      selected.push_back(&spec);
    }
  }
  else
  {
    std::unordered_set<std::string_view> requestedNames;
    usize selectionStart = 0;
    while(selectionStart <= opts.filters.size())
    {
      const usize separator = opts.filters.find(',', selectionStart);
      const std::string_view requestedName(opts.filters.data() + selectionStart, (separator == std::string::npos ? opts.filters.size() : separator) - selectionStart);
      if(!requestedNames.insert(requestedName).second)
      {
        fmt::print("Duplicate benchmark filter name '{}' in --filters. Each filter may be selected only once.\n", requestedName);
        return 1;
      }
      bool found = false;
      for(const auto& spec : registry)
      {
        if(spec.name == requestedName)
        {
          selected.push_back(&spec);
          found = true;
          break;
        }
      }
      if(!found)
      {
        fmt::print("No benchmark filter named '{}'. Available:\n", requestedName);
        for(const auto& spec : registry)
        {
          fmt::print("  {}\n", spec.name);
        }
        return 1;
      }
      if(separator == std::string::npos)
      {
        break;
      }
      selectionStart = separator + 1;
    }
  }

  const std::string projectionAxis = opts.projectionAxis.has_value() ? std::to_string(*opts.projectionAxis) : "default";
  const std::string workingMemoryBytes = opts.workingMemoryBytes.has_value() ? std::to_string(*opts.workingMemoryBytes) : "default";
  fmt::print("Config: mode={} target-voxels={} z-slices={} force-2d={} projection-axis={} warmup={} repeats={} cache-memory-budget-bytes={} maximum-working-memory-bytes={} working-memory-bytes={} "
             "ooc-only={} new-only={} filters={}\n",
             opts.mode, opts.targetVoxels, opts.zSlices, opts.force2D, projectionAxis, opts.warmup, opts.repeats, cacheBudgetManager.budgetBytes(), cacheBudgetManager.maximumWorkingMemoryBytes(),
             workingMemoryBytes, opts.oocOnly, opts.newOnly, opts.filters);

  // --- Run the three-config sweep (per-filter error isolation lives in RunFilterBenchmark). ---
  ip_bench::SweepOptions sweep;
  sweep.targetVoxels = opts.targetVoxels;
  sweep.minZSlices = opts.zSlices;
  sweep.force2D = opts.force2D;
  sweep.projectionAxis = opts.projectionAxis;
  sweep.warmup = opts.warmup;
  sweep.repeats = opts.repeats;
  sweep.oocRequested = (storeMode == DataStorageMode::ForceOutOfCore);
  sweep.oocOnly = opts.oocOnly;
  sweep.skipItkIncore = opts.newOnly;

  std::vector<ip_bench::BenchmarkRow> rows;
  try
  {
    rows = ip_bench::RunSweep(selected, sweep);
  } catch(const std::exception& ex)
  {
    fmt::print(stderr, "Benchmark sweep aborted: {}\n", ex.what());
    return 1;
  }

  // Always print the optimization-priority summary to the console.
  ip_bench::PrintSummary(rows);

  if(opts.requireNewResult)
  {
    bool missingResult = false;
    for(const ip_bench::BenchmarkRow& row : rows)
    {
      const bool hasResult = sweep.oocRequested ? row.newOocMs.has_value() : row.newIncoreMs.has_value();
      if(!hasResult)
      {
        fmt::print("Required new-filter result missing for '{}': {}\n", row.filter, row.notes.empty() ? "no diagnostic was recorded" : row.notes);
        missingResult = true;
      }
    }
    if(missingResult)
    {
#ifdef SIMPLNX_IMAGEPROCESSING_BENCH_HAS_OOC
      SimplnxOoc::shutdown();
#endif
      return 1;
    }
  }

  // Write the CSV only when --out is supplied.
  if(!opts.outCsv.empty())
  {
    const Result<> csvResult = ip_bench::WriteCsv(opts.outCsv, rows);
    if(csvResult.invalid())
    {
      fmt::print("\nFailed to write CSV '{}': {}\n", opts.outCsv, FirstErrorMessage(csvResult.errors()));
      return 1;
    }
    fmt::print("\nWrote CSV: {}\n", opts.outCsv);
  }

#ifdef SIMPLNX_IMAGEPROCESSING_BENCH_HAS_OOC
  SimplnxOoc::shutdown(); // flush dirty chunks + release OOC session files before exit
#endif

  return 0;
}
