#include <catch2/catch.hpp>

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/DBSCANFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/DBSCAN.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// 2D Cases - Derived From https://scikit-learn.org/stable/auto_examples/cluster/plot_cluster_comparison.html
const std::string k_AnisoArrayName = "aniso";
const DataPath k_AnisoArrayPath = DataPath({k_AnisoArrayName});
const std::string k_BlobsArrayName = "blobs";
const DataPath k_BlobsArrayPath = DataPath({k_BlobsArrayName});
const std::string k_CirclesArrayName = "noisy_circles";
const DataPath k_CirclesArrayPath = DataPath({k_CirclesArrayName});
const std::string k_MoonsArrayName = "noisy_moons";
const DataPath k_MoonsArrayPath = DataPath({k_MoonsArrayName});
const std::string k_NoStructureArrayName = "no_structure";
const DataPath k_NoStructureArrayPath = DataPath({k_NoStructureArrayName});
const std::string k_VariedArrayName = "varied";
const DataPath k_VariedArrayPath = DataPath({k_VariedArrayName});

const DataPath k_AnisoGeomPath = DataPath({"AnisoGeometry"});
const DataPath k_AnsioClusterArrayPath = k_AnisoGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Aniso Cluster Ids");
const DataPath k_BlobsGeomPath = DataPath({"BlobsGeometry"});
const DataPath k_BlobsClusterArrayPath = k_BlobsGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Blobs Cluster Ids");
const DataPath k_CirclesGeomPath = DataPath({"CirclesGeometry"});
const DataPath k_CirclesClusterArrayPath = k_CirclesGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Circles Cluster Ids");
const DataPath k_MoonsGeomPath = DataPath({"MoonsGeometry"});
const DataPath k_MoonsClusterArrayPath = k_MoonsGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Moons Cluster Ids");
const DataPath k_NoStructureGeomPath = DataPath({"NoStructureGeometry"});
const DataPath k_NoStructureClusterArrayPath = k_NoStructureGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("No Structure Cluster Ids");
const DataPath k_VariedGeomPath = DataPath({"VariedGeometry"});
const DataPath k_VariedClusterArrayPath = k_VariedGeomPath.createChildPath(Constants::k_Vertex_Data).createChildPath("Varied Cluster Ids");

const std::string k_IdsPostFix = " Ids";
const std::string k_AMPostFix = " AM";

const fs::path k_2DTestFile(fmt::format("{}/dbscan_test/7_0_2d_dbscan_test_data.dream3d", unit_test::k_TestFilesDir));

void LDFTestCase2D(const DataPath& targetPath, float32 epsilonVal, int32 minPtsVal, const DataPath& exemplarClusterIds)
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_2DTestFile);

  const std::string k_GeneratedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const DataPath k_GeneratedIdsPath = DataPath{{k_GeneratedIdsName}};
  const DataPath k_GeneratedAMPath = DataPath{{targetPath.getTargetName() + k_AMPostFix}};

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(epsilonVal));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(minPtsVal));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_GeneratedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_GeneratedAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_LDF_2d_{}_test.dream3d", unit_test::k_BinaryTestOutputDir, targetPath.getTargetName())));
#endif

  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(k_GeneratedIdsPath);
  int32 maxVal = *std::max_element(generatedIds.begin(), generatedIds.end()) + 1;

  REQUIRE(maxVal == dataStructure.getDataAs<AttributeMatrix>(k_GeneratedAMPath)->getNumTuples());

  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(k_GeneratedIdsPath), dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

std::vector<usize> BinPoints(const Int32Array& dataArray)
{
  int32 maxVal = *std::max_element(dataArray.begin(), dataArray.end());
  std::vector<usize> bins(maxVal + 1, 0);

  for(int32 val : dataArray)
  {
    bins[val]++;
  }

  return bins;
}

