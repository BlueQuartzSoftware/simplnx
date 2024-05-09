#include "OrientationAnalysis/Filters/ComputeSchmidsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_SchmidsArrayName("Schmids");
const std::string k_SlipSystemsArrayName("SlipSystems");
const std::string k_PolesArrayName("Poles");
const std::string k_PhisArrayName("Schmid_Phis");
const std::string k_LambdasArrayName("Schmid_Lambdas");
const std::string k_CalculatedArrayPrefix("Calculated_");
} // namespace

TEST_CASE("OrientationAnalysis::ComputeSchmidsFilter", "[OrientationAnalysis][ComputeSchmidsFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);
  DataPath cellFeatureDataPath({k_DataContainer, k_CellFeatureData});
  DataPath avgQuatsDataPath = cellFeatureDataPath.createChildPath(k_AvgQuats);
  DataPath featurePhasesDataPath = cellFeatureDataPath.createChildPath(k_Phases);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeSchmidsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeSchmidsFilter::k_LoadingDirection_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 1.0F, 1.0F}));
    args.insertOrAssign(ComputeSchmidsFilter::k_StoreAngleComponents_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSchmidsFilter::k_OverrideSystem_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeSchmidsFilter::k_SlipPlane_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.0F, 0.0F, 1.0F}));
    args.insertOrAssign(ComputeSchmidsFilter::k_SlipDirection_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 0.0F, 0.0F}));

    args.insertOrAssign(ComputeSchmidsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesDataPath));
    args.insertOrAssign(ComputeSchmidsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsDataPath));
    args.insertOrAssign(ComputeSchmidsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(ComputeSchmidsFilter::k_SchmidsArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_SchmidsArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_SlipSystemsArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_SlipSystemsArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_PolesArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_PolesArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_PhisArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_PhisArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_LambdasArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_LambdasArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the float output arrays with those precalculated from the file
  {
    std::vector<std::string> comparisonNames = {k_SchmidsArrayName, k_PhisArrayName, k_LambdasArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_CalculatedArrayPrefix + comparisonName});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData, 1);
    }

    comparisonNames = {k_SlipSystemsArrayName, k_PolesArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_CalculatedArrayPrefix + comparisonName});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData, 1);
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_schmids.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
