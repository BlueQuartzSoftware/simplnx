#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/WriteLosAlamosFFTFilter.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const fs::path exemplarFilePath = fs::path(fmt::format("{}/LosAlamosFFTExemplar.txt", unit_test::k_TestFilesDir));
const fs::path writtenFilePath = fs::path(fmt::format("{}/LosAlamosFFT.txt", unit_test::k_BinaryTestOutputDir));

constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkSliceTuples = k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkTotalTuples = k_BenchmarkDim * k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkEulerComponents = 3;
constexpr std::array<std::array<float32, k_BenchmarkEulerComponents>, 4> k_BenchmarkEulerAngles = {
    std::array<float32, k_BenchmarkEulerComponents>{0.0F, 0.0F, 0.0F},
    std::array<float32, k_BenchmarkEulerComponents>{Constants::k_PiOver2F, Constants::k_PiOver4F, Constants::k_PiOver3F},
    std::array<float32, k_BenchmarkEulerComponents>{Constants::k_PiF, 2.0F * Constants::k_PiOver12F, Constants::k_PiOver12F},
    std::array<float32, k_BenchmarkEulerComponents>{3.0F * Constants::k_PiOver2F, Constants::k_PiOver2F, Constants::k_PiOver4F},
};
constexpr std::array<std::array<std::string_view, k_BenchmarkEulerComponents>, 4> k_BenchmarkEulerRecordValues = {
    std::array<std::string_view, k_BenchmarkEulerComponents>{"0.000", "0.000", "0.000"},
    std::array<std::string_view, k_BenchmarkEulerComponents>{"90.000", "45.000", "60.000"},
    std::array<std::string_view, k_BenchmarkEulerComponents>{"180.000", "30.000", "15.000"},
    std::array<std::string_view, k_BenchmarkEulerComponents>{"270.000", "90.000", "45.000"},
};

const std::string k_BenchmarkGeometryName = "Los Alamos FFT Benchmark Image Geometry";
const std::string k_BenchmarkCellDataName = "Cell Data";
const DataPath k_BenchmarkGeometryPath({k_BenchmarkGeometryName});
const DataPath k_BenchmarkCellDataPath = k_BenchmarkGeometryPath.createChildPath(k_BenchmarkCellDataName);
const DataPath k_BenchmarkFeatureIdsPath = k_BenchmarkCellDataPath.createChildPath("Feature Ids");
const DataPath k_BenchmarkEulerAnglesPath = k_BenchmarkCellDataPath.createChildPath("Euler Angles");
const DataPath k_BenchmarkPhasesPath = k_BenchmarkCellDataPath.createChildPath("Phases");

struct ScopedTempFile
{
  explicit ScopedTempFile(fs::path filePath)
  : path(std::move(filePath))
  {
    std::error_code errorCode;
    fs::remove(path, errorCode);
  }

  ~ScopedTempFile()
  {
    std::error_code errorCode;
    fs::remove(path, errorCode);
  }

  fs::path path;
};

const std::array<float32, k_BenchmarkEulerComponents>& ExpectedEulerAngles(usize tupleIndex)
{
  return k_BenchmarkEulerAngles[tupleIndex % k_BenchmarkEulerAngles.size()];
}

int32 ExpectedFeatureId(usize tupleIndex)
{
  return static_cast<int32>((tupleIndex % 97) + 1);
}

int32 ExpectedPhase(usize tupleIndex)
{
  return static_cast<int32>(((tupleIndex * 3) % 7) + 1);
}

std::string ExpectedRecord(usize tupleIndex)
{
  const usize x = tupleIndex % k_BenchmarkDim;
  const usize y = (tupleIndex / k_BenchmarkDim) % k_BenchmarkDim;
  const usize z = tupleIndex / k_BenchmarkSliceTuples;
  const auto& eulerValues = k_BenchmarkEulerRecordValues[tupleIndex % k_BenchmarkEulerRecordValues.size()];
  return fmt::format("{} {} {} {} {} {} {} {}", eulerValues[0], eulerValues[1], eulerValues[2], x + 1, y + 1, z + 1, ExpectedFeatureId(tupleIndex), ExpectedPhase(tupleIndex));
}

std::vector<char> readIn(fs::path filePath)
{
  std::ifstream file(filePath.string(), std::ios_base::binary);

  if(file)
  {
    // Read the output file size.
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read the complete file into a byte vector.
    std::vector<char> contents(length); // act as a buffer
    file.read(contents.data(), length);

    // Build a string from the byte buffer.
    return contents;
  }
  return {};
}

void CompareResults() // compare hash of both file strings
{
  REQUIRE(fs::exists(writtenFilePath));
  REQUIRE(fs::exists(exemplarFilePath));
  const std::vector<char> exemplar = readIn(exemplarFilePath);
  const std::vector<char> data = readIn(writtenFilePath);
  for(size_t i = 0; i < 1024; i++)
  {
    if(exemplar[i] != data[i])
    {
      std::cout << "Output difference at byte offset " << i << std::endl;
      REQUIRE(exemplar[i] == data[i]);
      break;
    }
  }
  REQUIRE(exemplar.size() == data.size());
}
} // namespace

TEST_CASE("SimplnxCore::WriteLosAlamosFFTFilter: Valid Filter Execution", "[SimplnxCore][WriteLosAlamosFFTFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "bin_feature_phases.tar.gz", "bin_feature_phases");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "LosAlamosFFTExemplar.tar.gz", "LosAlamosFFTExemplar.txt");

  // Reuse the compact binary feature-phases fixture.
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/bin_feature_phases/6_6_find_feature_phases_binary.dream3d", unit_test::k_TestFilesDir)));

  {
    // Configure the filter arguments.
    WriteLosAlamosFFTFilter filter;
    Arguments args;

    args.insertOrAssign(WriteLosAlamosFFTFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(writtenFilePath));

    args.insertOrAssign(WriteLosAlamosFFTFilter::k_ImageGeomPath, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BinaryPhases"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ::CompareResults();

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteLosAlamosFFTFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteLosAlamosFFTFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteLosAlamosFFTFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteLosAlamosFFTFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<WriteLosAlamosFFTFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteLosAlamosFFTFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_ImageGeomPath) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
