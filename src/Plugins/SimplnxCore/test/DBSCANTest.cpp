#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/Algorithms/DBSCAN.hpp"
#include "SimplnxCore/Filters/DBSCANFilter.hpp"

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

void CheckClusterInvariants(const DataStructure& dataStructure, const DataPath& idsPath, const DataPath& amPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(idsPath));
  const auto& ids = dataStructure.getDataRefAs<Int32Array>(idsPath);

  // Invariant 1: all IDs non-negative (0 = noise, >=1 = cluster label)
  for(int32 id : ids)
  {
    REQUIRE(id >= 0);
  }

  // Invariant 2: IDs are contiguous — no gap between 0 and maxId
  const int32 maxId = *std::max_element(ids.begin(), ids.end());
  std::vector<bool> seen(static_cast<usize>(maxId + 1), false);
  for(const int32 id : ids)
  {
    seen[static_cast<usize>(id)] = true;
  }
  // Ignore ID zero because that's reserved for unlabeled points
  for(int32 i = 1; i <= maxId; i++)
  {
    REQUIRE(seen[static_cast<usize>(i)]);
  }

  // Invariant 3: AM tuple count equals maxId + 1
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(amPath));
  REQUIRE(static_cast<usize>(maxId + 1) == dataStructure.getDataRefAs<AttributeMatrix>(amPath).getNumberOfTuples());
}

void LDFTestCase2D(const DataPath& targetPath, float32 epsilonVal, int32 minPtsVal, const DataPath& exemplarClusterIds)
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_2DTestFile);

  const std::string generatedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const auto generatedIdsPath = DataPath{{generatedIdsName}};
  const auto generatedAMPath = DataPath{{targetPath.getTargetName() + k_AMPostFix}};

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
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(generatedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(generatedAMPath));

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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath));
  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(generatedIdsPath);
  const int32 maxVal = *std::max_element(generatedIds.begin(), generatedIds.end()) + 1;

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath));
  REQUIRE(maxVal == dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath).getNumberOfTuples());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath), dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));

  ::CheckClusterInvariants(dataStructure, generatedIdsPath, generatedAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

std::vector<usize> BinPoints(const Int32Array& dataArray)
{
  const int32 maxVal = *std::max_element(dataArray.begin(), dataArray.end());
  std::vector<usize> bins(maxVal + 1, 0);

  for(const int32 val : dataArray)
  {
    bins[val]++;
  }

  return bins;
}

void RandomTestCase2D(const DataPath& targetPath, float32 epsilonVal, int32 minPtsVal, const DataPath& exemplarClusterIds, ChoicesParameter::ValueType randomType)
{
  REQUIRE((randomType == to_underlying(DBSCAN::ParseOrder::Random) || randomType == to_underlying(DBSCAN::ParseOrder::SeededRandom)));

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_2DTestFile);

  const std::string generatedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const auto generatedIdsPath = DataPath{{generatedIdsName}};
  const auto generatedAMPath = DataPath{{targetPath.getTargetName() + k_AMPostFix}};

  uint64 seed = std::mt19937_64::default_seed;

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(randomType));
    args.insertOrAssign(DBSCANFilter::k_SeedValue_Key, std::make_any<uint64>(seed)); // Will be ignored if randomType == DBSCAN::ParseOrder::Random
    args.insertOrAssign(DBSCANFilter::k_SeedArrayName_Key, std::make_any<std::string>("seed_array"));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(epsilonVal));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(minPtsVal));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(generatedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(generatedAMPath));

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

  // ParseOrder::Random draws a time-based seed inside the filter, so the shuffle differs on every
  // run. Report the seed the run actually used; without it a failure below is not reproducible.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64Array>(DataPath{{"seed_array"}}));
  const uint64 usedSeed = dataStructure.getDataRefAs<UInt64Array>(DataPath{{"seed_array"}})[0];
  INFO(fmt::format("Parse order {} ran with seed {}", randomType, usedSeed));
  if(randomType == to_underlying(DBSCAN::ParseOrder::SeededRandom))
  {
    // SeededRandom must round-trip the user-supplied seed unchanged
    REQUIRE(usedSeed == seed);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath));
  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(generatedIdsPath);
  std::vector<usize> generatedBins = ::BinPoints(generatedIds);
  REQUIRE_FALSE(generatedBins.empty());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath));
  REQUIRE(generatedBins.size() == dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath).getNumberOfTuples());

  // SeededRandom is deterministic so its output can be compared against the LDF exemplar.
  // Random uses a time-based seed: border-grid assignment is order-dependent, so cluster sizes
  // may differ from the exemplar across runs. Only structural invariants are checked for that case.
  if(randomType == to_underlying(DBSCAN::ParseOrder::SeededRandom))
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));
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
  }

  ::CheckClusterInvariants(dataStructure, generatedIdsPath, generatedAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Aniso", "[SimplnxCore][DBSCAN]")
{
  const float32 epsVal = 0.15f;
  const int32 minPtsVal = 4;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath);
  ::RandomTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_AnisoArrayPath, epsVal, minPtsVal, k_AnsioClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Blobs", "[SimplnxCore][DBSCAN]")
{
  const float32 epsVal = 0.3f;
  const int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath);
  ::RandomTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_BlobsArrayPath, epsVal, minPtsVal, k_BlobsClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Noisy Circles", "[SimplnxCore][DBSCAN]")
{
  const float32 epsVal = 0.3f;
  const int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath);
  ::RandomTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_CirclesArrayPath, epsVal, minPtsVal, k_CirclesClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Noisy Moons", "[SimplnxCore][DBSCAN]")
{
  const float32 epsVal = 0.3f;
  const int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath);
  ::RandomTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_MoonsArrayPath, epsVal, minPtsVal, k_MoonsClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: No Structure", "[SimplnxCore][DBSCAN]")
{
  const float32 epsVal = 0.3f;
  const int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath);
  ::RandomTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_NoStructureArrayPath, epsVal, minPtsVal, k_NoStructureClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 2D Test: Varied", "[SimplnxCore][DBSCAN]")
{
  const float32 epsVal = 0.18f;
  const int32 minPtsVal = 3;
  // The exemplars were generated with LDF
  ::LDFTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath);
  ::RandomTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath, DBSCAN::ParseOrder::Random);
  ::RandomTestCase2D(k_VariedArrayPath, epsVal, minPtsVal, k_VariedClusterArrayPath, DBSCAN::ParseOrder::SeededRandom);
}

