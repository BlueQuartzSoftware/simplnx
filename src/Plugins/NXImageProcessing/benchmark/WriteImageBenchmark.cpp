#include "BenchmarkPreferenceSnapshot.hpp"
#include "NXImageProcessing/Filters/WriteImageFilter.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#ifdef SIMPLNX_WRITE_IMAGE_BENCH_HAS_OOC
#include "SimplnxOoc/HDF5ChunkedStore.hpp"
#include "SimplnxOoc/OocDataIOManager.hpp"
#include "SimplnxOoc/PreferenceFormatResolver.hpp"
#endif

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr usize k_DimX = 800;
constexpr usize k_DimY = 800;
constexpr usize k_DimZ = 400;
constexpr usize k_VoxelCount = k_DimX * k_DimY * k_DimZ;
constexpr usize k_FillBlockElements = 1024 * 1024;

const DataPath k_ImageGeomPath({"Image Geometry"});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath("Cell Data");
const DataPath k_InputArrayPath = k_CellDataPath.createChildPath("Input");

const Uuid k_WriteImageUuid = FilterTraits<WriteImageFilter>::uuid;

struct Options
{
  DataStorageMode storage_mode = DataStorageMode::ForceInCore;
  int warmup = 1;
  int repeats = 3;
  fs::path output_root;
  fs::path csv_path;
  std::string cases = "direct-xy,direct-xz,direct-yz,color-tiff,color-png";
  bool retain_output = false;
};

struct OocCounters
{
  uint64 returned_extent_reads = 0;
  uint64 caller_extent_reads = 0;
  uint64 chunk_cache_misses = 0;
  uint64 metadata_batches = 0;
};

struct OutputStats
{
  uint64 file_count = 0;
  uint64 byte_count = 0;
};

struct TimingRow
{
  std::string mode;
  std::string writer;
  std::string case_name;
  std::string data_type;
  std::string data_format;
  std::string chunk_shape;
  std::vector<double> samples_ms;
  double median_ms = 0.0;
  double minimum_ms = 0.0;
  OutputStats output;
  OocCounters counter_delta;
};

std::string FirstError(const Result<>& result)
{
  return result.errors().empty() ? "operation failed without an error message" : result.errors().front().message;
}

bool NextValue(int argc, char** argv, int& index, std::string& value)
{
  if(index + 1 >= argc)
  {
    return false;
  }
  value = argv[++index];
  return true;
}

void PrintUsage()
{
  fmt::print("Usage: WriteImageBenchmark --mode <incore|ooc> --output-root <path> --csv <path> [options]\n"
             "  --cases <comma-list>  direct-xy,direct-xz,direct-yz,color-tiff,color-png\n"
             "  --retain-output       Keep the files from the final iteration\n"
             "  --warmup <count>      Default: 1\n"
             "  --repeats <count>     Default: 3\n");
}

Result<Options> ParseOptions(int argc, char** argv)
{
  Options options;
  for(int index = 1; index < argc; ++index)
  {
    const std::string_view argument = argv[index];
    std::string value;
    try
    {
      if(argument == "--mode" && NextValue(argc, argv, index, value))
      {
        if(value == "incore")
        {
          options.storage_mode = DataStorageMode::ForceInCore;
        }
        else if(value == "ooc")
        {
          options.storage_mode = DataStorageMode::ForceOutOfCore;
        }
        else
        {
          return MakeErrorResult<Options>(-9300, fmt::format("Unknown storage mode '{}'. Expected 'incore' or 'ooc'.", value));
        }
      }
      else if(argument == "--output-root" && NextValue(argc, argv, index, value))
      {
        options.output_root = value;
      }
      else if(argument == "--csv" && NextValue(argc, argv, index, value))
      {
        options.csv_path = value;
      }
      else if(argument == "--cases" && NextValue(argc, argv, index, value))
      {
        options.cases = value;
      }
      else if(argument == "--retain-output")
      {
        options.retain_output = true;
      }
      else if(argument == "--warmup" && NextValue(argc, argv, index, value))
      {
        options.warmup = std::stoi(value);
      }
      else if(argument == "--repeats" && NextValue(argc, argv, index, value))
      {
        options.repeats = std::stoi(value);
      }
      else if(argument == "--help" || argument == "-h")
      {
        PrintUsage();
        return MakeErrorResult<Options>(-9301, "help requested");
      }
      else
      {
        return MakeErrorResult<Options>(-9302, fmt::format("Unknown or incomplete argument '{}'.", argument));
      }
    } catch(const std::exception& exception)
    {
      return MakeErrorResult<Options>(-9303, fmt::format("Could not parse argument '{}': {}", argument, exception.what()));
    }
  }

  if(options.output_root.empty() || options.csv_path.empty())
  {
    return MakeErrorResult<Options>(-9304, "Both --output-root and --csv are required.");
  }
  if(options.warmup < 0 || options.repeats <= 0)
  {
    return MakeErrorResult<Options>(-9305, fmt::format("Warmup must be nonnegative and repeats must be positive. Received warmup={} and repeats={}.", options.warmup, options.repeats));
  }
  return {std::move(options)};
}

