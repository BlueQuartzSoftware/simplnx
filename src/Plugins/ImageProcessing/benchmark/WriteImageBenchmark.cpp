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

const Uuid k_WriteImageUuid = Uuid::FromString("a8b920c7-5445-4c8a-b7d7-6cabc578d587").value();
const Uuid k_ItkWriterUuid = Uuid::FromString("a181ee3e-1678-4133-b9c5-a9dd7bfec62f").value();

struct Options
{
  DataStorageMode storageMode = DataStorageMode::ForceInCore;
  int warmup = 1;
  int repeats = 3;
  fs::path outputRoot;
  fs::path csvPath;
  std::string cases = "direct-xy,direct-xz,direct-yz,color-tiff,color-png";
  bool includeItk = false;
  bool retainOutput = false;
};

struct OocCounters
{
  uint64 returnedExtentReads = 0;
  uint64 callerExtentReads = 0;
  uint64 chunkCacheMisses = 0;
  uint64 metadataBatches = 0;
};

struct OutputStats
{
  uint64 fileCount = 0;
  uint64 byteCount = 0;
};

struct TimingRow
{
  std::string mode;
  std::string writer;
  std::string caseName;
  std::string dataType;
  std::string dataFormat;
  std::string chunkShape;
  std::vector<double> samplesMs;
  double medianMs = 0.0;
  double minimumMs = 0.0;
  OutputStats output;
  OocCounters counterDelta;
};

class PreferencesSentinel
{
public:
  explicit PreferencesSentinel(DataStorageMode storageMode)
  : m_Preferences(Application::Instance()->getPreferences())
  , m_OriginalMode(m_Preferences->dataStorageMode())
  , m_OriginalLargeDataSize(m_Preferences->valueAs<int64>(Preferences::k_LargeDataSize_Key))
  {
    m_Preferences->setDataStorageMode(storageMode);
    m_Preferences->setValue(Preferences::k_LargeDataSize_Key, int64{0});
  }

  ~PreferencesSentinel()
  {
    m_Preferences->setDataStorageMode(m_OriginalMode);
    m_Preferences->setValue(Preferences::k_LargeDataSize_Key, m_OriginalLargeDataSize);
  }

  PreferencesSentinel(const PreferencesSentinel&) = delete;
  PreferencesSentinel(PreferencesSentinel&&) = delete;
  PreferencesSentinel& operator=(const PreferencesSentinel&) = delete;
  PreferencesSentinel& operator=(PreferencesSentinel&&) = delete;

private:
  Preferences* m_Preferences = nullptr;
  DataStorageMode m_OriginalMode = DataStorageMode::Adaptive;
  int64 m_OriginalLargeDataSize = 0;
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
             "  --include-itk         Also time the ITK writer for direct cases\n"
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
          options.storageMode = DataStorageMode::ForceInCore;
        }
        else if(value == "ooc")
        {
          options.storageMode = DataStorageMode::ForceOutOfCore;
        }
        else
        {
          return MakeErrorResult<Options>(-9300, fmt::format("Unknown storage mode '{}'. Expected 'incore' or 'ooc'.", value));
        }
      }
      else if(argument == "--output-root" && NextValue(argc, argv, index, value))
      {
        options.outputRoot = value;
      }
      else if(argument == "--csv" && NextValue(argc, argv, index, value))
      {
        options.csvPath = value;
      }
      else if(argument == "--cases" && NextValue(argc, argv, index, value))
      {
        options.cases = value;
      }
      else if(argument == "--include-itk")
      {
        options.includeItk = true;
      }
      else if(argument == "--retain-output")
      {
        options.retainOutput = true;
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

  if(options.outputRoot.empty() || options.csvPath.empty())
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
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomPath.getTargetName());
  if(imageGeom == nullptr)
  {
    return MakeErrorResult(-9310, fmt::format("Could not create benchmark Image Geometry '{}'.", k_ImageGeomPath.toString()));
  }
  imageGeom->setDimensions({k_DimX, k_DimY, k_DimZ});
  imageGeom->setSpacing({0.25F, 0.5F, 0.75F});
  imageGeom->setOrigin({1.0F, 2.0F, 3.0F});

  auto* cellData = AttributeMatrix::Create(dataStructure, k_CellDataPath.getTargetName(), {k_DimZ, k_DimY, k_DimX}, imageGeom->getId());
  if(cellData == nullptr)
  {
    return MakeErrorResult(-9311, fmt::format("Could not create benchmark Attribute Matrix '{}'.", k_CellDataPath.toString()));
  }
  imageGeom->setCellData(*cellData);

  std::shared_ptr<AbstractDataStore<T>> store;
  {
    PreferencesSentinel preferences(storageMode);
    store = DataStoreUtilities::CreateDataStore<T>(dataStructure, k_InputArrayPath, {k_DimZ, k_DimY, k_DimX}, {1});
  }
  if(store == nullptr)
  {
    return MakeErrorResult(-9312, fmt::format("Could not create the '{}' benchmark store for '{}'.", DataTypeToString(GetDataType<T>()), k_InputArrayPath.toString()));
  }
  if(DataArray<T>::Create(dataStructure, k_InputArrayPath.getTargetName(), store, cellData->getId()) == nullptr)
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
  if(const auto* oocStore = dynamic_cast<const SimplnxOoc::HDF5ChunkedStore<T>*>(&store); oocStore != nullptr)
  {
    return {oocStore->getReturnedExtentReadCount(), oocStore->getCallerProvidedExtentReadCount(), oocStore->getParallelChunkCacheMissCount(), oocStore->getParallelChunkMetadataBatchCount()};
  }
