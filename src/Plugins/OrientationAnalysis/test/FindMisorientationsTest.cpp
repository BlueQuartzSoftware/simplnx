#include "OrientationAnalysis/Filters/FindMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{
const std::string k_MisorientationListArrayName_Exemplar("MisorientationList");
const std::string k_MisorientationListArrayName("CalculatedMisorientationList");
const std::string k_NeighborListArrayName("NeighborList");
} // namespace

TEST_CASE("OrientationAnalysis::FindMisorientationsFilter", "[OrientationAnalysis][FindMisorientationsFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData);
  DataPath cellFeatureDataPath({k_DataContainer, k_CellFeatureData});
  DataPath neighborLstDataPath = cellFeatureDataPath.createChildPath(k_NeighborListArrayName);
  DataPath avgQuatsDataPath = cellFeatureDataPath.createChildPath(k_AvgQuats);
  DataPath featurePhasesDataPath = cellFeatureDataPath.createChildPath(k_Phases);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindMisorientationsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(neighborLstDataPath));
    args.insertOrAssign(FindMisorientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsDataPath));
    args.insertOrAssign(FindMisorientationsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesDataPath));
    args.insertOrAssign(FindMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(FindMisorientationsFilter::k_MisorientationListArrayName_Key, std::make_any<std::string>(k_MisorientationListArrayName));

    args.insertOrAssign(FindMisorientationsFilter::k_FindAvgMisors_Key, std::make_any<bool>(false));
    // args.insertOrAssign(FindMisorientationsFilter::k_AvgMisorientationsArrayName_Key, std::make_any<std::string>("")); //use default value

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the k_MisorientationList output with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_MisorientationListArrayName_Exemplar});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_MisorientationListArrayName});
    UnitTest::CompareNeighborLists<float>(dataStructure, exemplarPath, calculatedPath);
  }

// Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_misorientations.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

// TODO: needs to be implemented. This will need the input .dream3d file to be regenerated with the missing data generated using DREAM3D 6.6
TEST_CASE("OrientationAnalysis::FindMisorientationsFilter: Misorientation Per Feature", "[OrientationAnalysis][FindMisorientations][.][UNIMPLEMENTED][!mayfail]")
{
}