std::vector<std::string> SplitCases(const std::string& cases)
{
  std::vector<std::string> result;
  usize begin = 0;
  while(begin <= cases.size())
  {
    const usize separator = cases.find(',', begin);
    result.emplace_back(cases.substr(begin, separator == std::string::npos ? std::string::npos : separator - begin));
    if(separator == std::string::npos)
    {
      break;
    }
    begin = separator + 1;
  }
  return result;
}

bool HasCase(const std::vector<std::string>& cases, std::string_view caseName)
{
  return std::find(cases.cbegin(), cases.cend(), caseName) != cases.cend();
}

template <typename T>
T DeterministicValue(usize index)
{
  if constexpr(std::is_same_v<T, uint8>)
  {
    return static_cast<uint8>((index * 131U + index / 17U) & 0xFFU);
  }
  else
  {
    return static_cast<float32>(index % 65521U) / 65520.0F;
  }
}

template <typename T>
Result<> BuildInput(DataStructure& dataStructure, DataStorageMode storageMode)
{
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeomPath.getTargetName());
  if(imageGeomPtr == nullptr)
  {
    return MakeErrorResult(-9310, fmt::format("Could not create benchmark Image Geometry '{}'.", k_ImageGeomPath.toString()));
  }
  imageGeomPtr->setDimensions({k_DimX, k_DimY, k_DimZ});
  if(auto result = imageGeomPtr->setSpacing({0.25F, 0.5F, 0.75F}); result.invalid())
  {
    return result;
  }
  if(auto result = imageGeomPtr->setOrigin({1.0F, 2.0F, 3.0F}); result.invalid())
  {
    return result;
  }

  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, k_CellDataPath.getTargetName(), {k_DimZ, k_DimY, k_DimX}, imageGeomPtr->getId());
  if(cellDataPtr == nullptr)
  {
    return MakeErrorResult(-9311, fmt::format("Could not create benchmark Attribute Matrix '{}'.", k_CellDataPath.toString()));
  }
  imageGeomPtr->setCellData(*cellDataPtr);

  std::shared_ptr<AbstractDataStore<T>> store;
  {
    ip_bench::BenchmarkPreferenceSnapshot preferences(*Application::Instance()->getPreferences(), storageMode);
    store = data_store_utilities::CreateDataStore<T>(dataStructure, k_InputArrayPath, {k_DimZ, k_DimY, k_DimX}, {1});
  }
  if(store == nullptr)
  {
    return MakeErrorResult(-9312, fmt::format("Could not create the '{}' benchmark store for '{}'.", DataTypeToString(GetDataType<T>()), k_InputArrayPath.toString()));
  }
  if(DataArray<T>::Create(dataStructure, k_InputArrayPath.getTargetName(), store, cellDataPtr->getId()) == nullptr)
  {
    return MakeErrorResult(-9313, fmt::format("Could not create benchmark Data Array '{}'.", k_InputArrayPath.toString()));
  }

  const bool isInMemory = store->getDataFormat().empty() || store->getDataFormat() == Preferences::k_InMemoryFormat.view();
  if(storageMode == DataStorageMode::ForceOutOfCore && isInMemory)
  {
    return MakeErrorResult(-9314, fmt::format("The OOC benchmark requested a disk-backed store, but '{}' uses format '{}'.", k_InputArrayPath.toString(), store->getDataFormat()));
  }
  if(storageMode == DataStorageMode::ForceInCore && !isInMemory)
  {
    return MakeErrorResult(-9315, fmt::format("The in-core benchmark requested memory storage, but '{}' uses format '{}'.", k_InputArrayPath.toString(), store->getDataFormat()));
  }

  std::vector<T> values(std::min(k_FillBlockElements, k_VoxelCount));
  for(usize offset = 0; offset < k_VoxelCount; offset += values.size())
  {
    const usize count = std::min(values.size(), k_VoxelCount - offset);
    for(usize index = 0; index < count; ++index)
    {
      values[index] = DeterministicValue<T>(offset + index);
    }
    if(Result<> result = store->copyFromBuffer(offset, nonstd::span<const T>(values.data(), count)); result.invalid())
    {
      return result;
    }
  }
  store->flush();
  return {};
}

