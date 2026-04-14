#include "SimplnxCore/Filters/ScalarSegmentFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/SegmentFeaturesTestUtils.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <set>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
// Exemplar archive
const std::string k_ArchiveName = "segment_features_exemplars.tar.gz";
const std::string k_DataDirName = "segment_features_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_SmallExemplarFile = k_DataDir / "scalar_small.dream3d";
const fs::path k_LargeExemplarFile = k_DataDir / "scalar_large.dream3d";

// Geometry names
constexpr StringLiteral k_GeomName = "DataContainer";
constexpr StringLiteral k_CellDataName = "CellData";
constexpr StringLiteral k_FeatureDataName = "CellFeatureData";

// Output array paths
const DataPath k_GeomPath({k_GeomName});
const DataPath k_FeatureIdsPath({k_GeomName, k_CellDataName, "FeatureIds"});
const DataPath k_ActivePath({k_GeomName, k_FeatureDataName, "Active"});
const DataPath k_MaskPath({k_GeomName, k_CellDataName, "Mask"});

// Test dimensions
constexpr usize k_SmallDim = 15;
constexpr usize k_SmallBlockSize = 5;
constexpr usize k_LargeDim = 200;
constexpr usize k_LargeBlockSize = 25;

/**
 * @brief Populates ScalarSegmentFeaturesFilter arguments.
 */
