#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/KMeansFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
const std::string k_ClusterData = "ClusterData";
const std::string k_ClusterDataNX = k_ClusterData + "NX";

const DataPath k_QuadGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_ClusterDataPath = k_QuadGeomPath.createChildPath(k_ClusterData);
const DataPath k_ClusterDataPathNX = k_QuadGeomPath.createChildPath(k_ClusterDataNX);

const std::string k_ClusterIdsName = "ClusterIds";
const std::string k_MeansName = "CLusterMeans";
const std::string k_ClusterIdsNameNX = k_ClusterIdsName + "NX";
const std::string k_MeansNameNX = k_MeansName + "NX";

const DataPath k_ClusterIdsPath = k_CellPath.createChildPath(k_ClusterIdsName);
const DataPath k_MeansPath = k_ClusterDataPath.createChildPath(k_MeansName);

const DataPath k_ClusterIdsPathNX = k_CellPath.createChildPath(k_ClusterIdsNameNX);
const DataPath k_MeansPathNX = k_ClusterDataPathNX.createChildPath(k_MeansNameNX);
} // namespace

TEST_CASE("ComplexCore::KMeans: Valid Filter Execution", "[ComplexCore][KMeans]")
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files/7_0_means_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    KMeansFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMeansFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(KMeansFilter::k_SeedValue_Key, std::make_any<uint64>(5489)); // Default Seed
    args.insertOrAssign(KMeansFilter::k_InitClusters_Key, std::make_any<uint64>(3));
    args.insertOrAssign(KMeansFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(KMeansFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(KMeansFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(KMeansFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));
    args.insertOrAssign(KMeansFilter::k_MeansArrayName_Key, std::make_any<std::string>(k_MeansNameNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<int32>(dataStructure, k_ClusterIdsPath, k_ClusterIdsPathNX);
  UnitTest::CompareArrays<float32>(dataStructure, k_MeansPath, k_MeansPathNX);

  // Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_k_means_0_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
