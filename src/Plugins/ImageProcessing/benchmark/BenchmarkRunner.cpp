#include "BenchmarkRunner.hpp"

#include "BenchmarkTiling.hpp"

// Reused from the golden-test harness for the data-dir path resolver (ip_golden::InputPath).
#include "ItkGoldenTestUtils.hpp"

#include "simplnx/Common/TypesUtility.hpp" // DataTypeToString
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp" // GetIOCollection

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{
using namespace nx::core;
using ip_bench::BenchmarkRow;

std::string FirstErrorMessage(const std::vector<Error>& errors)
{
  return errors.empty() ? "(no message)" : errors.front().message;
}

// RAII sentinel that mirrors UnitTest::PreferencesSentinel (save mode + large_data_size, apply, restore on
// destruction). Re-implemented locally because this standalone tool deliberately does NOT link the UnitTestCommon
// .cpp that defines PreferencesSentinel's ctor/dtor (see benchmark/CMakeLists.txt). This one restores in-memory only
// -- it never writes the preferences file to disk.
class BenchPreferencesSentinel
{
public:
  BenchPreferencesSentinel(DataStorageMode mode, int64 largeDataSize)
  {
    auto* prefs = Application::Instance()->getPreferences();
    m_OriginalMode = prefs->dataStorageMode();
    m_OriginalSize = prefs->valueAs<int64>(Preferences::k_LargeDataSize_Key);
    prefs->setDataStorageMode(mode);
    prefs->setValue(Preferences::k_LargeDataSize_Key, largeDataSize);
  }
  ~BenchPreferencesSentinel()
  {
    auto* prefs = Application::Instance()->getPreferences();
    prefs->setDataStorageMode(m_OriginalMode);
    prefs->setValue(Preferences::k_LargeDataSize_Key, m_OriginalSize);
  }
  BenchPreferencesSentinel(const BenchPreferencesSentinel&) = delete;
  BenchPreferencesSentinel(BenchPreferencesSentinel&&) = delete;
  BenchPreferencesSentinel& operator=(const BenchPreferencesSentinel&) = delete;
  BenchPreferencesSentinel& operator=(BenchPreferencesSentinel&&) = delete;

private:
  DataStorageMode m_OriginalMode;
  int64 m_OriginalSize;
};

// Removes every DataObject NOT present in @p baseline (the set of IDs snapshotted right after the tiled input was
// built). Handles both in-place-output filters (an array under the shared cell AM) and new-geometry-output filters
// like MaximumProjection (a whole new ImageGeom). removeData recurses into children and is a no-op for IDs already
// gone, so removing a parent before a stale child ID is harmless. Cheap: a few map removals + freeing the output.
void CleanToBaseline(DataStructure& ds, const std::unordered_set<DataObject::IdType>& baseline)
{
  const std::vector<DataObject::IdType> ids = ds.getAllDataObjectIds(); // snapshot (removal mutates the structure)
  for(DataObject::IdType id : ids)
  {
    if(baseline.find(id) == baseline.end())
    {
      ds.removeData(id);
    }
  }
}

std::unordered_set<DataObject::IdType> SnapshotIds(const DataStructure& ds)
{
  const std::vector<DataObject::IdType> ids = ds.getAllDataObjectIds();
  return std::unordered_set<DataObject::IdType>(ids.begin(), ids.end());
}

Result<DataPath> PrepareBenchmarkInput(DataStructure& dataStructure, const ip_bench::BenchmarkFilterSpec& spec, const DataPath& geometryPath, const std::string& cellAttributeMatrixName,
                                       const std::string& inputArrayName, usize targetVoxels, usize minZSlices, bool force2D, DataStorageMode storeMode)
{
  ip_bench::BenchmarkInputContext context;
  context.realInputPath = ip_golden::InputPath(spec.realInputFile);
  context.geometryPath = geometryPath;
  context.cellAttributeMatrixName = cellAttributeMatrixName;
  context.inputArrayName = inputArrayName;
  context.targetVoxels = targetVoxels;
  context.minZSlices = minZSlices;
  context.force2D = force2D;
  context.storeMode = storeMode;

  if(spec.prepareInput)
  {
    return spec.prepareInput(dataStructure, context);
  }

  DataPath inputPath;
  Result<> buildResult =
      ip_bench::BuildTiledInput(dataStructure, context.realInputPath, geometryPath, cellAttributeMatrixName, inputArrayName, targetVoxels, minZSlices, storeMode, inputPath, force2D);
  return ConvertResultTo<DataPath>(std::move(buildResult), std::move(inputPath));
}

// Runs one (filter, config): resolves the filter by UUID, applies the spec's arg preset, does an un-timed preflight
// pre-check (to catch e.g. ITK key mismatch without aborting the sweep), then times execute() over warmup + repeats.
// Returns the median ms, or nullopt with @p note set to the skip/error reason. @p sentinel must already be active for
// the desired store mode.
std::optional<double> RunConfig(FilterList* filterList, const Uuid& uuid, DataStructure& ds, const std::unordered_set<DataObject::IdType>& baseline, const ip_bench::BenchmarkFilterSpec& spec,
                                const DataPath& geom, const DataPath& input, const std::string& outName, int warmup, int repeats, const std::optional<uint32>& projectionAxis, std::string& note)
{
  IFilter::UniquePointer filter = filterList->createFilter(uuid);
  if(filter == nullptr)
  {
    note = fmt::format("could not create filter {}", uuid.str());
    return std::nullopt;
  }

  Arguments args;
  spec.setArgs(args, geom, input, outName);
  if(spec.supportsProjectionAxisOverride && projectionAxis.has_value())
  {
    args.insertOrAssign("projection_dimension", std::make_any<uint32>(*projectionAxis));
  }

  // Start from a clean slate: a prior config may have left an output object of the same name behind.
  CleanToBaseline(ds, baseline);

  try
  {
    IFilter::PreflightResult pf = filter->preflight(ds, args);
    if(pf.outputActions.invalid())
    {
      note = "preflight failed: " + FirstErrorMessage(pf.outputActions.errors());
      return std::nullopt;
    }
  } catch(const std::exception& ex)
  {
    note = std::string("preflight threw: ") + ex.what();
    return std::nullopt;
  }

  try
  {
    // Each timed iteration clears the previous run's output back to the tiled baseline, then executes. RunTimed times
    // the whole callable, but the tiled-input rebuild is NOT here (built once, outside) and the cleanup is O(#outputs)
    // while the filter execute() over the full volume dominates.
    const ip_bench::Timing t = ip_bench::RunTimed(
        [&]() {
          CleanToBaseline(ds, baseline);
          const IFilter::ExecuteResult ex = filter->execute(ds, args);
          if(ex.result.invalid())
          {
            throw std::runtime_error(FirstErrorMessage(ex.result.errors()));
          }
        },
        warmup, repeats);
    return t.median_ms;
  } catch(const std::exception& ex)
  {
    note = std::string("execute failed: ") + ex.what();
    return std::nullopt;
  }
}

// --- CSV formatting helpers ---------------------------------------------------

std::string CsvEscape(const std::string& field)
{
  if(field.find_first_of(",\"\n\r") == std::string::npos)
  {
    return field;
  }
  std::string escaped = "\"";
  for(char c : field)
  {
    if(c == '"')
    {
      escaped += '"';
    }
    escaped += c;
  }
  escaped += '"';
  return escaped;
}

std::string MsCell(const std::optional<double>& ms)
{
  return ms.has_value() ? fmt::format("{:.3f}", *ms) : "n/a";
}

std::string RatioCell(const std::optional<double>& num, const std::optional<double>& den)
{
  if(num.has_value() && den.has_value() && *den > 0.0)
  {
    return fmt::format("{:.3f}", *num / *den);
  }
  return {}; // blank when either side is missing
}

// Splits one CSV line into fields, honoring double-quoted fields (with "" escaping) so quoted notes containing commas
// survive a merge round-trip.
std::vector<std::string> ParseCsvLine(const std::string& line)
{
  std::vector<std::string> fields;
  std::string cur;
  bool inQuotes = false;
  for(usize i = 0; i < line.size(); ++i)
  {
    const char c = line[i];
    if(inQuotes)
    {
      if(c == '"')
      {
        if(i + 1 < line.size() && line[i + 1] == '"')
        {
          cur += '"';
          ++i;
        }
        else
        {
          inQuotes = false;
        }
      }
      else
      {
        cur += c;
      }
    }
    else if(c == '"')
    {
      inQuotes = true;
    }
    else if(c == ',')
    {
      fields.push_back(cur);
      cur.clear();
    }
    else if(c != '\r')
    {
      cur += c;
    }
  }
  fields.push_back(cur);
  return fields;
}

std::optional<double> ParseMs(const std::string& cell)
{
  try
  {
    size_t consumed = 0;
    const double v = std::stod(cell, &consumed);
    if(consumed == 0)
    {
      return std::nullopt;
    }
    return v;
  } catch(const std::exception&)
  {
    return std::nullopt; // "n/a" / blank / non-numeric
  }
}

// A parsed CSV row keyed for merge (filter + type), carrying the ms cells needed to rebuild ratios.
struct ParsedRow
{
  std::string filter;
  std::string type;
  std::string inDims;
  std::string inVoxels;
  std::optional<double> itkIncoreMs;
  std::optional<double> newIncoreMs;
  std::optional<double> newOocMs;
  std::string notes;
};

Result<std::vector<ParsedRow>> ReadCsv(const std::string& path)
{
  std::ifstream in(path);
  if(!in.is_open())
  {
    return MakeErrorResult<std::vector<ParsedRow>>(-9210, fmt::format("Merge: could not open CSV '{}'", path));
  }
  std::string headerLine;
  if(!std::getline(in, headerLine))
  {
    return MakeErrorResult<std::vector<ParsedRow>>(-9211, fmt::format("Merge: CSV '{}' is empty", path));
  }
  const std::vector<std::string> header = ParseCsvLine(headerLine);
  std::unordered_map<std::string, usize> col;
  for(usize i = 0; i < header.size(); ++i)
  {
    col[header[i]] = i;
  }
  const std::vector<std::string> required = {"filter", "type", "itk_incore_ms", "new_incore_ms", "new_ooc_ms"};
  for(const auto& name : required)
  {
    if(col.find(name) == col.end())
    {
      return MakeErrorResult<std::vector<ParsedRow>>(-9212, fmt::format("Merge: CSV '{}' is missing the '{}' column", path, name));
    }
  }
  auto at = [&](const std::vector<std::string>& f, const std::string& name) -> std::string {
    const usize idx = col[name];
    return idx < f.size() ? f[idx] : std::string{};
  };

  std::vector<ParsedRow> rows;
  std::string line;
  while(std::getline(in, line))
  {
    if(line.empty())
    {
      continue;
    }
    const std::vector<std::string> f = ParseCsvLine(line);
    ParsedRow r;
    r.filter = at(f, "filter");
    r.type = at(f, "type");
    r.inDims = col.count("in_dims") ? at(f, "in_dims") : std::string{};
    r.inVoxels = col.count("in_voxels") ? at(f, "in_voxels") : std::string{};
    r.itkIncoreMs = ParseMs(at(f, "itk_incore_ms"));
    r.newIncoreMs = ParseMs(at(f, "new_incore_ms"));
    r.newOocMs = ParseMs(at(f, "new_ooc_ms"));
    r.notes = col.count("notes") ? at(f, "notes") : std::string{};
    rows.push_back(std::move(r));
  }
  return {std::move(rows)};
}
} // namespace

