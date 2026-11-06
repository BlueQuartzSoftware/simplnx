#include "AvizoWriterFailureTestUtilities.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/WriteAvizoRectilinearCoordinateFilter.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::WriteAvizoRectilinearCoordinateFilter: Valid Filter Execution", "[SimplnxCore][WriteAvizoRectilinearCoordinateFilter]")
{
  UnitTest::LoadPlugins();

  const std::string kDataInputArchive = "6_6_avizo_writers.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_avizo_writers";
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_avizo_writers/6_6_avizo_writers_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteAvizoRectilinearCoordinateFilter filter;
  DataStructure ds;
  Arguments args;

  fs::path exemplarOutputPath = fs::path(fmt::format("{}/6_6_avizo_writers/6_6_avizo_rectilinear_coordinate_writer.am", unit_test::k_TestFilesDir));
  fs::path computedOutputPath(fmt::format("{}/NX_AvisoRectilinearOutput.am", unit_test::k_BinaryTestOutputDir));
  fs::path exemplarBinaryOutputPath = fs::path(fmt::format("{}/6_6_avizo_writers/6_6_avizo_rectilinear_coordinate_writer_binary.am", unit_test::k_TestFilesDir));
  fs::path computedBinaryOutputPath(fmt::format("{}/NX_AvisoRectilinearOutput_binary.am", unit_test::k_BinaryTestOutputDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteAvizoRectilinearCoordinateFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedOutputPath));
  args.insertOrAssign(WriteAvizoRectilinearCoordinateFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteAvizoRectilinearCoordinateFilter::k_GeometryPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100})));
  args.insertOrAssign(WriteAvizoRectilinearCoordinateFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_EbsdScanData, k_FeatureIds})));
  args.insertOrAssign(WriteAvizoRectilinearCoordinateFilter::k_Units_Key, std::make_any<StringParameter::ValueType>("microns"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  args.insertOrAssign(WriteAvizoRectilinearCoordinateFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedBinaryOutputPath));
  args.insertOrAssign(WriteAvizoRectilinearCoordinateFilter::k_WriteBinaryFile_Key, std::make_any<bool>(true));

  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<size_t> linesToSkip{8, 9}; // skip the author & DateTime lines
  std::ifstream computedFile(computedOutputPath);
  std::ifstream exemplarFile(exemplarOutputPath);
  UnitTest::CompareAsciiFiles(computedFile, exemplarFile, linesToSkip);
  std::ifstream computedBinaryFile(computedBinaryOutputPath, std::ios::binary);
  std::ifstream exemplarBinaryFile(exemplarBinaryOutputPath, std::ios::binary);
  UnitTest::CompareAsciiFiles(computedBinaryFile, exemplarBinaryFile, linesToSkip);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAvizoRectilinearCoordinateFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteAvizoRectilinearCoordinateFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteAvizoRectilinearCoordinateFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteAvizoRectilinearCoordinateFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteAvizoRectilinearCoordinateFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteAvizoRectilinearCoordinateFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<bool>(WriteAvizoRectilinearCoordinateFilter::k_WriteBinaryFile_Key) == true);
      CHECK(args.value<DataPath>(WriteAvizoRectilinearCoordinateFilter::k_GeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteAvizoRectilinearCoordinateFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(WriteAvizoRectilinearCoordinateFilter::k_Units_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::WriteAvizoRectilinearCoordinateFilter: Geometry cell tuple count", "[SimplnxCore][WriteAvizoRectilinearCoordinateFilter][TupleCount]")
{
  UnitTest::LoadPlugins();
  using FilterType = WriteAvizoRectilinearCoordinateFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);

  // A matching input in another Attribute Matrix must remain valid.
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 1), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_GeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(FilterType::k_OutputFile_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "TupleCountWriteAvizoRectilinearCoordinate.txt"));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -5834);
    const DataPath expectedPath = inputPath;
    const usize expectedTuples = tuples;
    REQUIRE(errorRef.message.find(expectedPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", expectedTuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::WriteAvizoRectilinearCoordinateFilter: Source read failure preserves destination", "[SimplnxCore][WriteAvizoRectilinearCoordinateFilter][AvizoWriter]")
{
  UnitTest::LoadPlugins();
  const bool writeBinary = GENERATE(false, true);
  const bool existingDestination = GENERATE(false, true);
  const bool throwAllocation = GENERATE(false, true);
  DYNAMIC_SECTION("binary=" << writeBinary << ", existing destination=" << existingDestination << ", allocation failure=" << throwAllocation)
  {
    auto store = std::make_shared<UnitTest::AvizoWriterFailure::SecondBlockFailureStore<DataStore<int32>>>(ShapeType{1, 1, 65537}, ShapeType{1}, std::optional<int32>{7});
    UnitTest::AvizoWriterFailure::CheckSecondBlockFailure<WriteAvizoRectilinearCoordinateFilter>(writeBinary, existingDestination, store, throwAllocation);
  }
}
