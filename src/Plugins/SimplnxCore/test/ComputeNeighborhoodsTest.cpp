#include "SimplnxCore/Filters/ComputeNeighborhoodsFilter.hpp"
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

const std::string k_Neighborhoods_1("Neighborhoods_1");
const std::string k_NeighborhoodList_1("NeighborhoodList_1");

const std::string k_Neighborhoods_3("Neighborhoods_3");
const std::string k_NeighborhoodList_3("NeighborhoodList_3");

const std::string k_NeighborhoodsNX("computed neighborhood");
const std::string k_NeighborhoodListNX("computed neighborhood list");
} // namespace

TEST_CASE("SimplnxCore::ComputeNeighborhoods_1", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "compute_feature_neighborhoods.tar.gz",
                                                              "compute_feature_neighborhoods");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_neighborhoods/compute_feature_neighborhoods.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath(k_CellFeatureData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeNeighborhoodsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insert(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(1.0F));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters)));
    args.insert(ComputeNeighborhoodsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Phases)));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsNX));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the k_Neighborhoods output array with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_Neighborhoods_1});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodsNX});
    UnitTest::CompareArrays<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  // Compare the k_NeighborhoodList output neighbor list with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodList_1});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodListNX});
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplarPath, calculatedPath);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighborhoods.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeNeighborhoods_3", "[SimplnxCore][ComputeNeighborhoods]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "compute_feature_neighborhoods.tar.gz",
                                                              "compute_feature_neighborhoods");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_neighborhoods/compute_feature_neighborhoods.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath(k_CellFeatureData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeNeighborhoodsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insert(ComputeNeighborhoodsFilter::k_MultiplesOfAverage_Key, std::make_any<float32>(3.0F));
    args.insert(ComputeNeighborhoodsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(ComputeNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters)));
    args.insert(ComputeNeighborhoodsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Phases)));
    args.insert(ComputeNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<std::string>(k_NeighborhoodsNX));
    args.insert(ComputeNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<std::string>(k_NeighborhoodListNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the k_Neighborhoods output array with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_Neighborhoods_3});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodsNX});
    UnitTest::CompareArrays<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  // Compare the k_NeighborhoodList output neighbor list with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodList_3});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodListNX});
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplarPath, calculatedPath);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighborhoods.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
