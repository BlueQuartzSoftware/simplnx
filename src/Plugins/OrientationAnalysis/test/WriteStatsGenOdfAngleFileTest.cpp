#include "simplnx/Common/Constants.hpp"
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

namespace
{
const DataPath k_AnalyticalEulersPath({"AnalyticalEulers"});
const DataPath k_AnalyticalPhasesPath({"AnalyticalPhases"});
const DataPath k_AnalyticalBoolMaskPath({"AnalyticalBoolMask"});
const DataPath k_AnalyticalUInt8MaskPath({"AnalyticalUInt8Mask"});

DataStructure CreateAnalyticalDataStructure()
{
  DataStructure dataStructure;

  auto eulersStore = data_store_utilities::CreateDataStore<float32>(dataStructure, k_AnalyticalEulersPath, {4}, {3});
  auto phasesStore = data_store_utilities::CreateDataStore<int32>(dataStructure, k_AnalyticalPhasesPath, {4}, {1});
  auto boolMaskStore = data_store_utilities::CreateDataStore<bool>(dataStructure, k_AnalyticalBoolMaskPath, {4}, {1});
  auto uint8MaskStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, k_AnalyticalUInt8MaskPath, {4}, {1});
  REQUIRE(eulersStore != nullptr);
  REQUIRE(phasesStore != nullptr);
  REQUIRE(boolMaskStore != nullptr);
  REQUIRE(uint8MaskStore != nullptr);

  auto* eulersPtr = Float32ArrayType::Create(dataStructure, k_AnalyticalEulersPath.getTargetName(), eulersStore);
  auto* phasesPtr = Int32ArrayType::Create(dataStructure, k_AnalyticalPhasesPath.getTargetName(), phasesStore);
  auto* boolMaskPtr = BoolArrayType::Create(dataStructure, k_AnalyticalBoolMaskPath.getTargetName(), boolMaskStore);
  auto* uint8MaskPtr = UInt8ArrayType::Create(dataStructure, k_AnalyticalUInt8MaskPath.getTargetName(), uint8MaskStore);
  REQUIRE(eulersPtr != nullptr);
  REQUIRE(phasesPtr != nullptr);
  REQUIRE(boolMaskPtr != nullptr);
  REQUIRE(uint8MaskPtr != nullptr);

  const std::array<float32, 12> eulerValues = {
      0.0F, constants::k_PiOver2F, constants::k_PiF, 0.25F, 0.5F, 0.75F, constants::k_PiOver2F, 0.0F, constants::k_PiOver2F, 1.0F, 1.25F, 1.5F,
  };
  const std::array<int32, 4> phaseValues = {1, 1, 2, 2};
  const std::array<bool, 4> maskValues = {true, false, true, false};
  for(usize valueIdx = 0; valueIdx < eulerValues.size(); valueIdx++)
  {
    (*eulersPtr)[valueIdx] = eulerValues[valueIdx];
  }
  for(usize tupleIdx = 0; tupleIdx < phaseValues.size(); tupleIdx++)
  {
    (*phasesPtr)[tupleIdx] = phaseValues[tupleIdx];
    (*boolMaskPtr)[tupleIdx] = maskValues[tupleIdx];
    (*uint8MaskPtr)[tupleIdx] = maskValues[tupleIdx] ? 1U : 0U;
  }

  return dataStructure;
}

std::vector<std::string> ReadLines(const fs::path& path)
{
  std::ifstream input(path);
  REQUIRE(input.is_open());

  std::vector<std::string> lines;
  std::string line;
  while(std::getline(input, line))
  {
    if(!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }
    lines.push_back(std::move(line));
  }
  return lines;
}