void RandomTestCase2D(const DataPath& targetPath, float32 epsilonVal, int32 minPtsVal, const DataPath& exemplarClusterIds, ChoicesParameter::ValueType randomType)
{
  REQUIRE((randomType == to_underlying(DBSCAN::ParseOrder::Random) || randomType == to_underlying(DBSCAN::ParseOrder::SeededRandom)));

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_2DTestFile);

  const std::string k_GeneratedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const DataPath k_GeneratedIdsPath = DataPath{{k_GeneratedIdsName}};
  const DataPath k_GeneratedAMPath = DataPath{{targetPath.getTargetName() + k_AMPostFix}};

  uint64 k_Seed = std::mt19937_64::default_seed;

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(randomType));
    args.insertOrAssign(DBSCANFilter::k_SeedValue_Key, std::make_any<uint64>(k_Seed)); // Will be ignored if randomType == DBSCAN::ParseOrder::Random
    args.insertOrAssign(DBSCANFilter::k_SeedArrayName_Key, std::make_any<std::string>("seed_array"));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(epsilonVal));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(minPtsVal));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_GeneratedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_GeneratedAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_Random_2d_{}_test.dream3d", unit_test::k_BinaryTestOutputDir, targetPath.getTargetName())));
#endif

  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(k_GeneratedIdsPath);
  std::vector<usize> generatedBins = ::BinPoints(generatedIds);
  REQUIRE_FALSE(generatedBins.empty());

  REQUIRE(generatedBins.size() == dataStructure.getDataAs<AttributeMatrix>(k_GeneratedAMPath)->getNumTuples());

  const auto& exemplarIds = dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds);
  std::vector<usize> exemplarBins = ::BinPoints(exemplarIds);
  REQUIRE_FALSE(exemplarBins.empty());

  REQUIRE(generatedBins.size() == exemplarBins.size());

  // Identifier 0 is unlabeled points so these should match regardless
  REQUIRE(generatedBins[0] == exemplarBins[0]);

  // Clusters should be the same but id labels may be different
  std::vector<bool> visited(exemplarBins.size(), false);
  for(usize i = 1; i < generatedBins.size(); i++)
  {
    bool found = false;
    for(usize j = 1; j < exemplarBins.size(); j++)
    {
      if(!visited[j])
      {
        if(generatedBins[i] == exemplarBins[j])
        {
          found = true;
          visited[j] = true;
          break;
        }
      }
    }
    REQUIRE(found);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Aniso", "[SimplnxCore][DBSCAN]")
{
  float32 epsVal = 0.15f;
  int32 minPtsVal = 4;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath);
  ::RandomTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Blobs", "[SimplnxCore][DBSCAN]")
{
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath);
  ::RandomTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Noisy Circles", "[SimplnxCore][DBSCAN]")
{
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath);
  ::RandomTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Noisy Moons", "[SimplnxCore][DBSCAN]")
{
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath);
  ::RandomTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: No Structure", "[SimplnxCore][DBSCAN]")
{
  float32 epsVal = 0.3f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath);
  ::RandomTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Varied", "[SimplnxCore][DBSCAN]")
{
  float32 epsVal = 0.18f;
  int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath);
  ::RandomTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 3D Test (LowDensityFirst)", "[SimplnxCore][DBSCAN]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/dbscan_test/7_0_3d_dbscan_test_data.dream3d", unit_test::k_TestFilesDir)));

  const DataPath vertexGeom =  DataPath{{"Reduced Vertex Geom"}};
  const DataPath targetPath = vertexGeom.createChildPath("Shared Vertex List");
  const DataPath exemplarClusterIds = vertexGeom.createChildPath("VertexData").createChildPath("Cluster Ids");

  const std::string k_GeneratedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const DataPath k_GeneratedIdsPath = vertexGeom.createChildPath(k_GeneratedIdsName);
  const DataPath k_GeneratedAMPath = vertexGeom.createChildPath(targetPath.getTargetName() + k_AMPostFix);

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.0099999998));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(5));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_GeneratedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_GeneratedAMPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_DBSCAN_LDF_3d_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(k_GeneratedIdsPath);
  int32 maxVal = *std::max_element(generatedIds.begin(), generatedIds.end()) + 1;

  REQUIRE(maxVal == dataStructure.getDataAs<AttributeMatrix>(k_GeneratedAMPath)->getNumTuples());

  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(k_GeneratedIdsPath), dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
