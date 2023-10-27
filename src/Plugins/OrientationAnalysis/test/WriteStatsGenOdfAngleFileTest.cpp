#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/WriteStatsGenOdfAngleFileFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <sstream>
namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: Valid Filter Execution", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "write_stats_gen_odf_angle_file.tar.gz",
                                                             "write_stats_gen_odf_angle_file");

  // Read Exemplar DREAM3D File
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
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_OutputFile_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/StatsGenODF_RadiansNoMask.txt", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_ConvertToDegrees_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteStatsGenOdfAngleFileFilter::k_UseMask_Key, std::make_any<bool>(false));
  preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  std::vector<size_t> linesToSkip{2};
  std::ifstream computedFile1(computedOutput1Path);
  std::ifstream exemplarFile1(exemplarOutput1Path);
  UnitTest::CompareAsciiFiles(computedFile1, exemplarFile1, linesToSkip);
  std::ifstream computedFile2(computedOutput2Path);
  std::ifstream exemplarFile2(exemplarOutput2Path);
  UnitTest::CompareAsciiFiles(computedFile2, exemplarFile2, linesToSkip);
}

TEST_CASE("OrientationAnalysis::WriteStatsGenOdfAngleFileFilter: InValid Filter Execution", "[OrientationAnalysis][WriteStatsGenOdfAngleFileFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "write_stats_gen_odf_angle_file.tar.gz",
                                                             "write_stats_gen_odf_angle_file");

  // Read Exemplar DREAM3D File
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
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
