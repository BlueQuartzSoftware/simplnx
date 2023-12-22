#include "SimplnxCore/Filters/FindFeaturePhasesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("SimplnxCore::FindFeaturePhasesFilter(Valid Parameters)", "[SimplnxCore][FindFeaturePhasesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);
  DataPath cellFeatureDataPath = smallIn100Group.createChildPath(Constants::k_CellFeatureData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(nx::core::Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(nx::core::Constants::k_FeatureIds);
  std::string computedPrefix = "Computed_";
  std::string featurePhasesName = computedPrefix + nx::core::Constants::k_Phases.str();
  DataPath featurePhasesPath = cellFeatureDataPath.createChildPath(featurePhasesName);

  {
    FindFeaturePhasesFilter ffpFilter;
    Arguments args;
    args.insert(FindFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesPath));
    args.insert(FindFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(FindFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(cellFeatureDataPath));
    args.insert(FindFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(featurePhasesName));

    auto preflightResult = ffpFilter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = ffpFilter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    DataPath exemplaryDataPath = cellFeatureDataPath.createChildPath(nx::core::Constants::k_Phases);
    const Int32Array& featureArrayExemplary = dataStructure.getDataRefAs<Int32Array>(exemplaryDataPath);

    const Int32Array& createdFeatureArray = dataStructure.getDataRefAs<Int32Array>(featurePhasesPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_phases_filter.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