template <typename T>
OocCounters ReadOocCounters(const AbstractDataStore<T>& store)
{
#ifdef SIMPLNX_WRITE_IMAGE_BENCH_HAS_OOC
  if(const auto* oocStore = dynamic_cast<const simplnx_ooc::HDF5ChunkedStore<T>*>(&store); oocStore != nullptr)
  {
    return {oocStore->getReturnedExtentReadCount(), oocStore->getCallerProvidedExtentReadCount(), oocStore->getParallelChunkCacheMissCount(), oocStore->getParallelChunkMetadataBatchCount()};
  }
#endif
  return {};
}

OocCounters SubtractCounters(const OocCounters& after, const OocCounters& before)
{
  return {.returned_extent_reads = after.returned_extent_reads - before.returned_extent_reads,
          .caller_extent_reads = after.caller_extent_reads - before.caller_extent_reads,
          .chunk_cache_misses = after.chunk_cache_misses - before.chunk_cache_misses,
          .metadata_batches = after.metadata_batches - before.metadata_batches};
}

template <typename T>
std::string ChunkShape(const AbstractDataStore<T>& store)
{
#ifdef SIMPLNX_WRITE_IMAGE_BENCH_HAS_OOC
  if(const auto* oocStore = dynamic_cast<const simplnx_ooc::HDF5ChunkedStore<T>*>(&store); oocStore != nullptr)
  {
    const std::optional<ShapeType> shapeResult = oocStore->getChunkShape();
    if(!shapeResult.has_value())
    {
      return {};
    }
    const ShapeType& shape = *shapeResult;
    std::string result;
    for(usize index = 0; index < shape.size(); ++index)
    {
      if(index != 0)
      {
        result += 'x';
      }
      result += std::to_string(shape[index]);
    }
    return result;
  }
#endif
  return {};
}

OutputStats MeasureOutput(const fs::path& directory)
{
  OutputStats stats;
  for(const fs::directory_entry& entry : fs::directory_iterator(directory))
  {
    if(entry.is_regular_file())
    {
      ++stats.file_count;
      stats.byte_count += entry.file_size();
    }
  }
  return stats;
}

Result<> ResetOutputDirectory(const fs::path& directory)
{
  std::error_code error;
  fs::remove_all(directory, error);
  if(error)
  {
    return MakeErrorResult(-9320, fmt::format("Could not remove benchmark output directory '{}': {}", directory.string(), error.message()));
  }
  fs::create_directories(directory, error);
  if(error)
  {
    return MakeErrorResult(-9321, fmt::format("Could not create benchmark output directory '{}': {}", directory.string(), error.message()));
  }
  return {};
}

Arguments CreateWriterArguments(IFilter& filter, usize plane, const fs::path& outputPath, bool createColorTable)
{
  Arguments arguments = filter.getDefaultArguments();
  arguments.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(k_ImageGeomPath));
  arguments.insertOrAssign("image_array_path", std::make_any<DataPath>(k_InputArrayPath));
  arguments.insertOrAssign("file_name", std::make_any<fs::path>(outputPath));
  arguments.insertOrAssign("index_offset", std::make_any<uint64>(0));
  arguments.insertOrAssign("plane_index", std::make_any<ChoicesParameter::ValueType>(plane));
  arguments.insertOrAssign("total_index_digits", std::make_any<Int32ParameterType::ValueType>(4));
  arguments.insertOrAssign("leading_digit_character", std::make_any<StringParameter::ValueType>("0"));
  arguments.insertOrAssign("create_color_table", std::make_any<bool>(createColorTable));
  arguments.insertOrAssign("selected_preset", std::make_any<std::string>(color_table_utilities::GetDefaultRGBPresetName()));
  arguments.insertOrAssign("use_mask", std::make_any<bool>(false));
  arguments.insertOrAssign("add_scale_bar", std::make_any<bool>(false));
  return arguments;
}

