#include <catch2/catch.hpp>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/KMedoidsFilter.hpp"

using namespace complex;

namespace
{
constexpr std::array<uint32, 12> k_CircleIndexes = {553, 554, 555, 557, 601, 602, 649, 651, 696, 697, 742, 744};
constexpr std::array<uint32, 7> k_TriangleIndexes = {556, 600, 603, 647, 694, 743, 745};
constexpr std::array<uint32, 6> k_XIndexes = {604, 648, 650, 695, 698, 741};

const std::string k_ClusterData = "ClusterData";
const std::string k_ClusterDataNX = k_ClusterData + "NX";

const DataPath k_QuadGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_ClusterDataPathNX = k_QuadGeomPath.createChildPath(k_ClusterDataNX);

const std::string k_ClusterIdsName = "ClusterIds";
const std::string k_MedoidsName = "ClusterMedoids";
const std::string k_ClusterIdsNameNX = k_ClusterIdsName + "NX";
const std::string k_MedoidsNameNX = k_MedoidsName + "NX";

const DataPath k_ClusterIdsPathNX = k_CellPath.createChildPath(k_ClusterIdsNameNX);
} // namespace

TEST_CASE("ComplexCore::KMedoidsFilter: Valid Filter Execution", "[ComplexCore][KMedoidsFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "k_files.tar.gz", "k_files");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files/7_0_medoids_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    KMedoidsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMedoidsFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(KMedoidsFilter::k_SeedValue_Key, std::make_any<uint64>(5489)); // Default Seed
    args.insertOrAssign(KMedoidsFilter::k_InitClusters_Key, std::make_any<uint64>(3));
    args.insertOrAssign(KMedoidsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(KMedoidsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(KMedoidsFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(KMedoidsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));
    args.insertOrAssign(KMedoidsFilter::k_MedoidsArrayName_Key, std::make_any<std::string>(k_MedoidsNameNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  /**
   * To check the validity of the filter we will be testing for a 5x5 square cut out as a pattern
   * rather then specific data constants. This is due to the disparity between cross platform random distribution.
   *
   * Here's how it should look:
   * T = triangle
   * C = Circle
   * X = X
   *
   * X C T C T
   * T X C C X
   * T X C X C
   * T C C T X
   * C C C T C
   *
   * The identifiers for the types is most easily defined by checking the following:
   * |--------------|
   * | Type | Index |
   * |--------------|
   * |  X  |  741   |
   * |--------------|
   * |  C  |  742   |
   * |--------------|
   * |  T  |  743   |
   * |--------------|
   *
   * Be sure to check that oll of those values are unique before validating the rest of the indexes,
   * i.e. index 741 and 742 should not be the same
   */

  auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(k_ClusterIdsPathNX);

  int32 xVal = clusterIds[741];
  int32 cVal = clusterIds[742];
  int32 tVal = clusterIds[743];

  REQUIRE(xVal != cVal);
  REQUIRE(cVal != tVal);
  REQUIRE(tVal != xVal);

  for(auto index : k_XIndexes)
  {
    REQUIRE(xVal == clusterIds[index]);
  }

  for(auto index : k_CircleIndexes)
  {
    REQUIRE(cVal == clusterIds[index]);
  }

  for(auto index : k_TriangleIndexes)
  {
    REQUIRE(tVal == clusterIds[index]);
  }

  // Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_k_medoids_0_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
