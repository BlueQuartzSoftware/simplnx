
#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/FindEuclideanDistMapFilter.hpp"

using namespace complex;
using namespace complex::Constants;

TEST_CASE("Core::FindEuclideanDistMap", "[Core][FindEuclideanDistMap]")
{
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_stats_test.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath("CellFeatureData");

  const std::string k_CalculatedPrefix("Calculated_");
  const std::string k_GBDistancesArrayName("GBManhattanDistances");
  const std::string k_TJDistancesArrayName("TJManhattanDistances");
  const std::string k_QPDistancesArrayName("QPManhattanDistances");
  const std::string k_NearestNeighborsArrayName("NearestNeighbors");
  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    FindEuclideanDistMapFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insert(FindEuclideanDistMapFilter::k_CalcManhattanDist_Key, std::make_any<bool>(true));
    args.insert(FindEuclideanDistMapFilter::k_DoBoundaries_Key, std::make_any<bool>(true));
    args.insert(FindEuclideanDistMapFilter::k_DoTripleLines_Key, std::make_any<bool>(true));
    args.insert(FindEuclideanDistMapFilter::k_DoQuadPoints_Key, std::make_any<bool>(true));
    args.insert(FindEuclideanDistMapFilter::k_SaveNearestNeighbors_Key, std::make_any<bool>(true));
    // Input Arrays
    args.insert(FindEuclideanDistMapFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));

    args.insert(FindEuclideanDistMapFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAttributeMatrix.createChildPath(k_FeatureIds)));
    // Output Arrays
    args.insert(FindEuclideanDistMapFilter::k_GBDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_GBDistancesArrayName));
    args.insert(FindEuclideanDistMapFilter::k_TJDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_TJDistancesArrayName));
    args.insert(FindEuclideanDistMapFilter::k_QPDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_QPDistancesArrayName));
    args.insert(FindEuclideanDistMapFilter::k_NearestNeighborsArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_NearestNeighborsArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    std::vector<std::string> comparisonNames = {k_GBDistancesArrayName, k_TJDistancesArrayName, k_QPDistancesArrayName, k_NearestNeighborsArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellData, k_CalculatedPrefix + comparisonName});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);
    }
  }

  // Write the DataStructure out to the file system
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_euclidean_dist_map.dream3d", unit_test::k_BinaryTestOutputDir)));
}