void SetupArgs(Arguments& args, bool useMask, bool isPeriodic, int tolerance, ChoicesParameter::ValueType neighborScheme = 0, bool randomize = false)
{
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(DataPath({k_GeomName, k_CellDataName, "ScalarData"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(tolerance));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(useMask ? k_MaskPath : DataPath{}));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>(std::string(k_FeatureDataName)));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(randomize));
}
} // namespace

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Small Correctness", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // int32 1-comp => 15*15*4 = 900 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 900, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_SmallExemplarFile);

  std::string testName = GENERATE("Base", "Masked", "Periodic", "Tolerance");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    const bool useMask = (testName == "Masked");
    const bool isPeriodic = (testName == "Periodic");
    const int tolerance = (testName == "Tolerance") ? 1 : 0;

    const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};

    DataStructure dataStructure;
    auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, {k_SmallDim, k_SmallDim, k_SmallDim}, std::string(k_GeomName), std::string(k_CellDataName));
    BuildScalarTestData(dataStructure, cellShape, am->getId(), k_SmallBlockSize, "ScalarData", isPeriodic);

    if(useMask)
    {
      BuildSphericalMask(dataStructure, cellShape, am->getId());
    }

    UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(DataPath({k_GeomName, k_CellDataName, "ScalarData"})));

    ScalarSegmentFeaturesFilter filter;
    Arguments args;
    SetupArgs(args, useMask, isPeriodic, tolerance);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Compare against exemplar
    const std::string exemplarGeomName = testName + "_Exemplar";
    const DataPath exemplarFeatureIdsPath({exemplarGeomName, std::string(k_CellDataName), "FeatureIds"});
    const DataPath exemplarActivePath({exemplarGeomName, std::string(k_FeatureDataName), "Active"});

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath));
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath), dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath));
    CompareDataArrays<uint8>(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath), dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: FaceEdgeVertex Connectivity", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // Shared test: verifies vertex and edge connectivity with FaceEdgeVertex scheme.
  // Setup lambda creates ScalarData with 4 isolated voxels (2 pairs) and configures args.
  auto setupScalar = [](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, ChoicesParameter::ValueType neighborScheme) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    const DataPath scalarPath = cellDataPath.createChildPath("ScalarData");
    auto scalarDS = DataStoreUtilities::CreateResolvedDataStore<int32>(ds, scalarPath, cellShape, {1});
    auto* scalarArr = DataArray<int32>::Create(ds, "ScalarData", scalarDS, am.getId());
    auto& store = scalarArr->getDataStoreRef();
    store.fill(0);
    store[0 * 9 + 0 * 3 + 0] = 1; // (0,0,0) — vertex pair A
    store[1 * 9 + 1 * 3 + 1] = 1; // (1,1,1) — vertex pair B
    store[0 * 9 + 0 * 3 + 2] = 2; // (2,0,0) — edge pair C
    store[1 * 9 + 1 * 3 + 2] = 2; // (2,1,1) — edge pair D

    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(cellDataPath.createChildPath("ScalarData")));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(0));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>("CellFeatureData"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  };

  RunFaceEdgeVertexConnectivityTest<ScalarSegmentFeaturesFilter>([&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupScalar(args, ds, gp, cp, 0); },
                                                                 [&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupScalar(args, ds, gp, cp, 1); });
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: 200x200x200 Large OOC", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // int32 1-comp => 200*200*4 = 160,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 160000, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_LargeExemplarFile);

  const ShapeType cellShape = {k_LargeDim, k_LargeDim, k_LargeDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, {k_LargeDim, k_LargeDim, k_LargeDim}, std::string(k_GeomName), std::string(k_CellDataName));
  BuildScalarTestData(dataStructure, cellShape, am->getId(), k_LargeBlockSize, "ScalarData", true);
  BuildSphericalMask(dataStructure, cellShape, am->getId());

  UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(DataPath({k_GeomName, k_CellDataName, "ScalarData"})));

  ScalarSegmentFeaturesFilter filter;
  Arguments args;
  SetupArgs(args, /*useMask=*/true, /*isPeriodic=*/true, /*tolerance=*/0);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Compare against exemplar
  const DataPath exemplarFeatureIdsPath({"DataContainer_Exemplar", std::string(k_CellDataName), "FeatureIds"});
  const DataPath exemplarActivePath({"DataContainer_Exemplar", std::string(k_FeatureDataName), "Active"});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath));
  CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath), dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath));
  CompareDataArrays<uint8>(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath), dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: No Valid Voxels Returns Error", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  RunNoValidVoxelsErrorTest<ScalarSegmentFeaturesFilter>([](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, const DataPath& maskPath) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    CreateTestDataArray<int32>(ds, "ScalarData", cellShape, {1}, am.getId());

    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(cellDataPath.createChildPath("ScalarData")));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(0));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>("FeatureData"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  });
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Randomize Feature IDs", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  constexpr usize k_ExpectedFeatures = 27; // 3^3
  const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, {k_SmallDim, k_SmallDim, k_SmallDim}, std::string(k_GeomName), std::string(k_CellDataName));
  BuildScalarTestData(dataStructure, cellShape, am->getId(), k_SmallBlockSize);

  ScalarSegmentFeaturesFilter filter;
  Arguments args;
  SetupArgs(args, /*useMask=*/false, /*isPeriodic=*/false, /*tolerance=*/0, /*neighborScheme=*/0, /*randomize=*/true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
  const auto& actives = dataStructure.getDataRefAs<UInt8Array>(k_ActivePath);
  REQUIRE(actives.getNumberOfTuples() == k_ExpectedFeatures + 1);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();
  std::set<int32> uniqueIds;
  int32 minId = std::numeric_limits<int32>::max();
  int32 maxId = std::numeric_limits<int32>::min();
  for(usize i = 0; i < featureStore.getNumberOfTuples(); i++)
  {
    int32 fid = featureStore.getValue(i);
    uniqueIds.insert(fid);
    minId = std::min(minId, fid);
    maxId = std::max(maxId, fid);
  }
  REQUIRE(minId == 1);
  REQUIRE(maxId == static_cast<int32>(k_ExpectedFeatures));
  REQUIRE(uniqueIds.size() == k_ExpectedFeatures);
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Generate Test Data", "[SimplnxCore][ScalarSegmentFeatures][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/scalar_segment_features", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  // Small input data (15^3) — one geometry per test variant
  {
    const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
    const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

    DataStructure ds;

    auto* amBase = BuildSegmentFeaturesTestGeometry(ds, dims, "Base", std::string(k_CellDataName));
    BuildScalarTestData(ds, cellShape, amBase->getId(), k_SmallBlockSize);

    auto* amMasked = BuildSegmentFeaturesTestGeometry(ds, dims, "Masked", std::string(k_CellDataName));
    BuildScalarTestData(ds, cellShape, amMasked->getId(), k_SmallBlockSize);
    BuildSphericalMask(ds, cellShape, amMasked->getId());

    auto* amPeriodic = BuildSegmentFeaturesTestGeometry(ds, dims, "Periodic", std::string(k_CellDataName));
    BuildScalarTestData(ds, cellShape, amPeriodic->getId(), k_SmallBlockSize, "ScalarData", true);

    auto* amTolerance = BuildSegmentFeaturesTestGeometry(ds, dims, "Tolerance", std::string(k_CellDataName));
    BuildScalarTestData(ds, cellShape, amTolerance->getId(), k_SmallBlockSize);

    UnitTest::WriteTestDataStructure(ds, outputDir / "small_input.dream3d");
  }

  // Large input data (200^3) — mask=true, periodic=true
  {
    const ShapeType cellShape = {k_LargeDim, k_LargeDim, k_LargeDim};
    const std::array<usize, 3> dims = {k_LargeDim, k_LargeDim, k_LargeDim};

    DataStructure ds;
    auto* am = BuildSegmentFeaturesTestGeometry(ds, dims, std::string(k_GeomName), std::string(k_CellDataName));
    BuildScalarTestData(ds, cellShape, am->getId(), k_LargeBlockSize, "ScalarData", true);
    BuildSphericalMask(ds, cellShape, am->getId());

    UnitTest::WriteTestDataStructure(ds, outputDir / "large_input.dream3d");
  }
}