TEST_CASE("SimplnxCore::DBSCAN: 3D Test (LowDensityFirst)", "[SimplnxCore][DBSCAN]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "dbscan_test.tar.gz", "dbscan_test");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/dbscan_test/7_0_3d_dbscan_test_data.dream3d", unit_test::k_TestFilesDir)));

  const auto vertexGeom = DataPath{{"Reduced Vertex Geom"}};
  const DataPath targetPath = vertexGeom.createChildPath("Shared Vertex List");
  const DataPath exemplarClusterIds = vertexGeom.createChildPath("VertexData").createChildPath("Cluster Ids");

  const std::string generatedIdsName = targetPath.getTargetName() + k_IdsPostFix;
  const DataPath generatedIdsPath = vertexGeom.createChildPath(generatedIdsName);
  const DataPath generatedAMPath = vertexGeom.createChildPath(targetPath.getTargetName() + k_AMPostFix);

  {
    // Instantiate the filter and an Arguments Object
    DBSCANFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.0099999998f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(5));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(targetPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(generatedIdsName));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(generatedAMPath));

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

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath));
  const auto& generatedIds = dataStructure.getDataRefAs<Int32Array>(generatedIdsPath);
  int32 maxVal = *std::max_element(generatedIds.begin(), generatedIds.end()) + 1;

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath));
  REQUIRE(maxVal == dataStructure.getDataRefAs<AttributeMatrix>(generatedAMPath).getNumberOfTuples());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));
  UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<Int32Array>(generatedIdsPath), dataStructure.getDataRefAs<Int32Array>(exemplarClusterIds));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCAN: Analytical Fixture F1 - No Clusters Warning", "[SimplnxCore][DBSCAN]")
{
  // Class 1 oracle: 4 points at unit-square corners, epsilon=0.1, minPoints=5.
  // Cell side = 0.1/sqrt(2) ~= 0.0707 -> each point occupies its own 1-point cell -> no core grids -> warning -85640.
  // Expected: all cluster IDs = 0, AM has 1 tuple.
  DataStructure dataStructure;

  const DataPath pointsPath{{"points"}};
  const DataPath clusterIdsPath{{"cluster_ids"}};
  const DataPath featureAMPath{{"cluster_am"}};

  auto* pointsArr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "points", {4}, {2});
  REQUIRE(pointsArr != nullptr);
  auto& pointsRef = pointsArr->getDataStoreRef();
  pointsRef[0] = 0.0f;
  pointsRef[1] = 0.0f; // P0 = (0, 0)
  pointsRef[2] = 1.0f;
  pointsRef[3] = 0.0f; // P1 = (1, 0)
  pointsRef[4] = 0.0f;
  pointsRef[5] = 1.0f; // P2 = (0, 1)
  pointsRef[6] = 1.0f;
  pointsRef[7] = 1.0f; // P3 = (1, 1)

  {
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(0.1f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(5));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(pointsPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("cluster_ids"));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(featureAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    // No core grids -> warning, not error
    REQUIRE(executeResult.result.valid());
    REQUIRE_FALSE(executeResult.result.warnings().empty());
    REQUIRE(executeResult.result.warnings()[0].code == -85640);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(clusterIdsPath));
  const auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(clusterIdsPath);
  for(int32 id : clusterIds)
  {
    REQUIRE(id == 0);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath));
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath).getNumberOfTuples() == 1);

  ::CheckClusterInvariants(dataStructure, clusterIdsPath, featureAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCAN: Analytical Fixture F2 - Mask Exclusion", "[SimplnxCore][DBSCAN]")
{
  // Class 1 oracle: 3 points, P2 masked out.
  // epsilon=1.0, minPoints=2 -> cell side = 1.0/sqrt(2) ~= 0.707.
  // Active points P0=(0.0, 0.0) and P1=(0.1, 0.0) both land in grid cell 0 -> 2 points >= minPoints -> core grid -> Cluster 1.
  // P2=(0.0, 0.1) is masked -> excluded from binning -> stays cluster ID 0.
  // Expected: cluster_ids = [1, 1, 0], AM has 2 tuples.
  DataStructure dataStructure;

  const DataPath pointsPath{{"points"}};
  const DataPath maskPath{{"mask"}};
  const DataPath clusterIdsPath{{"cluster_ids"}};
  const DataPath featureAMPath{{"cluster_am"}};

  auto* pointsArr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "points", {3}, {2});
  REQUIRE(pointsArr != nullptr);
  auto& pointsRef = pointsArr->getDataStoreRef();
  pointsRef[0] = 0.0f;
  pointsRef[1] = 0.0f; // P0 = (0.0, 0.0) included
  pointsRef[2] = 0.1f;
  pointsRef[3] = 0.0f; // P1 = (0.1, 0.0) included
  pointsRef[4] = 0.0f;
  pointsRef[5] = 0.1f; // P2 = (0.0, 0.1) masked out

  auto* maskArr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "mask", {3}, {1});
  REQUIRE(maskArr != nullptr);
  auto& maskRef = maskArr->getDataStoreRef();
  maskRef[0] = 1; // P0 included
  maskRef[1] = 1; // P1 included
  maskRef[2] = 0; // P2 excluded

  {
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(2));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(pointsPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("cluster_ids"));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(featureAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(clusterIdsPath));
  const auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(clusterIdsPath);
  REQUIRE(clusterIds[0] == 1); // P0 -> Cluster 1
  REQUIRE(clusterIds[1] == 1); // P1 -> Cluster 1
  REQUIRE(clusterIds[2] == 0); // P2 masked -> noise
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath));
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath).getNumberOfTuples() == 2);

  ::CheckClusterInvariants(dataStructure, clusterIdsPath, featureAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCAN: Analytical Fixture F3 - All Points Masked", "[SimplnxCore][DBSCAN]")
{
  // Class 1 oracle: same 3 points as F2, but every point is masked off.
  // With no active point there are no grid bounds to derive, so no grid cell can be occupied and no
  // core grid can exist. This pins the contract for that case; before the bounds guard was added,
  // the grid dimensions were computed by casting NaN to usize, which is undefined behavior.
  // Expected: warning -85640, all cluster IDs = 0, AM has 1 tuple.
  DataStructure dataStructure;

  const DataPath pointsPath{{"points"}};
  const DataPath maskPath{{"mask"}};
  const DataPath clusterIdsPath{{"cluster_ids"}};
  const DataPath featureAMPath{{"cluster_am"}};

  auto* pointsArr = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "points", {3}, {2});
  REQUIRE(pointsArr != nullptr);
  auto& pointsRef = pointsArr->getDataStoreRef();
  pointsRef[0] = 0.0f;
  pointsRef[1] = 0.0f; // P0 = (0.0, 0.0)
  pointsRef[2] = 0.1f;
  pointsRef[3] = 0.0f; // P1 = (0.1, 0.0)
  pointsRef[4] = 0.0f;
  pointsRef[5] = 0.1f; // P2 = (0.0, 0.1)

  auto* maskArr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "mask", {3}, {1});
  REQUIRE(maskArr != nullptr);
  auto& maskRef = maskArr->getDataStoreRef();
  maskRef[0] = 0;
  maskRef[1] = 0;
  maskRef[2] = 0;

  {
    DBSCANFilter filter;
    Arguments args;

    args.insertOrAssign(DBSCANFilter::k_ParseOrderIndex_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DBSCAN::ParseOrder::LowDensityFirst)));
    args.insertOrAssign(DBSCANFilter::k_Epsilon_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(DBSCANFilter::k_MinPoints_Key, std::make_any<int32>(2));
    args.insertOrAssign(DBSCANFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(DBSCANFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    args.insertOrAssign(DBSCANFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(pointsPath));
    args.insertOrAssign(DBSCANFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("cluster_ids"));
    args.insertOrAssign(DBSCANFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(featureAMPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    // Nothing to cluster -> warning, not error and not a crash
    REQUIRE(executeResult.result.valid());
    REQUIRE_FALSE(executeResult.result.warnings().empty());
    REQUIRE(executeResult.result.warnings()[0].code == -85640);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(clusterIdsPath));
  const auto& clusterIds = dataStructure.getDataRefAs<Int32Array>(clusterIdsPath);
  for(int32 id : clusterIds)
  {
    REQUIRE(id == 0);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath));
  REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(featureAMPath).getNumberOfTuples() == 1);

  ::CheckClusterInvariants(dataStructure, clusterIdsPath, featureAMPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::DBSCANFilter: SIMPL Backwards Compatibility", "[SimplnxCore][DBSCANFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "DBSCANFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "DBSCANFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<DBSCANFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (AMPathBuilderFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<float32>(DBSCANFilter::k_Epsilon_Key) == 2.5f);
      CHECK(args.value<int32>(DBSCANFilter::k_MinPoints_Key) == 5);
      CHECK(args.value<ChoicesParameter::ValueType>(DBSCANFilter::k_DistanceMetric_Key) == 0);
      CHECK(args.value<bool>(DBSCANFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(DBSCANFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(DBSCANFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(DBSCANFilter::k_FeatureIdsArrayName_Key) == "TestName");
    }
  }
}
