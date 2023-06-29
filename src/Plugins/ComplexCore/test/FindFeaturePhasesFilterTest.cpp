#include "ComplexCore/Filters/FindFeaturePhasesFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

TEST_CASE("ComplexCore::FindFeaturePhasesFilter(Valid Parameters)", "[ComplexCore][FindFeaturePhasesFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d",
                                                             complex::unit_test::k_BinaryTestOutputDir);
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData);
  DataPath cellFeatureDataPath = smallIn100Group.createChildPath(Constants::k_CellFeatureData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(complex::Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(complex::Constants::k_FeatureIds);
  std::string computedPrefix = "Computed_";
  std::string featurePhasesName = computedPrefix + complex::Constants::k_Phases.str();
  DataPath featurePhasesPath = cellFeatureDataPath.createChildPath(featurePhasesName);

  {
    FindFeaturePhasesFilter ffpFilter;
    Arguments args;
    args.insert(FindFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesPath));
    args.insert(FindFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(FindFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(cellFeatureDataPath));
    args.insert(FindFeaturePhasesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<std::string>(featurePhasesName));

    auto preflightResult = ffpFilter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = ffpFilter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);

    DataPath exemplaryDataPath = cellFeatureDataPath.createChildPath(complex::Constants::k_Phases);
    const Int32Array& featureArrayExemplary = dataStructure.getDataRefAs<Int32Array>(exemplaryDataPath);

    const Int32Array& createdFeatureArray = dataStructure.getDataRefAs<Int32Array>(featurePhasesPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }

// Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_phases_filter.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
