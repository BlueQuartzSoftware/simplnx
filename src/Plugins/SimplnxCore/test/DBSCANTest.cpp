#include <catch2/catch.hpp>

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/DBSCANFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr std::array<uint32, 12> k_CircleIndexes = {553, 554, 555, 557, 601, 602, 649, 651, 696, 697, 742, 744};
constexpr std::array<uint32, 7> k_TriangleIndexes = {556, 600, 603, 647, 694, 743, 745};
constexpr std::array<uint32, 6> k_XIndexes = {604, 648, 650, 695, 698, 741};

const std::string k_TargetArrayName = "STRAIN";
const std::string k_ClusterDataNX = "ClusterAM";

const DataPath k_GeomPath = DataPath({"6_6_DBSCAN"});
const DataPath k_CellPath = k_GeomPath.createChildPath(Constants::k_CellData);
const DataPath k_TargetArrayPath = k_CellPath.createChildPath(k_TargetArrayName);
const DataPath k_ClusterDataPathNX = k_GeomPath.createChildPath(k_ClusterDataNX);

const std::string k_ClusterIdsName = "ClusterIds";
const std::string k_ClusterIdsNameNX = k_ClusterIdsName + "NX";

const DataPath k_ClusterIdsPath = k_CellPath.createChildPath(k_ClusterIdsName);
const DataPath k_ClusterIdsPathNX = k_CellPath.createChildPath(k_ClusterIdsNameNX);
} // namespace

TEST_CASE("SimplnxCore::DBSCAN: Valid Filter Execution (Precached, Iterative)", "[SimplnxCore][DBSCAN]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "DBSCAN_tests.tar.gz", "DBSCAN_tests");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/DBSCAN_tests/default/6_5_DBSCAN_Data.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_SeedChoice_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(AlgType::Iterative)));
    args.insertOrAssign(DBSCANFilter::k_UsePrecaching_Key, std::make_any<bool>(true));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.01));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(50));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_TargetArrayPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPath), dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPathNX));

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_precached_iterative_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::DBSCAN: Valid Filter Execution (uncached, Iterative)", "[SimplnxCore][DBSCAN]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "DBSCAN_tests.tar.gz", "DBSCAN_tests");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/DBSCAN_tests/default/6_5_DBSCAN_Data.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_SeedChoice_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(AlgType::Iterative)));
    args.insertOrAssign(DBSCANFilter::k_UsePrecaching_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.01));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(50));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_TargetArrayPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPath), dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPathNX));

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_uncached_iterative_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::DBSCAN: Valid Filter Execution (precached, Random)", "[SimplnxCore][DBSCAN]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "DBSCAN_tests.tar.gz", "DBSCAN_tests");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/DBSCAN_tests/default/6_5_DBSCAN_Data.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_SeedChoice_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(AlgType::SeededRandom)));
    args.insertOrAssign(DBSCANFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(DBSCANFilter::k_UsePrecaching_Key, std::make_any<bool>(true));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.01));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(50));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_TargetArrayPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  const auto& clusterIdsDataStore = dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPathNX).getDataStoreRef();
  REQUIRE(*std::max_element(clusterIdsDataStore.cbegin(), clusterIdsDataStore.cend()) == 1);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_precached_random_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::DBSCAN: Valid Filter Execution (uncached, Random)", "[SimplnxCore][DBSCAN]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "DBSCAN_tests.tar.gz", "DBSCAN_tests");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/DBSCAN_tests/default/6_5_DBSCAN_Data.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_SeedChoice_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(AlgType::SeededRandom)));
    args.insertOrAssign(DBSCANFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(DBSCANFilter::k_UsePrecaching_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.01));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(50));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_TargetArrayPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  const auto& clusterIdsDataStore = dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPathNX).getDataStoreRef();
  REQUIRE(*std::max_element(clusterIdsDataStore.cbegin(), clusterIdsDataStore.cend()) == 1);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_uncached_random_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::DBSCAN: Valid Detailed Filter Execution (cached, Random)", "[SimplnxCore][DBSCAN]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "DBSCAN_tests.tar.gz", "DBSCAN_tests");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/DBSCAN_tests/default/6_5_DBSCAN_Data.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_SeedChoice_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(AlgType::SeededRandom)));
    args.insertOrAssign(DBSCANFilter::k_SeedValue_Key, std::make_any<uint64>(5489));
    args.insertOrAssign(DBSCANFilter::k_UsePrecaching_Key, std::make_any<bool>(true));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.06));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(100));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_TargetArrayPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  const auto& clusterIdsDataStore = dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPathNX).getDataStoreRef();
  REQUIRE(*std::max_element(clusterIdsDataStore.cbegin(), clusterIdsDataStore.cend()) == 2);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_detailed_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
