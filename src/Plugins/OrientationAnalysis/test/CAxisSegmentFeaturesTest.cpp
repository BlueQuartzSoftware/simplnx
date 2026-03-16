#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/SegmentFeaturesTestUtils.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <set>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
// Exemplar archive (shared across Scalar, EBSD, CAxis)
const std::string k_ArchiveName = "segment_features_exemplars.tar.gz";
const std::string k_DataDirName = "segment_features_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_SmallExemplarFile = k_DataDir / "caxis_small.dream3d";
const fs::path k_LargeExemplarFile = k_DataDir / "caxis_large.dream3d";

// Geometry names
constexpr StringLiteral k_GeomName = "DataContainer";
constexpr StringLiteral k_CellDataName = "CellData";
constexpr StringLiteral k_FeatureDataName = "CellFeatureData";
constexpr StringLiteral k_EnsembleName = "CellEnsembleData";

// Output array paths
const DataPath k_GeomPath({k_GeomName});
const DataPath k_FeatureIdsPath({k_GeomName, k_CellDataName, "FeatureIds"});
const DataPath k_ActivePath({k_GeomName, k_FeatureDataName, "Active"});
const DataPath k_MaskPath({k_GeomName, k_CellDataName, "Mask"});
const DataPath k_QuatsPath({k_GeomName, k_CellDataName, "Quats"});
const DataPath k_PhasesPath({k_GeomName, k_CellDataName, "Phases"});
const DataPath k_CrystalStructuresPath({k_GeomName, k_EnsembleName, "CrystalStructures"});

// Test dimensions
constexpr usize k_SmallDim = 15;
constexpr usize k_SmallBlockSize = 5;
constexpr usize k_LargeDim = 200;
constexpr usize k_LargeBlockSize = 25;

/**
 * @brief Populates CAxisSegmentFeaturesFilter arguments.
 */
