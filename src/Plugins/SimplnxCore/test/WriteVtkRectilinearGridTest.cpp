#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/WriteVtkRectilinearGridFilter.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::WriteVtkRectilinearGridFilter: Valid Filter Execution", "[SimplnxCore][WriteVtkRectilinearGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "vtk_rectilinear_grid_writer.tar.gz", "vtk_rectilinear_grid_writer");
  // Read input DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, and an Arguments Object
  WriteVtkRectilinearGridFilter filter;
  Arguments args;

  fs::path exemplarOutputPath = fs::path(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid.vtk", unit_test::k_TestFilesDir));
  fs::path computedOutputPath(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid.vtk", unit_test::k_BinaryTestOutputDir));
  fs::path exemplarBinaryOutputPath = fs::path(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid_binary.vtk", unit_test::k_TestFilesDir));
  fs::path computedBinaryOutputPath(fmt::format("{}/vtk_rectilinear_grid_writer/vtk_rectilinear_grid_binary.vtk", unit_test::k_BinaryTestOutputDir));

  MultiArraySelectionParameter::ValueType selectedArrayPaths = {k_ConfidenceIndexArrayPath, k_EulersArrayPath, k_FitArrayPath, k_ImageQualityArrayPath, k_PhasesArrayPath, k_SEMSignalArrayPath};

  // write out ascii vtk file
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedOutputPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedArrayPaths));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // write out binary vtk file
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedBinaryOutputPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key, std::make_any<bool>(true));

  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<size_t> linesToSkip{1}; // skip the version line
  std::ifstream computedFile(computedOutputPath);
  std::ifstream exemplarFile(exemplarOutputPath);
  UnitTest::CompareAsciiFiles(computedFile, exemplarFile, linesToSkip);
  std::ifstream computedBinaryFile(computedBinaryOutputPath, std::ios::binary);
  std::ifstream exemplarBinaryFile(exemplarBinaryOutputPath, std::ios::binary);
  UnitTest::CompareAsciiFiles(computedBinaryFile, exemplarBinaryFile, linesToSkip);

  // Remove the test files since they can get quite large.
  std::error_code errorCode;
  std::filesystem::remove_all(fmt::format("{}/vtk_rectilinear_grid_writer", unit_test::k_BinaryTestOutputDir), errorCode);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::WriteVtkRectilinearGridFilter: InValid Filter Execution", "[SimplnxCore][WriteVtkRectilinearGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Read input DREAM3D File
  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, and an Arguments Object
  WriteVtkRectilinearGridFilter filter;
  Arguments args;

  fs::path computedOutputPath(fmt::format("{}/NX_vtk_rectilinear_grid.vtk", unit_test::k_BinaryTestOutputDir));

  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedOutputPath));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteVtkRectilinearGridFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));

  SECTION("No Selected Arrays")
  {
    args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  }
  SECTION("Mismatching Tuples in Selected Arrays")
  {
    args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                            k_CrystalStructuresArrayPath, k_ConfidenceIndexArrayPath, k_EulersArrayPath, k_FitArrayPath, k_ImageQualityArrayPath, k_PhasesArrayPath, k_SEMSignalArrayPath}));
  }
  SECTION("Selected Arrays are not Cell Level Arrays")
  {
    args.insertOrAssign(WriteVtkRectilinearGridFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_CrystalStructuresArrayPath}));
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::WriteVtkRectilinearGridFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteVtkRectilinearGridFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteVtkRectilinearGridFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteVtkRectilinearGridFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteVtkRectilinearGridFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteVtkRectilinearGridFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<bool>(WriteVtkRectilinearGridFilter::k_WriteBinaryFile_Key) == true);
      // Complex type (DataContainerFromMultiSelectionFilterParameterConverter) - verified by successful pipeline loading
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