template <typename T>
Result<TimingRow> RunCase(DataStructure& dataStructure, const Options& options, const Uuid& writerUuid, std::string writerName, std::string caseName, usize plane, const std::string& extension,
                          bool createColorTable)
{
  IFilter::UniquePointerType filter = Application::Instance()->getFilterList()->createFilter(writerUuid);
  if(filter == nullptr)
  {
    return MakeErrorResult<TimingRow>(-9322, fmt::format("Could not create the '{}' writer with UUID '{}'.", writerName, writerUuid.str()));
  }

  const fs::path caseDirectory = options.output_root / fmt::format("{}-{}", writerName, caseName);
  if(Result<> resetResult = ResetOutputDirectory(caseDirectory); resetResult.invalid())
  {
    return ConvertResultTo<TimingRow>(std::move(resetResult), {});
  }
  Arguments arguments = CreateWriterArguments(*filter, plane, caseDirectory / ("slice" + extension), createColorTable);
  IFilter::PreflightResult preflightResult = filter->preflight(dataStructure, arguments);
  if(preflightResult.output_actions.invalid())
  {
    return MakeErrorResult<TimingRow>(-9323, fmt::format("The '{}' writer failed preflight for case '{}': {}", writerName, caseName,
                                                         preflightResult.output_actions.errors().empty() ? "no error message" : preflightResult.output_actions.errors().front().message));
  }

  const auto& store = dataStructure.getDataRefAs<DataArray<T>>(k_InputArrayPath).getDataStoreRef();
  const OocCounters before = ReadOocCounters(store);
  std::vector<double> samples;
  samples.reserve(static_cast<usize>(options.repeats));
  OutputStats outputStats;
  const int iterationCount = options.warmup + options.repeats;
  for(int iteration = 0; iteration < iterationCount; ++iteration)
  {
    if(Result<> resetResult = ResetOutputDirectory(caseDirectory); resetResult.invalid())
    {
      return ConvertResultTo<TimingRow>(std::move(resetResult), {});
    }
    const auto start = std::chrono::steady_clock::now();
    IFilter::ExecuteResult executeResult = filter->execute(dataStructure, arguments);
    const auto stop = std::chrono::steady_clock::now();
    if(executeResult.result.invalid())
    {
      return MakeErrorResult<TimingRow>(-9324, fmt::format("The '{}' writer failed execution for case '{}': {}", writerName, caseName, FirstError(executeResult.result)));
    }
    outputStats = MeasureOutput(caseDirectory);
    if(iteration >= options.warmup)
    {
      samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
  }
  const OocCounters after = ReadOocCounters(store);

  if(!options.retain_output)
  {
    if(Result<> resetResult = ResetOutputDirectory(caseDirectory); resetResult.invalid())
    {
      return ConvertResultTo<TimingRow>(std::move(resetResult), {});
    }
  }

  std::vector<double> sorted = samples;
  std::sort(sorted.begin(), sorted.end());
  TimingRow row;
  row.mode = options.storage_mode == DataStorageMode::ForceOutOfCore ? "ooc" : "incore";
  row.writer = std::move(writerName);
  row.case_name = std::move(caseName);
  row.data_type = std::string(DataTypeToString(GetDataType<T>()));
  row.data_format = store.getDataFormat().empty() ? "InMemory" : store.getDataFormat();
  row.chunk_shape = ChunkShape(store);
  row.samples_ms = std::move(samples);
  row.median_ms = sorted[sorted.size() / 2];
  row.minimum_ms = sorted.front();
  row.output = outputStats;
  row.counter_delta = SubtractCounters(after, before);
  return {std::move(row)};
}

Result<> AppendCsv(const fs::path& csvPath, const std::vector<TimingRow>& rows, int warmup, int repeats)
{
  std::error_code error;
  fs::create_directories(csvPath.parent_path(), error);
  if(error)
  {
    return MakeErrorResult(-9330, fmt::format("Could not create CSV directory '{}': {}", csvPath.parent_path().string(), error.message()));
  }
  const bool writeHeader = !fs::exists(csvPath) || fs::file_size(csvPath) == 0;
  std::ofstream output(csvPath, std::ios::app);
  if(!output.is_open())
  {
    return MakeErrorResult(-9331, fmt::format("Could not open benchmark CSV '{}'.", csvPath.string()));
  }
  if(writeHeader)
  {
    output << "mode,writer,case,type,dims,voxels,warmup,repeats,sample_1_ms,sample_2_ms,sample_3_ms,median_ms,min_ms,output_files,output_bytes,data_format,chunk_shape,returned_extent_reads,caller_"
              "extent_reads,chunk_cache_misses,metadata_batches\n";
  }
  for(const TimingRow& row : rows)
  {
    output << row.mode << ',' << row.writer << ',' << row.case_name << ',' << row.data_type << ',' << k_DimX << 'x' << k_DimY << 'x' << k_DimZ << ',' << k_VoxelCount << ',' << warmup << ','
           << repeats;
    for(usize index = 0; index < 3; ++index)
    {
      output << ',';
      if(index < row.samples_ms.size())
      {
        output << fmt::format("{:.3f}", row.samples_ms[index]);
      }
    }
    output << ',' << fmt::format("{:.3f}", row.median_ms) << ',' << fmt::format("{:.3f}", row.minimum_ms) << ',' << row.output.file_count << ',' << row.output.byte_count << ',' << row.data_format
           << ',' << row.chunk_shape << ',' << row.counter_delta.returned_extent_reads << ',' << row.counter_delta.caller_extent_reads << ',' << row.counter_delta.chunk_cache_misses << ','
           << row.counter_delta.metadata_batches << '\n';
  }
  return {};
}

void PrintRow(const TimingRow& row)
{
  fmt::print("mode={} writer={} case={} type={} median_ms={:.3f} samples_ms=", row.mode, row.writer, row.case_name, row.data_type, row.median_ms);
  for(usize index = 0; index < row.samples_ms.size(); ++index)
  {
    fmt::print("{}{:.3f}", index == 0 ? "" : ";", row.samples_ms[index]);
  }
  fmt::print(" files={} bytes={} format={} chunk={} returned_extent_reads={} caller_extent_reads={} chunk_cache_misses={} metadata_batches={}\n", row.output.file_count, row.output.byte_count,
             row.data_format, row.chunk_shape.empty() ? "n/a" : row.chunk_shape, row.counter_delta.returned_extent_reads, row.counter_delta.caller_extent_reads, row.counter_delta.chunk_cache_misses,
             row.counter_delta.metadata_batches);
}

template <typename T>
Result<> AddCase(std::vector<TimingRow>& rows, DataStructure& dataStructure, const Options& options, const Uuid& writerUuid, const std::string& writerName, const std::string& caseName, usize plane,
                 const std::string& extension, bool createColorTable)
{
  Result<TimingRow> result = RunCase<T>(dataStructure, options, writerUuid, writerName, caseName, plane, extension, createColorTable);
  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }
  PrintRow(result.value());
  rows.push_back(std::move(result.value()));
  return {};
}

