#include "SimplnxCore/Filters/FindNeighborhoodsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace fs = std::filesystem;

namespace
{
const std::string k_Volumes("Volumes");
const std::string k_EquivalentDiameters("EquivalentDiameters");
const std::string k_Neighborhoods("Neighborhoods");
const std::string k_NeighborhoodList("NeighborhoodList");
const std::string k_NeighborhoodsNX("NeighborhoodsNX");
const std::string k_NeighborhoodListNX("NeighborhoodListNX");
} // namespace

TEST_CASE("SimplnxCore::FindNeighborhoods", "[SimplnxCore][FindNeighborhoods]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath(k_CellFeatureData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindNeighborhoodsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insert(FindNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(1.0F));
    args.insert(FindNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(FindNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters)));
    args.insert(FindNeighborhoodsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Phases)));
    args.insert(FindNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
    args.insert(FindNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsNX));
    args.insert(FindNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the k_Neighborhoods output array with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_Neighborhoods});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodsNX});
    UnitTest::CompareArrays<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  // Compare the k_NeighborhoodList output neighborlist with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodList});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodListNX});
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplarPath, calculatedPath);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighborhoods.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