#endif
  return {};
}

OocCounters SubtractCounters(const OocCounters& after, const OocCounters& before)
{
  return {after.returnedExtentReads - before.returnedExtentReads, after.callerExtentReads - before.callerExtentReads, after.chunkCacheMisses - before.chunkCacheMisses,
          after.metadataBatches - before.metadataBatches};
}

template <typename T>
std::string ChunkShape(const AbstractDataStore<T>& store)
{
#ifdef SIMPLNX_WRITE_IMAGE_BENCH_HAS_OOC
  if(const auto* oocStore = dynamic_cast<const SimplnxOoc::HDF5ChunkedStore<T>*>(&store); oocStore != nullptr)
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
      ++stats.fileCount;
      stats.byteCount += entry.file_size();
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
  arguments.insertOrAssign("total_index_digits", std::make_any<Int32Parameter::ValueType>(4));
  arguments.insertOrAssign("leading_digit_character", std::make_any<StringParameter::ValueType>("0"));
  arguments.insertOrAssign("create_color_table", std::make_any<bool>(createColorTable));
  arguments.insertOrAssign("selected_preset", std::make_any<std::string>(ColorTableUtilities::GetDefaultRGBPresetName()));
  arguments.insertOrAssign("use_mask", std::make_any<bool>(false));
  arguments.insertOrAssign("add_scale_bar", std::make_any<bool>(false));
  return arguments;
}