Result<> RunDirectCases(const Options& options, const std::vector<std::string>& cases, std::vector<TimingRow>& rows)
{
  DataStructure dataStructure;
  if(Result<> buildResult = BuildInput<uint8>(dataStructure, options.storage_mode); buildResult.invalid())
  {
    return buildResult;
  }
  const std::string mode = options.storage_mode == DataStorageMode::ForceOutOfCore ? "OOC" : "in-core";
  fmt::print("Built {} uint8 input: {}x{}x{} = {} voxels, format={}\n", mode, k_DimX, k_DimY, k_DimZ, k_VoxelCount, dataStructure.getDataRefAs<DataArray<uint8>>(k_InputArrayPath).getDataFormat());

  const std::array<std::tuple<std::string, usize>, 3> directCases = {{{"direct-xy", 0}, {"direct-xz", 1}, {"direct-yz", 2}}};
  for(const auto& [caseName, plane] : directCases)
  {
    if(!HasCase(cases, caseName))
    {
      continue;
    }
    if(Result<> result = AddCase<uint8>(rows, dataStructure, options, k_WriteImageUuid, "new", caseName, plane, ".tif", false); result.invalid())
    {
      return result;
    }
  }
  return {};
}

Result<> RunColorCases(const Options& options, const std::vector<std::string>& cases, std::vector<TimingRow>& rows)
{
  if(!HasCase(cases, "color-tiff") && !HasCase(cases, "color-png"))
  {
    return {};
  }
  DataStructure dataStructure;
  if(Result<> buildResult = BuildInput<float32>(dataStructure, options.storage_mode); buildResult.invalid())
  {
    return buildResult;
  }
  const std::string mode = options.storage_mode == DataStorageMode::ForceOutOfCore ? "OOC" : "in-core";
  fmt::print("Built {} float32 input: {}x{}x{} = {} voxels, format={}\n", mode, k_DimX, k_DimY, k_DimZ, k_VoxelCount, dataStructure.getDataRefAs<DataArray<float32>>(k_InputArrayPath).getDataFormat());

  if(HasCase(cases, "color-tiff"))
  {
    if(Result<> result = AddCase<float32>(rows, dataStructure, options, k_WriteImageUuid, "new", "color-tiff", 0, ".tif", true); result.invalid())
    {
      return result;
    }
  }
  if(HasCase(cases, "color-png"))
  {
    if(Result<> result = AddCase<float32>(rows, dataStructure, options, k_WriteImageUuid, "new", "color-png", 0, ".png", true); result.invalid())
    {
      return result;
    }
  }
  return {};
}
} // namespace