void SetupArgs(Arguments& args, bool useMask, float32 tolerance = 5.0f, ChoicesParameter::ValueType neighborScheme = 0, bool randomize = false)
{
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(tolerance));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(useMask ? k_MaskPath : DataPath{}));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(std::string(k_FeatureDataName)));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(randomize));
}
} // namespace

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: Small Correctness", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  // Quats float32 4-comp => 15*15*4*4 = 3,600 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 3600, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_SmallExemplarFile);

  std::string testName = GENERATE("Base", "Masked");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    const bool useMask = (testName == "Masked");
    const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
    const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

    DataStructure dataStructure;
    auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_GeomName), std::string(k_CellDataName));
    auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_GeomPath);
    BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 0, k_SmallBlockSize); // Hexagonal_High

    if(useMask)
    {
      BuildSphericalMask(dataStructure, cellShape, am->getId());
    }

    CAxisSegmentFeaturesFilter filter;
    Arguments args;
    SetupArgs(args, useMask);

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
    CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath),
                             dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath));
    CompareDataArrays<uint8>(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath),
                             dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: 200x200x200 Large OOC", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // Quats float32 4-comp => 200*200*4*4 = 640,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 640000, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_LargeExemplarFile);

  const ShapeType cellShape = {k_LargeDim, k_LargeDim, k_LargeDim};
  const std::array<usize, 3> dims = {k_LargeDim, k_LargeDim, k_LargeDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_GeomName), std::string(k_CellDataName));
  auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_GeomPath);
  BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 0, k_LargeBlockSize); // Hexagonal_High
  BuildSphericalMask(dataStructure, cellShape, am->getId());

  CAxisSegmentFeaturesFilter filter;
  Arguments args;
  SetupArgs(args, /*useMask=*/true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath exemplarFeatureIdsPath({"DataContainer_Exemplar", std::string(k_CellDataName), "FeatureIds"});
  const DataPath exemplarActivePath({"DataContainer_Exemplar", std::string(k_FeatureDataName), "Active"});

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath));
  CompareDataArrays<int32>(exemplarDS.getDataRefAs<Int32Array>(exemplarFeatureIdsPath),
                           dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath));
  CompareDataArrays<uint8>(exemplarDS.getDataRefAs<UInt8Array>(exemplarActivePath),
                           dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: No Valid Voxels Returns Error", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  RunNoValidVoxelsErrorTest<CAxisSegmentFeaturesFilter>([](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, const DataPath& maskPath) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
    BuildOrientationTestData(ds, cellShape, geom.getId(), am.getId(), 0, 3); // Hexagonal_High

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Quats")));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Phases")));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geom", "CellEnsembleData", "CrystalStructures"})));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("Grain Data"));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  });
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: Randomize Feature IDs", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  constexpr usize k_ExpectedFeatures = 3; // 3 Z-layers with 1 merge-pair pillar
  const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
  const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_GeomName), std::string(k_CellDataName));
  auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_GeomPath);
  BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 0, k_SmallBlockSize); // Hexagonal_High

  CAxisSegmentFeaturesFilter filter;
  Arguments args;
  SetupArgs(args, /*useMask=*/false, /*tolerance=*/5.0f, /*neighborScheme=*/0, /*randomize=*/true);

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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: High Tolerance Merges All", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
  const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_GeomName), std::string(k_CellDataName));
  auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_GeomPath);
  BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 0, k_SmallBlockSize); // Hexagonal_High

  CAxisSegmentFeaturesFilter filter;
  Arguments args;
  SetupArgs(args, /*useMask=*/false, /*tolerance=*/90.0f);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // With tolerance=90 degrees, all C-axis directions on the hemisphere merge into 1 feature
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
  const auto& actives = dataStructure.getDataRefAs<UInt8Array>(k_ActivePath);
  REQUIRE(actives.getNumberOfTuples() == 2); // 1 feature + index 0

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();
  for(usize i = 0; i < featureStore.getNumberOfTuples(); i++)
  {
    REQUIRE(featureStore.getValue(i) == 1);
  }
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: Generate Test Data", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/caxis_segment_features", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  // Small input data (15^3) — one geometry per test variant
  {
    const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
    const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

    DataStructure ds;

    auto* amBase = BuildSegmentFeaturesTestGeometry(ds, dims, "Base", std::string(k_CellDataName));
    auto& geomBase = ds.getDataRefAs<ImageGeom>(DataPath({"Base"}));
    BuildOrientationTestData(ds, cellShape, geomBase.getId(), amBase->getId(), 0, k_SmallBlockSize); // Hexagonal_High

    auto* amMasked = BuildSegmentFeaturesTestGeometry(ds, dims, "Masked", std::string(k_CellDataName));
    auto& geomMasked = ds.getDataRefAs<ImageGeom>(DataPath({"Masked"}));
    BuildOrientationTestData(ds, cellShape, geomMasked.getId(), amMasked->getId(), 0, k_SmallBlockSize);
    BuildSphericalMask(ds, cellShape, amMasked->getId());

    UnitTest::WriteTestDataStructure(ds, outputDir / "small_input.dream3d");
  }

  // Large input data (200^3) — mask=true
  {
    const ShapeType cellShape = {k_LargeDim, k_LargeDim, k_LargeDim};
    const std::array<usize, 3> dims = {k_LargeDim, k_LargeDim, k_LargeDim};

    DataStructure ds;
    auto* am = BuildSegmentFeaturesTestGeometry(ds, dims, std::string(k_GeomName), std::string(k_CellDataName));
    auto& geom = ds.getDataRefAs<ImageGeom>(k_GeomPath);
    BuildOrientationTestData(ds, cellShape, geom.getId(), am->getId(), 0, k_LargeBlockSize); // Hexagonal_High
    BuildSphericalMask(ds, cellShape, am->getId());

    UnitTest::WriteTestDataStructure(ds, outputDir / "large_input.dream3d");
  }
}
