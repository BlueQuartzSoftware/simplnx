#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "complex_plugins/Utilities/TestUtilities.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/FindFeatureCentroidsFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("Core::FindFeatureCentroidsFilter", "[Core][FindFeatureCentroidsFilter]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_find_feature_centroids.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  const std::string k_CentroidsNX("Centroids NX");

  // Instantiate FindFeatureCentroidsFilter
  {
    FindFeatureCentroidsFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_SmallIN100ScanData, k_FeatureIds});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});
    const DataPath k_FeatureAttributeMatrix({k_DataContainer, k_CellFeatureData});
    const DataPath k_SelectedImageGeometry({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(FindFeatureCentroidsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsNXArrayPath));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(k_FeatureAttributeMatrix));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_SelectedImageGeometry));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});

    const auto& k_CentroidsArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsArrayPath);
    const auto& k_CentroidsNXArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsNXArrayPath);

    CompareDataArrays<float>(k_CentroidsArray, k_CentroidsNXArray);
  }

  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_centroids.dream3d", unit_test::k_BinaryDir)));
}
