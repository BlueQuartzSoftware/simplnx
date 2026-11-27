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
const fs::path k_ExemplarFilePath = fs::path(fmt::format("{}/LosAlamosFFTExemplar.txt", unit_test::k_TestFilesDir));
const fs::path k_WrittenFilePath = fs::path(fmt::format("{}/LosAlamosFFT.txt", unit_test::k_BinaryTestOutputDir));

constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkSliceTuples = k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkTotalTuples = k_BenchmarkDim * k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkEulerComponents = 3;
constexpr std::array<std::array<float32, k_BenchmarkEulerComponents>, 4> k_BenchmarkEulerAngles = {
    std::array<float32, k_BenchmarkEulerComponents>{0.0F, 0.0F, 0.0F},
    std::array<float32, k_BenchmarkEulerComponents>{constants::k_PiOver2F, constants::k_PiOver4F, constants::k_PiOver3F},
    std::array<float32, k_BenchmarkEulerComponents>{constants::k_PiF, 2.0F * constants::k_PiOver12F, constants::k_PiOver12F},
    std::array<float32, k_BenchmarkEulerComponents>{3.0F * constants::k_PiOver2F, constants::k_PiOver2F, constants::k_PiOver4F},
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

std::vector<char> ReadIn(const fs::path& filePath)
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
  REQUIRE(fs::exists(k_WrittenFilePath));
  REQUIRE(fs::exists(k_ExemplarFilePath));
  const std::vector<char> exemplar = ReadIn(k_ExemplarFilePath);
  const std::vector<char> data = ReadIn(k_WrittenFilePath);
  for(size_t i = 0; i < 1024; i++)
  {
    if(exemplar[i] != data[i])
    {
      std::cout << "Output difference at byte offset " << i << '\n';
      REQUIRE(exemplar[i] == data[i]);
      break;
    }
  }
  REQUIRE(exemplar.size() == data.size());
}
} // namespace

TEST_CASE("SimplnxCore::WriteLosAlamosFFTFilter: Valid Filter Execution", "[SimplnxCore][WriteLosAlamosFFTFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "bin_feature_phases.tar.gz", "bin_feature_phases");

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "LosAlamosFFTExemplar.tar.gz", "LosAlamosFFTExemplar.txt");

  // Reuse the compact binary feature-phases fixture.
  DataStructure dataStructure = unit_test::LoadDataStructure(fs::path(fmt::format("{}/bin_feature_phases/6_6_find_feature_phases_binary.dream3d", unit_test::k_TestFilesDir)));

  {
    // Configure the filter arguments.
    WriteLosAlamosFFTFilter filter;
    Arguments args;

    args.insertOrAssign(WriteLosAlamosFFTFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_WrittenFilePath));

    args.insertOrAssign(WriteLosAlamosFFTFilter::k_ImageGeomPath, std::make_any<DataPath>(DataPath({constants::k_SmallIN100})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({constants::k_SmallIN100, constants::k_EbsdScanData, constants::k_FeatureIds})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({constants::k_SmallIN100, constants::k_EbsdScanData, constants::k_EulerAngles})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({constants::k_SmallIN100, constants::k_EbsdScanData, "BinaryPhases"})));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ::CompareResults();

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteLosAlamosFFTFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteLosAlamosFFTFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteLosAlamosFFTFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteLosAlamosFFTFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<WriteLosAlamosFFTFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteLosAlamosFFTFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_ImageGeomPath) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteLosAlamosFFTFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

TEST_CASE("SimplnxCore::WriteLosAlamosFFTFilter: Geometry cell tuple count", "[SimplnxCore][WriteLosAlamosFFTFilter][TupleCount]")
{
  unit_test::LoadPlugins();
  using FilterType = WriteLosAlamosFFTFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Equally sized siblings expose the missing geometry check; separate short inputs cover every parameter.
  const auto counts = GENERATE((std::array<usize, 3>{0, 0, 0}), (std::array<usize, 3>{11, 11, 11}), (std::array<usize, 3>{12, 12, 12}), (std::array<usize, 3>{13, 13, 13}),
                               (std::array<usize, 3>{11, 12, 12}), (std::array<usize, 3>{12, 11, 12}), (std::array<usize, 3>{12, 12, 11}));
  const usize tuples = counts[0];
  CAPTURE(counts);

  // A matching input in another Attribute Matrix must remain valid.
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 1), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});
  auto* eulerAMPtr = AttributeMatrix::Create(dataStructure, "Euler Data", ShapeType{counts[1]});
  auto* phaseAMPtr = AttributeMatrix::Create(dataStructure, "Phase Data", ShapeType{counts[2]});
  REQUIRE(eulerAMPtr != nullptr);
  REQUIRE(phaseAMPtr != nullptr);
  REQUIRE(DataArray<float32>::Create(dataStructure, "EulerAngles", std::make_shared<DataStore<float32>>(ShapeType{counts[1]}, ShapeType{3}, 0), eulerAMPtr->getId()) != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Phases", std::make_shared<DataStore<int32>>(ShapeType{counts[2]}, ShapeType{1}, 1), phaseAMPtr->getId()) != nullptr);
  const DataPath eulerPath({"Euler Data", "EulerAngles"});
  const DataPath phasePath({"Phase Data", "Phases"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_ImageGeomPath, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(FilterType::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerPath));
  args.insertOrAssign(FilterType::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasePath));
  args.insertOrAssign(FilterType::k_OutputFile_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "TupleCountWriteLosAlamosFFT.txt"));

  const auto result = filter.preflight(dataStructure, args);
  if(counts[0] == 12 && counts[1] == 12 && counts[2] == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().size() == 1);
    const auto& errorRef = result.output_actions.errors()[0];
    REQUIRE(errorRef.code == -73461);
    const DataPath expectedPath = counts[0] != 12 ? inputPath : (counts[1] != 12 ? eulerPath : phasePath);
    const usize expectedTuples = counts[0] != 12 ? counts[0] : (counts[1] != 12 ? counts[1] : counts[2]);
    REQUIRE(errorRef.message.find(expectedPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", expectedTuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}