template <typename T>
Result<TimingRow> RunCase(DataStructure& dataStructure, const Options& options, const Uuid& writerUuid, std::string writerName, std::string caseName, usize plane, std::string extension,
                          bool createColorTable)
{
  IFilter::UniquePointer filter = Application::Instance()->getFilterList()->createFilter(writerUuid);
  if(filter == nullptr)
  {
    return MakeErrorResult<TimingRow>(-9322, fmt::format("Could not create the '{}' writer with UUID '{}'.", writerName, writerUuid.str()));
  }

  const fs::path caseDirectory = options.outputRoot / fmt::format("{}-{}", writerName, caseName);
  if(Result<> resetResult = ResetOutputDirectory(caseDirectory); resetResult.invalid())
  {
    return ConvertResultTo<TimingRow>(std::move(resetResult), {});
  }
  Arguments arguments = CreateWriterArguments(*filter, plane, caseDirectory / ("slice" + extension), createColorTable);
  IFilter::PreflightResult preflightResult = filter->preflight(dataStructure, arguments);
  if(preflightResult.outputActions.invalid())
  {
    return MakeErrorResult<TimingRow>(-9323, fmt::format("The '{}' writer failed preflight for case '{}': {}", writerName, caseName,
                                                         preflightResult.outputActions.errors().empty() ? "no error message" : preflightResult.outputActions.errors().front().message));
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

  if(!options.retainOutput)
  {
    if(Result<> resetResult = ResetOutputDirectory(caseDirectory); resetResult.invalid())
    {
      return ConvertResultTo<TimingRow>(std::move(resetResult), {});
    }
  }

  std::vector<double> sorted = samples;
  std::sort(sorted.begin(), sorted.end());
  TimingRow row;
  row.mode = options.storageMode == DataStorageMode::ForceOutOfCore ? "ooc" : "incore";
  row.writer = std::move(writerName);
  row.caseName = std::move(caseName);
  row.dataType = std::string(DataTypeToString(GetDataType<T>()));
  row.dataFormat = store.getDataFormat().empty() ? "InMemory" : store.getDataFormat();
  row.chunkShape = ChunkShape(store);
  row.samplesMs = std::move(samples);
  row.medianMs = sorted[sorted.size() / 2];
  row.minimumMs = sorted.front();
  row.output = outputStats;
  row.counterDelta = SubtractCounters(after, before);
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
    output << row.mode << ',' << row.writer << ',' << row.caseName << ',' << row.dataType << ',' << k_DimX << 'x' << k_DimY << 'x' << k_DimZ << ',' << k_VoxelCount << ',' << warmup << ',' << repeats;
    for(usize index = 0; index < 3; ++index)
    {
      output << ',';
      if(index < row.samplesMs.size())
      {
        output << fmt::format("{:.3f}", row.samplesMs[index]);
      }
    }
    output << ',' << fmt::format("{:.3f}", row.medianMs) << ',' << fmt::format("{:.3f}", row.minimumMs) << ',' << row.output.fileCount << ',' << row.output.byteCount << ',' << row.dataFormat << ','
           << row.chunkShape << ',' << row.counterDelta.returnedExtentReads << ',' << row.counterDelta.callerExtentReads << ',' << row.counterDelta.chunkCacheMisses << ','
           << row.counterDelta.metadataBatches << '\n';
  }
  return {};
}

void PrintRow(const TimingRow& row)
{
  fmt::print("mode={} writer={} case={} type={} median_ms={:.3f} samples_ms=", row.mode, row.writer, row.caseName, row.dataType, row.medianMs);
  for(usize index = 0; index < row.samplesMs.size(); ++index)
  {
    fmt::print("{}{:.3f}", index == 0 ? "" : ";", row.samplesMs[index]);
  }
  fmt::print(" files={} bytes={} format={} chunk={} returned_extent_reads={} caller_extent_reads={} chunk_cache_misses={} metadata_batches={}\n", row.output.fileCount, row.output.byteCount,
             row.dataFormat, row.chunkShape.empty() ? "n/a" : row.chunkShape, row.counterDelta.returnedExtentReads, row.counterDelta.callerExtentReads, row.counterDelta.chunkCacheMisses,
             row.counterDelta.metadataBatches);
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
  if(Result<> buildResult = BuildInput<uint8>(dataStructure, options.storageMode); buildResult.invalid())
  {
    return buildResult;
  }
  const std::string mode = options.storageMode == DataStorageMode::ForceOutOfCore ? "OOC" : "in-core";
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
    if(options.includeItk)
    {
      if(Result<> result = AddCase<uint8>(rows, dataStructure, options, k_ItkWriterUuid, "itk", caseName, plane, ".tif", false); result.invalid())
      {
        return result;
      }
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
  if(Result<> buildResult = BuildInput<float32>(dataStructure, options.storageMode); buildResult.invalid())
  {
    return buildResult;
  }
  const std::string mode = options.storageMode == DataStorageMode::ForceOutOfCore ? "OOC" : "in-core";
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
  SimplnxOoc::registerIOManager(application->getIOCollection());
  DataStructure::setDefaultFormatResolver(std::make_shared<SimplnxOoc::PreferenceFormatResolver>());
#endif

  if(options.storageMode == DataStorageMode::ForceOutOfCore && !DataStoreUtilities::GetIOCollection().anyManagerFinalizesImport())
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
  Result<> csvResult = AppendCsv(options.csvPath, rows, options.warmup, options.repeats);
  if(csvResult.invalid())
  {
    fmt::print(stderr, "Could not write benchmark CSV: {}\n", FirstError(csvResult));
    return 1;
  }

#ifdef SIMPLNX_WRITE_IMAGE_BENCH_HAS_OOC
  SimplnxOoc::shutdown();
#endif
  return 0;
}
