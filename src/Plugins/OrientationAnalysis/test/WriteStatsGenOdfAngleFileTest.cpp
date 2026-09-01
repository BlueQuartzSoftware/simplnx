#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "OrientationAnalysis/Filters/WriteStatsGenOdfAngleFileFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nonstd/span.hpp>
#include <sstream>
#include <string_view>
namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: Valid Filter Execution", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  UnitTest::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "write_stats_gen_odf_angle_file.tar.gz", "write_stats_gen_odf_angle_file");

  auto exemplarFilePath = fs::path(fmt::format("{}/write_stats_gen_odf_angle_file/write_stats_gen_odf_angle_file.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  fs::path exemplarOutput1Path = fs::path(fmt::format("{}/write_stats_gen_odf_angle_file/StatsGenODF_Phase_1.txt", unit_test::k_TestFilesDir));
  fs::path exemplarOutput2Path = fs::path(fmt::format("{}/write_stats_gen_odf_angle_file/StatsGenODF_RadiansNoMask_Phase_1.txt", unit_test::k_TestFilesDir));
  fs::path computedOutput1Path(fmt::format("{}/StatsGenODF_Phase_1.txt", unit_test::k_BinaryTestOutputDir));
  fs::path computedOutput2Path(fmt::format("{}/StatsGenODF_RadiansNoMask_Phase_1.txt", unit_test::k_BinaryTestOutputDir));

  WriteStatsGenOdfAngleFileFilter filter;
  Arguments args;

  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/StatsGenODF.txt", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Weight_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Sigma_Key, std::make_any<int32>(1));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Delimiter_Key, std::make_any<ChoicesParameter::ValueType>(WriteStatsGenOdfAngleFileFilter::k_SpaceDelimiter));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(Constants::k_EulersArrayPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/StatsGenODF_RadiansNoMask.txt", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(false));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // The first two lines contain variable DREAM3D metadata.
  std::vector<size_t> linesToSkip{1, 2};
  std::ifstream computedFile1(computedOutput1Path);
  std::ifstream exemplarFile1(exemplarOutput1Path);
  UnitTest::CompareAsciiFiles(computedFile1, exemplarFile1, linesToSkip);
  std::ifstream computedFile2(computedOutput2Path);
  std::ifstream exemplarFile2(exemplarOutput2Path);
  UnitTest::CompareAsciiFiles(computedFile2, exemplarFile2, linesToSkip);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: InValid Filter Execution", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "write_stats_gen_odf_angle_file.tar.gz", "write_stats_gen_odf_angle_file");

  auto exemplarFilePath = fs::path(fmt::format("{}/write_stats_gen_odf_angle_file/write_stats_gen_odf_angle_file.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  WriteStatsGenOdfAngleFileFilter filter;
  Arguments args;

  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/StatsGenODF.txt", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Weight_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Sigma_Key, std::make_any<int32>(1));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Delimiter_Key, std::make_any<ChoicesParameter::ValueType>(WriteStatsGenOdfAngleFileFilter::k_SpaceDelimiter));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(Constants::k_EulersArrayPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));

  SECTION("default weight < 1")
  {
    args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Weight_Key, std::make_any<float32>(-1.0f));
  }

  SECTION("default sigma < 1")
  {
    args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Sigma_Key, std::make_any<int32>(0));
  }

  SECTION("input cell data arrays have mismatching tuples")
  {
    auto* invalidPhaseArray = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, "Invalid_Phases_Array", std::vector<usize>{50, 50, 1}, std::vector<usize>{1});

    args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Invalid_Phases_Array"})));
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteStatsGenOdfAngleFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteStatsGenOdfAngleFileFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteStatsGenOdfAngleFileFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<float32>(WriteStatsGenOdfAngleFileFilter::k_Weight_Key) == 2.5f);
      CHECK(args.value<int32>(WriteStatsGenOdfAngleFileFilter::k_Sigma_Key) == 5);
      CHECK(args.value<ChoicesParameter::ValueType>(WriteStatsGenOdfAngleFileFilter::k_Delimiter_Key) == 0);
      CHECK(args.value<bool>(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key) == true);
      CHECK(args.value<bool>(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(WriteStatsGenOdfAngleFileFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteStatsGenOdfAngleFileFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteStatsGenOdfAngleFileFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
