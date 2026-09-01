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

  // The shared test verifies vertex-connected and edge-connected region pairs.
  // Its setup callable creates four isolated cells and configures FaceEdgeVertex connectivity.
  auto setupScalar = [](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, ChoicesParameter::ValueType neighborScheme) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    const DataPath scalarPath = cellDataPath.createChildPath("ScalarData");
    auto scalarDS = DataStoreUtilities::CreateDataStore<int32>(ds, scalarPath, cellShape, {1}, IDataAction::Mode::Execute);
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

    auto executeResult = filter.execute(dataStructure, args);
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

      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      UnitTest::CompareArrays<int32>(dataStructure, exemplaryFeatureIdsArrayPath, computedFeatureIdsPath);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Masked Voxel 0 Seed Validation", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();

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
  auto executeResult = filter.execute(dataStructure, args);
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

  // The 4 by 1 by 1 fixture requires IsPeriodic to change boundary connectivity.
  // Without wrapping, values [5, 9, 9, 5] produce regions {0}, {1, 2}, and {3}.
  // With wrapping, the end cells join and produce regions {0, 3} and {1, 2}.
  // Face and FaceEdgeVertex modes must apply the same boundary wrap.
  auto runFilter = [](bool isPeriodic, ChoicesParameter::ValueType neighborScheme) -> std::vector<int32> {
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
    auto executeResult = filter.execute(dataStructure, args);
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
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -87000);
}