int main(int argc, char** argv)
{
  Result<Options> optionsResult = ParseOptions(argc, argv);
  if(optionsResult.invalid())
  {
    if(!optionsResult.errors().empty() && optionsResult.errors().front().code == -9301)
    {
      return 0;
    }
    fmt::print(stderr, "{}\n", optionsResult.errors().empty() ? "Could not parse benchmark options." : optionsResult.errors().front().message);
    PrintUsage();
    return 1;
  }
  const Options options = std::move(optionsResult.value());

  auto application = Application::GetOrCreateInstance();
  Result<> loadResult = application->loadPlugins(SIMPLNX_BUILD_DIR, true);
  if(loadResult.invalid())
  {
    fmt::print(stderr, "Could not load benchmark plugins from '{}': {}\n", std::string(SIMPLNX_BUILD_DIR), FirstError(loadResult));
    return 1;
  }

#ifdef SIMPLNX_WRITE_IMAGE_BENCH_HAS_OOC
  simplnx_ooc::RegisterIoManager(application->getIOCollection());
  DataStructure::SetDefaultFormatResolver(std::make_shared<simplnx_ooc::PreferenceFormatResolver>());
#endif

  if(options.storage_mode == DataStorageMode::ForceOutOfCore && !data_store_utilities::GetIOCollection().anyManagerFinalizesImport())
  {
    fmt::print(stderr, "The OOC benchmark requires a registered disk-backed IO manager.\n");
    return 1;
  }

  const std::vector<std::string> cases = SplitCases(options.cases);
  std::vector<TimingRow> rows;
  Result<> directResult = RunDirectCases(options, cases, rows);
  if(directResult.invalid())
  {
    fmt::print(stderr, "Direct benchmark failed: {}\n", FirstError(directResult));
    return 1;
  }
  Result<> colorResult = RunColorCases(options, cases, rows);
  if(colorResult.invalid())
  {
    fmt::print(stderr, "Color benchmark failed: {}\n", FirstError(colorResult));
    return 1;
  }
  Result<> csvResult = AppendCsv(options.csv_path, rows, options.warmup, options.repeats);
  if(csvResult.invalid())
  {
    fmt::print(stderr, "Could not write benchmark CSV: {}\n", FirstError(csvResult));
    return 1;
  }

#ifdef SIMPLNX_WRITE_IMAGE_BENCH_HAS_OOC
  simplnx_ooc::Shutdown();
#endif
  return 0;
}