void RequireAnalyticalOdfFile(const fs::path& path, const std::string& expectedRow)
{
  const std::vector<std::string> lines = ReadLines(path);
  REQUIRE(lines.size() == 8);
  REQUIRE(lines[3] == "# Angle Data is Space delimited.");
  REQUIRE(lines[4] == "# Euler angles are expressed in degrees");
  REQUIRE(lines[5] == "# Euler0 Euler1 Euler2 Weight Sigma");
  REQUIRE(lines[6] == "Angle Count:1");
  REQUIRE(lines[7] == expectedRow);
}

} // namespace

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: Valid Filter Execution", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  unit_test::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "write_stats_gen_odf_angle_file.tar.gz", "write_stats_gen_odf_angle_file");

  auto exemplarFilePath = fs::path(fmt::format("{}/write_stats_gen_odf_angle_file/write_stats_gen_odf_angle_file.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);

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
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(constants::k_EulersArrayPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(constants::k_PhasesArrayPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/StatsGenODF_RadiansNoMask.txt", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(false));
  preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
  executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // The first two lines contain variable DREAM3D metadata.
  std::vector<size_t> linesToSkip{1, 2};
  std::ifstream computedFile1(computedOutput1Path);
  std::ifstream exemplarFile1(exemplarOutput1Path);
  unit_test::CompareAsciiFiles(computedFile1, exemplarFile1, linesToSkip);
  std::ifstream computedFile2(computedOutput2Path);
  std::ifstream exemplarFile2(exemplarOutput2Path);
  unit_test::CompareAsciiFiles(computedFile2, exemplarFile2, linesToSkip);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: analytical Bool and UInt8 masks write exact phase rows", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool useUInt8Mask = GENERATE(false, true);
  CAPTURE(scenario, useUInt8Mask);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateAnalyticalDataStructure();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_AnalyticalEulersPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(k_AnalyticalEulersPath));

  const std::string outputStem = useUInt8Mask ? "Task10dUInt8Mask" : "Task10dBoolMask";
  const fs::path outputFile = fs::path(fmt::format("{}/{}.txt", unit_test::k_BinaryTestOutputDir, outputStem));
  const DataPath maskPath = useUInt8Mask ? k_AnalyticalUInt8MaskPath : k_AnalyticalBoolMaskPath;

  WriteStatsGenOdfAngleFileFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Weight_Key, std::make_any<float32>(2.5F));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Sigma_Key, std::make_any<int32>(3));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Delimiter_Key, std::make_any<ChoicesParameter::ValueType>(WriteStatsGenOdfAngleFileFilter::k_SpaceDelimiter));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AnalyticalEulersPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_AnalyticalPhasesPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const fs::path outputDirectory = outputFile.parent_path();
  RequireAnalyticalOdfFile(outputDirectory / fmt::format("{}_Phase_1.txt", outputStem), "0.00000000 90.00000000 180.00000000 2.50000000 3");
  RequireAnalyticalOdfFile(outputDirectory / fmt::format("{}_Phase_2.txt", outputStem), "90.00000000 0.00000000 90.00000000 2.50000000 3");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: Colon delimiter writes colon separated rows", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateAnalyticalDataStructure();
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(k_AnalyticalEulersPath));

  const fs::path outputFile = fs::path(fmt::format("{}/StatsGenODF_Colon.txt", unit_test::k_BinaryTestOutputDir));
  WriteStatsGenOdfAngleFileFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Weight_Key, std::make_any<float32>(2.5F));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Sigma_Key, std::make_any<int32>(3));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Delimiter_Key, std::make_any<ChoicesParameter::ValueType>(WriteStatsGenOdfAngleFileFilter::k_ColonDelimiter));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AnalyticalEulersPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_AnalyticalPhasesPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_AnalyticalBoolMaskPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // The mask keeps (0, pi/2, pi) in phase 1 and (pi/2, 0, pi/2) in phase 2.
  // Multiplying by 180/pi gives (0, 90, 180) and (90, 0, 90) degrees.
  // Append the supplied weight 2.5 and sigma 3, separating all five values with ':'.
  const std::array<std::string, 2> expectedRows = {"0.00000000:90.00000000:180.00000000:2.50000000:3", "90.00000000:0.00000000:90.00000000:2.50000000:3"};
  for(usize phaseIndex = 0; phaseIndex < expectedRows.size(); phaseIndex++)
  {
    CAPTURE(phaseIndex);
    const auto lines = ReadLines(outputFile.parent_path() / fmt::format("StatsGenODF_Colon_Phase_{}.txt", phaseIndex + 1));
    REQUIRE(lines.size() == 8);
    CHECK(lines[3] == "# Angle Data is Colon delimited.");
    CHECK(lines[4] == "# Euler angles are expressed in degrees");
    CHECK(lines[6] == "Angle Count:1");
    CHECK(lines[7] == expectedRows[phaseIndex]);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: InValid Filter Execution", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "write_stats_gen_odf_angle_file.tar.gz", "write_stats_gen_odf_angle_file");

  auto exemplarFilePath = fs::path(fmt::format("{}/write_stats_gen_odf_angle_file/write_stats_gen_odf_angle_file.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  WriteStatsGenOdfAngleFileFilter filter;
  Arguments args;

  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/StatsGenODF.txt", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Weight_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Sigma_Key, std::make_any<int32>(1));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_Delimiter_Key, std::make_any<ChoicesParameter::ValueType>(WriteStatsGenOdfAngleFileFilter::k_SpaceDelimiter));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(constants::k_EulersArrayPath));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(constants::k_MaskArrayPath));

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
    auto* invalidPhaseArrayPtr = UInt32ArrayType::CreateWithStore<UInt32DataStoreType>(dataStructure, "Invalid_Phases_Array", std::vector<usize>{50, 50, 1}, std::vector<usize>{1});

    args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Invalid_Phases_Array"})));
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteStatsGenOdfAngleFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteStatsGenOdfAngleFileFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<WriteStatsGenOdfAngleFileFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
