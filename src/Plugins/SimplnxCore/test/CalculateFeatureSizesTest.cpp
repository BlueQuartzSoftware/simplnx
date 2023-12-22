#include "SimplnxCore/Filters/CalculateFeatureSizesFilter.hpp"
#include "SimplnxCore/Filters/ReadRawBinaryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

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

} // namespace

TEST_CASE("SimplnxCore::CalculateFeatureSizes", "[SimplnxCore][CalculateFeatureSizes]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(nx::core::Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(nx::core::Constants::k_FeatureIds);
  DataPath featureGroup = smallIn100Group.createChildPath(nx::core::Constants::k_CellFeatureData);
  std::string volumesName = "computed_volumes";
  std::string numElementsName = "computed_NumElements";
  std::string EquivalentDiametersName = "computed_EquivalentDiameters";

  std::vector<std::string> featureNames = {k_Volumes, k_EquivalentDiameters, ::k_NumElements};

  {
    CalculateFeatureSizesFilter filter;
    Arguments args;

    args.insert(CalculateFeatureSizesFilter::k_GeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insert(CalculateFeatureSizesFilter::k_SaveElementSizes_Key, std::make_any<bool>(false));
    args.insert(CalculateFeatureSizesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(CalculateFeatureSizesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(featureGroup));
    args.insert(CalculateFeatureSizesFilter::k_VolumesPath_Key, std::make_any<std::string>(volumesName));
    args.insert(CalculateFeatureSizesFilter::k_EquivalentDiametersPath_Key, std::make_any<std::string>(EquivalentDiametersName));
    args.insert(CalculateFeatureSizesFilter::k_NumElementsPath_Key, std::make_any<std::string>(numElementsName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare Outputs
  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_Volumes);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(volumesName));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_EquivalentDiameters);
    UnitTest::CompareArrays<float32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(EquivalentDiametersName));
  }

  {
    DataPath exemplaryDataPath = featureGroup.createChildPath(k_NumElements);
    UnitTest::CompareArrays<int32>(dataStructure, exemplaryDataPath, featureGroup.createChildPath(numElementsName));
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/calculate_feature_sizes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
