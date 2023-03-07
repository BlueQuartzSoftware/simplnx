#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindNeighborhoodsFilter.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::Constants;

namespace fs = std::filesystem;

namespace
{
const std::string k_Volumes("Volumes");
const std::string k_EquivalentDiameters("EquivalentDiameters");
const std::string k_Neighborhoods("Neighborhoods");
const std::string k_NeighborhoodList("NeighborhoodList");
} // namespace

TEST_CASE("ComplexCore::FindNeighborhoods", "[ComplexCore][FindNeighborhoods]")
{
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
    args.insert(FindNeighborhoodsFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(FindNeighborhoodsFilter::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_EquivalentDiameters)));
    args.insert(FindNeighborhoodsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Phases)));
    args.insert(FindNeighborhoodsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataAM.createChildPath(k_Centroids)));
    // Make the output arrays at the top level of the data structure, out and away from the exemplar arrays
    args.insert(FindNeighborhoodsFilter::k_NeighborhoodsArrayName_Key, std::make_any<DataPath>({k_Neighborhoods}));
    args.insert(FindNeighborhoodsFilter::k_NeighborhoodListArrayName_Key, std::make_any<DataPath>({k_NeighborhoodList}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the k_Neighborhoods output array with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_Neighborhoods});
    const DataPath calculatedPath({k_Neighborhoods});
    UnitTest::CompareArrays<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  // Compare the k_NeighborhoodList output neighborlist with those precalculated from the file
  {
    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_NeighborhoodList});
    const DataPath calculatedPath({k_NeighborhoodList});
    UnitTest::CompareNeighborLists<int32>(dataStructure, exemplarPath, calculatedPath);
  }

  // Write the DataStructure out to the file system
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_neighborhoods.dream3d", unit_test::k_BinaryTestOutputDir)));
}