namespace ip_bench
{
Timing RunTimed(const std::function<void()>& exec, int warmup, int repeats)
{
  for(int i = 0; i < warmup; ++i)
  {
    exec();
  }

  std::vector<double> samples;
  samples.reserve(static_cast<usize>(std::max(0, repeats)));
  for(int i = 0; i < repeats; ++i)
  {
    const auto t0 = std::chrono::steady_clock::now();
    exec();
    const auto t1 = std::chrono::steady_clock::now();
    samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
  }

  Timing timing;
  timing.repeats = repeats;
  if(samples.empty())
  {
    return timing;
  }
  std::sort(samples.begin(), samples.end());
  const usize n = samples.size();
  timing.median_ms = (n % 2 == 1) ? samples[n / 2] : 0.5 * (samples[n / 2 - 1] + samples[n / 2]);
  timing.min_ms = samples.front();
  return timing;
}

BenchmarkRow RunFilterBenchmark(const BenchmarkFilterSpec& spec, const SweepOptions& opts)
{
  BenchmarkRow row;
  row.filter = spec.name;

  auto* filterList = Application::Instance()->getFilterList();

  const DataPath geom({"Image Geometry"});
  const std::string cellAmName = "CellData";
  const std::string inArrayName = "Input";
  const std::string outName = "BenchmarkOutput";

  auto addNote = [&](const std::string& s) {
    if(!row.notes.empty())
    {
      row.notes += "; ";
    }
    row.notes += s;
  };

  // --- Build the shared IN-CORE tiled input ONCE (new-in-core + ITK-in-core reuse it). ---
  DataStructure dsInCore;
  DataPath inputPath;
  {
    const BenchPreferencesSentinel sentinel(DataStorageMode::ForceInCore, 0);
    Result<DataPath> r = PrepareBenchmarkInput(dsInCore, spec, geom, cellAmName, inArrayName, opts.targetVoxels, opts.minZSlices, opts.force2D, DataStorageMode::ForceInCore);
    if(r.invalid())
    {
      addNote("in-core input build failed: " + FirstErrorMessage(r.errors()));
      return row; // Without an input no config can run; the row still lands in the CSV.
    }
    inputPath = std::move(r.value());
  }

  const auto& geomRef = dsInCore.getDataRefAs<ImageGeom>(geom);
  const SizeVec3 dims = geomRef.getDimensions();
  row.inDims = fmt::format("{}x{}x{}", dims[0], dims[1], dims[2]);
  row.inVoxels = static_cast<uint64>(dims[0]) * static_cast<uint64>(dims[1]) * static_cast<uint64>(dims[2]);
  row.type = std::string(DataTypeToString(dsInCore.getDataRefAs<IDataArray>(inputPath).getDataType()).view());

  const std::unordered_set<DataObject::IdType> baseInCore = SnapshotIds(dsInCore);

  if(!opts.oocOnly)
  {
    // --- new-in-core ---
    {
      const BenchPreferencesSentinel sentinel(DataStorageMode::ForceInCore, 0);
      std::string note;
      row.newIncoreMs = RunConfig(filterList, spec.newFilterUuid, dsInCore, baseInCore, spec, geom, inputPath, outName, opts.warmup, opts.repeats, opts.projectionAxis, note);
      if(!note.empty())
      {
        addNote("new-incore " + note);
      }
    }

    // --- ITK-in-core (drive the LEGACY filter with the SAME arg preset -- new+legacy ImageProcessing filters share
    //     parameter-key strings, exactly as the [ItkGolden]/parity tests rely on). ---
    if(opts.skipItkIncore)
    {
      addNote("ITK skipped: --new-only");
    }
    else if(spec.legacyItkUuid.has_value() && filterList->createFilter(*spec.legacyItkUuid) != nullptr)
    {
      const BenchPreferencesSentinel sentinel(DataStorageMode::ForceInCore, 0);
      std::string note;
      row.itkIncoreMs = RunConfig(filterList, *spec.legacyItkUuid, dsInCore, baseInCore, spec, geom, inputPath, outName, opts.warmup, opts.repeats, opts.projectionAxis, note);
      if(!note.empty())
      {
        addNote("itk-incore " + note);
      }
    }
    else
    {
      addNote(spec.legacyItkUuid.has_value() ? "ITK filter unregistered (n/a)" : "ITK filter disabled (n/a)");
    }
  }

  // --- new-OOC: ONLY when the CLI requested it AND an out-of-core backend is actually loaded (self-skip; never
  //     silently claim a number). ---
  if(opts.oocRequested && DataStoreUtilities::GetIOCollection().anyManagerFinalizesImport())
  {
    DataStructure dsOoc;
    DataPath oocInput;
    bool built = false;
    {
      const BenchPreferencesSentinel sentinel(DataStorageMode::ForceOutOfCore, opts.oocThreshold);
      Result<DataPath> r = PrepareBenchmarkInput(dsOoc, spec, geom, cellAmName, inArrayName, opts.targetVoxels, opts.minZSlices, opts.force2D, DataStorageMode::ForceOutOfCore);
      if(r.invalid())
      {
        addNote("ooc input build failed: " + FirstErrorMessage(r.errors()));
      }
      else
      {
        oocInput = std::move(r.value());
        built = true;
      }
    }
    if(built)
    {
      const std::unordered_set<DataObject::IdType> baseOoc = SnapshotIds(dsOoc);
      const BenchPreferencesSentinel sentinel(DataStorageMode::ForceOutOfCore, opts.oocThreshold);
      std::string note;
      row.newOocMs = RunConfig(filterList, spec.newFilterUuid, dsOoc, baseOoc, spec, geom, oocInput, outName, opts.warmup, opts.repeats, opts.projectionAxis, note);
      if(!note.empty())
      {
        addNote("new-ooc " + note);
      }
    }
  }
  else if(opts.oocRequested)
  {
    addNote("OOC skipped: no out-of-core backend registered");
  }
  else
  {
    addNote("OOC skipped: --mode incore");
  }

  return row;
}

std::vector<BenchmarkRow> RunSweep(const std::vector<const BenchmarkFilterSpec*>& specs, const SweepOptions& opts)
{
  std::vector<BenchmarkRow> rows;
  rows.reserve(specs.size());
  for(const auto* spec : specs)
  {
    fmt::print("=== {} ===\n", spec->name);
    std::fflush(stdout);
    BenchmarkRow row;
    try
    {
      // RunFilterBenchmark already isolates per-config failures; this guard defends against any unexpected throw so
      // one bad filter can never abort the whole sweep.
      row = RunFilterBenchmark(*spec, opts);
    } catch(const std::exception& ex)
    {
      row.filter = spec->name;
      row.notes = std::string("unhandled exception: ") + ex.what();
    }
    if(opts.oocRequested)
    {
      // RunFilterBenchmark owns all per-filter DataStructures, so their stores are gone when it returns. Clear
      // backend caches now so chunks belonging to those dead stores cannot consume memory or bias later sweep rows.
      try
      {
        DataStoreUtilities::GetIOCollection().shutdownManagers();
      } catch(const std::exception& ex)
      {
        throw std::runtime_error(fmt::format("OOC backend cache cleanup failed after benchmark filter '{}': {}", spec->name, ex.what()));
      } catch(...)
      {
        throw std::runtime_error(fmt::format("OOC backend cache cleanup failed after benchmark filter '{}' with an unknown exception", spec->name));
      }
    }
    fmt::print("{:<32} type={:<8} dims={:<16} itk={:>10} new={:>10} ooc={:>10}{}\n", row.filter, row.type.empty() ? "?" : row.type, row.inDims.empty() ? "?" : row.inDims, MsCell(row.itkIncoreMs),
               MsCell(row.newIncoreMs), MsCell(row.newOocMs), row.notes.empty() ? "" : ("  [" + row.notes + "]"));
    std::fflush(stdout);
    rows.push_back(std::move(row));
  }
  return rows;
}

Result<> WriteCsv(const std::string& path, const std::vector<BenchmarkRow>& rows)
{
  std::ofstream out(path, std::ios::trunc);
  if(!out.is_open())
  {
    return MakeErrorResult(-9200, fmt::format("Could not open CSV output '{}'", path));
  }
  out << "filter,type,in_dims,in_voxels,itk_incore_ms,new_incore_ms,new_ooc_ms,new_vs_itk,ooc_vs_incore,notes\n";
  for(const auto& r : rows)
  {
    out << CsvEscape(r.filter) << ',' << CsvEscape(r.type) << ',' << CsvEscape(r.inDims) << ',' << r.inVoxels << ',' << MsCell(r.itkIncoreMs) << ',' << MsCell(r.newIncoreMs) << ','
        << MsCell(r.newOocMs) << ',' << RatioCell(r.newIncoreMs, r.itkIncoreMs) << ',' << RatioCell(r.newOocMs, r.newIncoreMs) << ',' << CsvEscape(r.notes) << '\n';
  }
  if(out.fail())
  {
    return MakeErrorResult(-9201, fmt::format("Error while writing CSV output '{}'", path));
  }
  return {};
}

void PrintSummary(const std::vector<BenchmarkRow>& rows)
{
  // --- new-in-core vs ITK-in-core, worst (slowest relative to ITK) first: the optimization priority list. ---
  fmt::print("\n=== SUMMARY: new-in-core vs ITK-in-core (worst first) ===\n");
  std::vector<const BenchmarkRow*> byItk;
  for(const auto& r : rows)
  {
    if(r.newIncoreMs.has_value() && r.itkIncoreMs.has_value() && *r.itkIncoreMs > 0.0)
    {
      byItk.push_back(&r);
    }
  }
  std::sort(byItk.begin(), byItk.end(), [](const BenchmarkRow* a, const BenchmarkRow* b) { return (*a->newIncoreMs / *a->itkIncoreMs) > (*b->newIncoreMs / *b->itkIncoreMs); });
  for(const BenchmarkRow* r : byItk)
  {
    fmt::print("  {:<32} new/itk={:6.3f}   (new={:.2f} ms, itk={:.2f} ms)\n", r->filter, *r->newIncoreMs / *r->itkIncoreMs, *r->newIncoreMs, *r->itkIncoreMs);
  }
  for(const auto& r : rows)
  {
    if(!(r.newIncoreMs.has_value() && r.itkIncoreMs.has_value() && *r.itkIncoreMs > 0.0))
    {
      fmt::print("  {:<32} new/itk=   n/a   ({})\n", r.filter, r.notes.empty() ? "no ITK baseline" : r.notes);
    }
  }

  // --- new-OOC vs new-in-core, worst (largest OOC overhead) first. ---
  fmt::print("\n=== SUMMARY: new-OOC vs new-in-core (worst first) ===\n");
  std::vector<const BenchmarkRow*> byOoc;
  for(const auto& r : rows)
  {
    if(r.newOocMs.has_value() && r.newIncoreMs.has_value() && *r.newIncoreMs > 0.0)
    {
      byOoc.push_back(&r);
    }
  }
  if(byOoc.empty())
  {
    fmt::print("  (no OOC timings -- run with --mode ooc on a build with an out-of-core backend)\n");
  }
  std::sort(byOoc.begin(), byOoc.end(), [](const BenchmarkRow* a, const BenchmarkRow* b) { return (*a->newOocMs / *a->newIncoreMs) > (*b->newOocMs / *b->newIncoreMs); });
  for(const BenchmarkRow* r : byOoc)
  {
    fmt::print("  {:<32} ooc/incore={:6.3f}   (ooc={:.2f} ms, incore={:.2f} ms)\n", r->filter, *r->newOocMs / *r->newIncoreMs, *r->newOocMs, *r->newIncoreMs);
  }
}

Result<> MergeCsv(const std::string& incoreCsv, const std::string& oocCsv, const std::string& outPath)
{
  Result<std::vector<ParsedRow>> incoreResult = ReadCsv(incoreCsv);
  if(incoreResult.invalid())
  {
    return ConvertResult(std::move(incoreResult));
  }
  Result<std::vector<ParsedRow>> oocResult = ReadCsv(oocCsv);
  if(oocResult.invalid())
  {
    return ConvertResult(std::move(oocResult));
  }
  const std::vector<ParsedRow> incoreRows = incoreResult.value();
  const std::vector<ParsedRow> oocRows = oocResult.value();

  auto key = [](const ParsedRow& r) { return r.filter + "\x1f" + r.type; };

  std::unordered_map<std::string, const ParsedRow*> oocByKey;
  for(const ParsedRow& r : oocRows)
  {
    oocByKey[key(r)] = &r;
  }

  std::vector<BenchmarkRow> merged;
  merged.reserve(incoreRows.size());
  for(const ParsedRow& in : incoreRows)
  {
    BenchmarkRow row;
    row.filter = in.filter;
    row.type = in.type;
    row.inDims = in.inDims;
    row.inVoxels = 0;
    try
    {
      row.inVoxels = in.inVoxels.empty() ? 0ULL : std::stoull(in.inVoxels);
    } catch(const std::exception&)
    {
      row.inVoxels = 0;
    }
    row.itkIncoreMs = in.itkIncoreMs;
    row.newIncoreMs = in.newIncoreMs;
    std::string notes = in.notes;

    const auto it = oocByKey.find(key(in));
    if(it != oocByKey.end())
    {
      row.newOocMs = it->second->newOocMs;
      if(!it->second->notes.empty())
      {
        notes = notes.empty() ? it->second->notes : (notes + "; " + it->second->notes);
      }
    }
    else
    {
      notes = notes.empty() ? "no OOC row matched on filter+type" : (notes + "; no OOC row matched on filter+type");
    }
    row.notes = notes;
    merged.push_back(std::move(row));
  }

  return WriteCsv(outPath, merged);
}
} // namespace ip_bench
