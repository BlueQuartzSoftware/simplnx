#include "ComplexCore/Filters/FindFeatureCentroidsFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::UnitTest;
using namespace complex::Constants;

TEST_CASE("ComplexCore::FindFeatureCentroidsFilter", "[ComplexCore][FindFeatureCentroidsFilter]")
{
  const std::string kDataInputArchive = "6_6_stats_test.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_stats_test.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

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
    args.insertOrAssign(FindFeatureCentroidsFilter::k_CentroidsArrayPath_Key, std::make_any<std::string>(k_CentroidsNX));
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

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
