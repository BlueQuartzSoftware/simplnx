#include "SimplnxCore/Filters/FindFeatureCentroidsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::FindFeatureCentroidsFilter", "[SimplnxCore][FindFeatureCentroidsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  const std::string k_CentroidsNX("Centroids NX");

  // Instantiate FindFeatureCentroidsFilter
  {
    FindFeatureCentroidsFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});
    const DataPath k_FeatureAttributeMatrix({k_DataContainer, k_CellFeatureData});
    const DataPath k_SelectedImageGeometry({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(FindFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsNX));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAttributeMatrix));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedImageGeometry));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});

    const auto& k_CentroidsArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsArrayPath);
    const auto& k_CentroidsNXArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsNXArrayPath);

    CompareDataArrays<float>(k_CentroidsArray, k_CentroidsNXArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
