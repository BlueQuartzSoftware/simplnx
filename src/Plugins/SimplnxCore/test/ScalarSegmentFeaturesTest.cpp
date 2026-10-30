#include "SimplnxCore/Filters/Algorithms/ScalarSegmentFeatures.hpp"
#include "SimplnxCore/Filters/ScalarSegmentFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/SegmentFeaturesTestUtils.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>
#include <set>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

namespace
{
// These paths select the segmentation exemplar archive.
const std::string k_ArchiveName = "segment_features_exemplars.tar.gz";
const std::string k_DataDirName = "segment_features_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_SmallExemplarFile = k_DataDir / "scalar_small.dream3d";
const fs::path k_LargeExemplarFile = k_DataDir / "scalar_large.dream3d";

// These names define the generated geometry hierarchy.
constexpr StringLiteral k_GeomName = "DataContainer";
constexpr StringLiteral k_CellDataName = "CellData";
constexpr StringLiteral k_FeatureDataName = "CellFeatureData";

// These paths select generated segmentation arrays.
const DataPath k_GeomPath({k_GeomName});
const DataPath k_FeatureIdsPath({k_GeomName, k_CellDataName, "FeatureIds"});
const DataPath k_ActivePath({k_GeomName, k_FeatureDataName, "Active"});
const DataPath k_MaskPath({k_GeomName, k_CellDataName, "Mask"});

// These dimensions define the small and large generated fixtures.
constexpr usize k_SmallDim = 15;
constexpr usize k_SmallBlockSize = 5;
constexpr usize k_LargeDim = 200;
constexpr usize k_LargeBlockSize = 25;

/**
 * @brief Populates ScalarSegmentFeaturesFilter arguments.
 * @param args Receives the filter arguments.
 * @param useMask True to use k_MaskPath.
 * @param isPeriodic True to connect cells across opposite geometry boundaries.
 * @param tolerance Maximum scalar difference within one feature.
 * @param neighborScheme 0 for face neighbors or 1 for all connected neighbors.
 * @param randomize True to randomize output feature identifiers.
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

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: FaceEdgeVertex Connectivity", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // The shared test verifies vertex-connected and edge-connected region pairs.
  // Its setup callable creates four isolated cells and configures FaceEdgeVertex connectivity.
  auto setupScalar = [](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, ChoicesParameter::ValueType neighborScheme) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    const DataPath scalarPath = cellDataPath.createChildPath("ScalarData");
    auto scalarDS = DataStoreUtilities::CreateDataStore<int32>(ds, scalarPath, cellShape, {1});
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

  RunFaceEdgeVertexConnectivityTest<ScalarSegmentFeaturesFilter>(
      scope, [&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupScalar(args, ds, gp, cp, 0); },
      [&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupScalar(args, ds, gp, cp, 1); });
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: No Valid Voxels Returns Error", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  RunNoValidVoxelsErrorTest<ScalarSegmentFeaturesFilter>(scope, [](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, const DataPath& maskPath) {
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
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

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
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  // Each small test variant uses a separate 15-cubed geometry.
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

  // The large 200-cubed fixture enables its mask and periodic boundaries.
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

namespace
{
const std::string k_SharedEdgesInputArrayName = "Shared Edges";
const std::string k_SharedPointsInputArrayName = "Shared Points";
const std::string k_NothingSharedInputArrayName = "Nothing Shared";
const std::string k_CombinationInputArrayName = "Combination";
const std::string k_ExemplarySharedEdgesFaceOnlyFeatureIdsName = "Exemplary Shared Edges FeatureIds - Face Only";
const std::string k_ExemplarySharedEdgesAllConnectedFeatureIdsName = "Exemplary Shared Edges FeatureIds - All Connected";
const std::string k_ExemplarySharedPointsFaceOnlyFeatureIdsName = "Exemplary Shared Points FeatureIds - Face Only";
const std::string k_ExemplarySharedPointsAllConnectedFeatureIdsName = "Exemplary Shared Points FeatureIds - All Connected";
const std::string k_ExemplaryNothingSharedFaceOnlyFeatureIdsName = "Exemplary Nothing Shared FeatureIds - Face Only";
const std::string k_ExemplaryNothingSharedAllConnectedFeatureIdsName = "Exemplary Nothing Shared FeatureIds - All Connected";
const std::string k_ExemplaryCombinationFaceOnlyFeatureIdsName = "Exemplary Combination FeatureIds - Face Only";
const std::string k_ExemplaryCombinationAllConnectedFeatureIdsName = "Exemplary Combination FeatureIds - All Connected";
} // namespace

TEST_CASE("SimplnxCore::ScalarSegmentFeatures", "[SimplnxCore][ScalarSegmentFeatures]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  // Load the Small IN100 input before scalar segmentation.
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    Arguments args;
    ScalarSegmentFeaturesFilter filter;

    DataPath smallIn100Group({k_DataContainer});
    DataPath ebsdScanDataPath = smallIn100Group.createChildPath(k_CellData);
    DataPath inputDataArrayPath = ebsdScanDataPath.createChildPath(k_FeatureIds);
    std::string outputFeatureIdsName = "Output_Feature_Ids";
    std::string computedCellDataName = "Computed_CellData";
    DataPath outputFeatureIdsPath = ebsdScanDataPath.createChildPath(outputFeatureIdsName);
    DataPath featureDataGroupPath = smallIn100Group.createChildPath(computedCellDataName);
    DataPath activeArrayDataPath = featureDataGroupPath.createChildPath(k_ActiveName);

    DataPath gridGeomDataPath({k_DataContainer});
    int scalarTolerance = 0;

    // Configure the unmasked scalar segmentation and its output hierarchy.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(inputDataArrayPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(scalarTolerance));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>(outputFeatureIdsName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>(computedCellDataName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    // Randomization verifies the optional feature-identifier permutation.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(true));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(activeArrayDataPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 847);
  }

  {
    // This output supports manual inspection of the segmented arrays.
    std::string filePath = fmt::format("{}/ScalarSegmentFeatures.dream3d", unit_test::k_BinaryTestOutputDir);
    // std::cout << "Writing file to: " << filePath << std::endl;
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(filePath);

    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Neighbor Scheme", "[Reconstruction][ScalarSegmentFeatures]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  /*
   * Catch2 runs the remainder of this test once for each generated parameter tuple.
   */
  auto [sectionName, inputDataArrayName, exemplaryFeatureIdsArrayName, neighborSchemeIndex] =
      GENERATE(std::make_tuple("Shared Edges - Face Only", k_SharedEdgesInputArrayName, k_ExemplarySharedEdgesFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Shared Edges - All Connected", k_SharedEdgesInputArrayName, k_ExemplarySharedEdgesAllConnectedFeatureIdsName, 1),
               std::make_tuple("Shared Points - Face Only", k_SharedPointsInputArrayName, k_ExemplarySharedPointsFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Shared Points - All Connected", k_SharedPointsInputArrayName, k_ExemplarySharedPointsAllConnectedFeatureIdsName, 1),
               std::make_tuple("Nothing Shared - Face Only", k_NothingSharedInputArrayName, k_ExemplaryNothingSharedFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Nothing Shared - All Connected", k_NothingSharedInputArrayName, k_ExemplaryNothingSharedAllConnectedFeatureIdsName, 1),
               std::make_tuple("Combination - Face Only", k_CombinationInputArrayName, k_ExemplaryCombinationFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Combination - All Connected", k_CombinationInputArrayName, k_ExemplaryCombinationAllConnectedFeatureIdsName, 1));

  /*
   * Each tuple selects one input layout and one neighbor-connectivity mode.
   */
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_neighbor_scheme_test.tar.gz", "segment_features_neighbor_scheme_test");
  auto baseDataFilePath = fs::path(fmt::format("{}/segment_features_neighbor_scheme_test/segment_features_neighbor_scheme_test.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    Arguments args;
    ScalarSegmentFeaturesFilter filter;

    DataPath smallIn100Group({k_SmallIn100ImageGeom});
    DataPath ebsdScanDataPath = smallIn100Group.createChildPath(k_Cell_Data);
    std::string outputFeatureIdsName = "Output_Feature_Ids";
    std::string computedCellDataName = "Computed_CellData";
    DataPath outputFeatureIdsPath = ebsdScanDataPath.createChildPath(outputFeatureIdsName);
    DataPath featureDataGroupPath = smallIn100Group.createChildPath(computedCellDataName);
    DataPath activeArrayDataPath = featureDataGroupPath.createChildPath(k_ActiveName);

    DataPath gridGeomDataPath({k_SmallIn100ImageGeom});
    int scalarTolerance = 0;

    // Configure the common output hierarchy for each generated scenario.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(scalarTolerance));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>(outputFeatureIdsName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>(computedCellDataName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));

    SECTION(sectionName)
    {
      DataPath inputDataArrayPath = ebsdScanDataPath.createChildPath(inputDataArrayName);
      DataPath exemplaryFeatureIdsArrayPath = ebsdScanDataPath.createChildPath(exemplaryFeatureIdsArrayName);
      DataPath computedFeatureIdsPath = ebsdScanDataPath.createChildPath(outputFeatureIdsName);
      args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(inputDataArrayPath));
      args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborSchemeIndex));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      UnitTest::CompareArrays<int32>(dataStructure, exemplaryFeatureIdsArrayPath, computedFeatureIdsPath);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Masked Voxel 0 Seed Validation", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // getSeed() must validate and stamp the first index exactly like each later seed.
  // The 5 by 1 by 1 fixture masks index 0 and uses values [9, 5, 5, 9, 7].
  // Tolerance 1 produces features {1, 2} and {4}. Masked cells keep identifier 0.
  // Starting directly at raw index 0 would create an empty feature and shift later identifiers.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
  imageGeom->setDimensions({5, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 5}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* scalarArrayPtr = CreateTestDataArray<int32>(dataStructure, "ScalarValues", ShapeType{1, 1, 5}, {1}, cellAM->getId());
  auto* maskArrayPtr = CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 5}, {1}, cellAM->getId());

  const std::vector<int32> scalarValues = {9, 5, 5, 9, 7};
  const std::vector<bool> maskValues = {false, true, true, false, true};
  for(usize cellIdx = 0; cellIdx < scalarValues.size(); cellIdx++)
  {
    (*scalarArrayPtr)[cellIdx] = scalarValues[cellIdx];
    (*maskArrayPtr)[cellIdx] = maskValues[cellIdx];
  }

  ScalarSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int32>(1));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "ScalarValues"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})));
  const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})).getDataStoreRef();
  const std::vector<int32> expectedFeatureIds = {0, 1, 1, 0, 2};
  for(usize cellIdx = 0; cellIdx < expectedFeatureIds.size(); cellIdx++)
  {
    INFO(fmt::format("cell index {}", cellIdx));
    REQUIRE(featureIdsRef[cellIdx] == expectedFeatureIds[cellIdx]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(DataPath({"Geometry", "CellFeatureData", "Active"})));
  REQUIRE(dataStructure.getDataRefAs<UInt8Array>(DataPath({"Geometry", "CellFeatureData", "Active"})).getNumberOfTuples() == 3);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Periodic Boundary Wrap", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  // The 4 by 1 by 1 fixture requires IsPeriodic to change boundary connectivity.
  // Without wrapping, values [5, 9, 9, 5] produce regions {0}, {1, 2}, and {3}.
  // With wrapping, the end cells join and produce regions {0, 3} and {1, 2}.
  // Face and FaceEdgeVertex modes must apply the same boundary wrap.
  auto runFilter = [scenario](bool isPeriodic, ChoicesParameter::ValueType neighborScheme) -> std::vector<int32> {
    UnitTest::AlgorithmTestScope scope(scenario);
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
    imageGeom->setDimensions({4, 1, 1});
    auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 4}, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    auto* scalarArrayPtr = CreateTestDataArray<int32>(dataStructure, "ScalarValues", ShapeType{1, 1, 4}, {1}, cellAM->getId());
    const std::vector<int32> scalarValues = {5, 9, 9, 5};
    for(usize cellIdx = 0; cellIdx < scalarValues.size(); cellIdx++)
    {
      (*scalarArrayPtr)[cellIdx] = scalarValues[cellIdx];
    }

    ScalarSegmentFeaturesFilter filter;
    Arguments args;
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int32>(1));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "ScalarValues"})));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>("CellFeatureData"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})));
    const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})).getDataStoreRef();
    std::vector<int32> featureIds(featureIdsRef.getNumberOfTuples());
    for(usize cellIdx = 0; cellIdx < featureIds.size(); cellIdx++)
    {
      featureIds[cellIdx] = featureIdsRef[cellIdx];
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
    return featureIds;
  };

  const std::vector<std::string> schemeNames = {"Face Neighbors", "All Connected Neighbors"};
  for(ChoicesParameter::ValueType neighborScheme = 0; neighborScheme < 2; neighborScheme++)
  {
    DYNAMIC_SECTION(schemeNames[neighborScheme])
    {
      REQUIRE(runFilter(false, neighborScheme) == std::vector<int32>{1, 2, 2, 3});
      REQUIRE(runFilter(true, neighborScheme) == std::vector<int32>{1, 2, 2, 1});
    }
  }
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Execute Error - All Cells Masked (-87000)", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // No valid seed exists when the mask excludes every cell, so execution must return -87000.
  // Starting directly at raw index 0 would incorrectly create an empty feature.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
  imageGeom->setDimensions({3, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 3}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* scalarArrayPtr = CreateTestDataArray<int32>(dataStructure, "ScalarValues", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  auto* maskArrayPtr = CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  for(usize cellIdx = 0; cellIdx < 3; cellIdx++)
  {
    (*scalarArrayPtr)[cellIdx] = 5;
    (*maskArrayPtr)[cellIdx] = false;
  }

  ScalarSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int32>(1));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "ScalarValues"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -87000);
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Geometry cell tuple count", "[SimplnxCore][ScalarSegmentFeatures][TupleCount]")
{
  UnitTest::LoadPlugins();
  using FilterType = ScalarSegmentFeaturesFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  // A disabled mask must not be checked; an enabled mask must match the geometry.
  const int maskMode = GENERATE(0, 1, 2);
  CAPTURE(maskMode);
  const usize maskTuples = maskMode == 1 ? tuples : 5;

  // A matching input in another Attribute Matrix must remain valid.
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 1), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});
  auto* maskAMPtr = AttributeMatrix::Create(dataStructure, "Mask Data", ShapeType{maskTuples});
  REQUIRE(maskAMPtr != nullptr);
  REQUIRE(DataArray<uint8>::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{maskTuples}, ShapeType{1}, 1), maskAMPtr->getId()) != nullptr);
  const DataPath maskPath({"Mask Data", "Mask"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_GridGeomPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_InputArrayPathKey, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(FilterType::k_UseMask_Key, std::make_any<bool>(maskMode != 0));
  args.insertOrAssign(FilterType::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12 && maskMode != 2)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -654);
    const DataPath expectedPath = tuples == 12 ? maskPath : inputPath;
    const usize expectedTuples = tuples == 12 ? maskTuples : tuples;
    REQUIRE(errorRef.message.find(expectedPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", expectedTuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

TEMPLATE_TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Exact 64-bit Integer Differences", "[SimplnxCore][ScalarSegmentFeatures][IntegerPrecision]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool alongZ = GENERATE(false, true);
  const auto neighborScheme = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  CAPTURE(alongZ, neighborScheme);

  constexpr TestType k_Large = TestType{1} << 53;
  constexpr TestType k_Min = std::numeric_limits<TestType>::min();
  constexpr TestType k_Max = std::numeric_limits<TestType>::max();
  constexpr int32 k_MaxTolerance = std::numeric_limits<int32>::max();
  struct Fixture
  {
    std::array<TestType, 3> values;
    int32 tolerance;
    std::array<int32, 3> expectedIds;
  };
  // Hand oracle: unequal neighbors stay separate at tolerance zero; the repeated
  // pair joins. Differences of one join at tolerance one. The full type range
  // exceeds one, while max - (max - INT32_MAX) equals INT32_MAX exactly.
  const std::array<Fixture, 8> fixtures = {{
      {{k_Large, k_Large + 1, k_Large + 1}, 0, {1, 2, 2}},
      {{k_Large + 1, k_Large, k_Large}, 0, {1, 2, 2}},
      {{k_Max, k_Max - 1, k_Max - 1}, 0, {1, 2, 2}},
      {{k_Min, k_Min + 1, k_Min + 1}, 0, {1, 2, 2}},
      {{k_Min, k_Max, k_Max}, 0, {1, 2, 2}},
      {{k_Large, k_Large + 1, k_Large + 1}, 1, {1, 1, 1}},
      {{k_Max, k_Min, k_Min}, 1, {1, 2, 2}},
      {{k_Max, k_Max - k_MaxTolerance, k_Max - k_MaxTolerance}, k_MaxTolerance, {1, 1, 1}},
  }};
  for(usize fixtureIndex = 0; fixtureIndex < fixtures.size(); fixtureIndex++)
  {
    DYNAMIC_SECTION("Fixture " << fixtureIndex)
    {
      const auto& fixture = fixtures[fixtureIndex];
      CAPTURE(fixture.values, fixture.tolerance);
      DataStructure dataStructure;
      auto* imageGeom = ImageGeom::Create(dataStructure, std::string(k_GeomName));
      REQUIRE(imageGeom != nullptr);
      imageGeom->setDimensions(alongZ ? SizeVec3{1, 1, 3} : SizeVec3{3, 1, 1});
      const ShapeType cellShape = alongZ ? ShapeType{3, 1, 1} : ShapeType{1, 1, 3};
      auto* cellAM = AttributeMatrix::Create(dataStructure, std::string(k_CellDataName), cellShape, imageGeom->getId());
      REQUIRE(cellAM != nullptr);
      imageGeom->setCellData(*cellAM);
      const DataPath scalarPath = k_GeomPath.createChildPath(k_CellDataName).createChildPath("ScalarData");
      auto scalarStore = DataStoreUtilities::CreateDataStore<TestType>(dataStructure, scalarPath, cellShape, {1});
      auto* scalarArray = DataArray<TestType>::Create(dataStructure, "ScalarData", scalarStore, cellAM->getId());
      REQUIRE(scalarArray != nullptr);
      auto writeResult = scalarStore->copyFromBuffer(0, nonstd::span<const TestType>(fixture.values.data(), fixture.values.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

      ScalarSegmentFeaturesFilter filter;
      Arguments args;
      SetupArgs(args, false, false, fixture.tolerance, neighborScheme, false);
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
      const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
      std::array<int32, 3> actualIds = {};
      auto readResult = featureIds.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actualIds.data(), actualIds.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(readResult);
      CHECK(actualIds == fixture.expectedIds);
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
      CHECK(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath).getNumberOfTuples() == static_cast<usize>(fixture.expectedIds.back() + 1));

      // The public CCL entry point remains callable after operator() releases its
      // slice buffers. Its native comparator must obey the same integer oracle.
      ScalarSegmentFeaturesInputValues inputValues{};
      inputValues.ScalarTolerance = fixture.tolerance;
      inputValues.NeighborScheme = static_cast<SegmentFeatures::NeighborScheme>(neighborScheme);
      inputValues.ImageGeometryPath = k_GeomPath;
      inputValues.InputDataPath = scalarPath;
      inputValues.FeatureIdsArrayPath = k_FeatureIdsPath;
      inputValues.CellFeatureAttributeMatrixPath = k_ActivePath.getParent();
      inputValues.ActiveArrayPath = k_ActivePath;
      const std::atomic_bool shouldCancel = false;
      const IFilter::MessageHandler messageHandler;
      ScalarSegmentFeatures algorithm(dataStructure, &inputValues, shouldCancel, messageHandler);
      auto initializeResult = scope.execute([&] { return algorithm(); });
      SIMPLNX_RESULT_REQUIRE_VALID(initializeResult);
      auto& featureIdsStoreRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
      auto unbufferedResult = scope.execute([&] { return algorithm.executeCCL(imageGeom, featureIdsStoreRef); });
      SIMPLNX_RESULT_REQUIRE_VALID(unbufferedResult);
      auto unbufferedReadResult = featureIdsStoreRef.copyIntoBuffer(0, nonstd::span<int32>(actualIds.data(), actualIds.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(unbufferedReadResult);
      INFO("Public CCL after slice-buffer release");
      CHECK(actualIds == fixture.expectedIds);
      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}
