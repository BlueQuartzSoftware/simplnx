#include "SimplnxCore/Filters/ComputeEuclideanDistMapFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_CalculatedPrefix = "Calculated_";
const std::string k_GBDistancesArrayName = "GBManhattanDistances";
const std::string k_TJDistancesArrayName = "TJManhattanDistances";
const std::string k_QPDistancesArrayName = "QPManhattanDistances";
const std::string k_NearestNeighborsArrayName = "NearestNeighbors";

bool ArrayExists(const DataStructure& dataStructure, const std::string& name)
{
  const DataPath calculatedPath({k_DataContainer, k_CellData, std::string(k_CalculatedPrefix) + name});
  return dataStructure.getDataAs<IDataArray>(calculatedPath) != nullptr;
};
} // namespace

TEST_CASE("SimplnxCore::ComputeEuclideanDistMap", "[SimplnxCore][ComputeEuclideanDistMap]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  const DataPath k_CellFeatureDataAM = k_DataContainerPath.createChildPath("CellFeatureData");

  // Run a matrix of scenarios. In each scenario exactly one calculated output is expected to exist and match its exemplar.
  auto [scenarioName, doBoundaries, doTripleLines, doQuadPoints, nnCompNum, expectedArrayName] =
      GENERATE(std::make_tuple("Boundaries only (GB distances)", true, false, false, 0, k_GBDistancesArrayName),
               std::make_tuple("Triple lines only (TJ distances)", false, true, false, 1, k_TJDistancesArrayName),
               std::make_tuple("Quad points only (QP distances)", false, false, true, 2, k_QPDistancesArrayName));

  INFO("Scenario: " << scenarioName);

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    ComputeEuclideanDistMapFilter filter;
    Arguments args;

    // Parameters
    args.insert(ComputeEuclideanDistMapFilter::k_CalcManhattanDist_Key, std::make_any<bool>(true));
    args.insert(ComputeEuclideanDistMapFilter::k_DoBoundaries_Key, std::make_any<bool>(doBoundaries));
    args.insert(ComputeEuclideanDistMapFilter::k_DoTripleLines_Key, std::make_any<bool>(doTripleLines));
    args.insert(ComputeEuclideanDistMapFilter::k_DoQuadPoints_Key, std::make_any<bool>(doQuadPoints));
    args.insert(ComputeEuclideanDistMapFilter::k_SaveNearestNeighbors_Key, std::make_any<bool>(true));

    // Input Arrays
    args.insert(ComputeEuclideanDistMapFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(ComputeEuclideanDistMapFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAttributeMatrix.createChildPath(k_FeatureIds)));

    // Output Arrays
    args.insert(ComputeEuclideanDistMapFilter::k_GBDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_GBDistancesArrayName));
    args.insert(ComputeEuclideanDistMapFilter::k_TJDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_TJDistancesArrayName));
    args.insert(ComputeEuclideanDistMapFilter::k_QPDistancesArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_QPDistancesArrayName));
    args.insert(ComputeEuclideanDistMapFilter::k_NearestNeighborsArrayName_Key, std::make_any<std::string>(k_CalculatedPrefix + k_NearestNeighborsArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Check that the appropriate array exists and the others don't exist
  for(const auto& outputName : {k_GBDistancesArrayName, k_TJDistancesArrayName, k_QPDistancesArrayName})
  {
    const bool shouldExist = (outputName == expectedArrayName);
    INFO("  Output: " << outputName << " (expected " << (shouldExist ? "present" : "absent") << ")");

    if(shouldExist)
    {
      REQUIRE(ArrayExists(dataStructure, outputName));
    }
    else
    {
      REQUIRE_FALSE(ArrayExists(dataStructure, outputName));
    }
  }

  // Check that the currently enabled array matches its exemplar
  const DataPath exemplarPath({k_DataContainer, k_CellData, expectedArrayName});
  const DataPath calculatedPath({k_DataContainer, k_CellData, k_CalculatedPrefix + expectedArrayName});
  const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
  const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
  UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData);

  // Check that the nearest neighbors array matches its exemplar
  // The nearest neighbors array has 3 components that are filled out based on which of the three distance arrays are enabled.
  // Component 0 is filled out if boundaries are enabled, component 1 is filled out if triple lines are enabled,
  // component 2 is filled out if quad points are enabled.  These component values are set to -1 otherwise.
  // So, we are going to compare the proper component values based on the currently enabled array
  REQUIRE(ArrayExists(dataStructure, k_NearestNeighborsArrayName));
  const DataPath exemplarNNPath({k_DataContainer, k_CellData, k_NearestNeighborsArrayName});
  const DataPath calculatedNNPath({k_DataContainer, k_CellData, k_CalculatedPrefix + k_NearestNeighborsArrayName});
  const auto& exemplarNNData = dataStructure.getDataRefAs<IDataArray>(exemplarNNPath);
  const auto& calculatedNNData = dataStructure.getDataRefAs<IDataArray>(calculatedNNPath);
  UnitTest::CompareDataArraysByComponent<int32>(exemplarNNData, calculatedNNData, 0, nnCompNum);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
