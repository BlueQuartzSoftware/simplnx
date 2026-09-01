#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/SegmentFeaturesTestUtils.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <fmt/format.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_ArchiveName = "segment_features_exemplars.tar.gz";
const std::string k_DataDirName = "segment_features_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_SmallExemplarFile = k_DataDir / "ebsd_small.dream3d";
const fs::path k_LargeExemplarFile = k_DataDir / "ebsd_large.dream3d";

constexpr StringLiteral k_GeomName = "DataContainer";
constexpr StringLiteral k_CellDataName = "CellData";
constexpr StringLiteral k_FeatureDataName = "CellFeatureData";
constexpr StringLiteral k_EnsembleName = "CellEnsembleData";

const DataPath k_GeomPath({k_GeomName});
const DataPath k_FeatureIdsPath({k_GeomName, k_CellDataName, "FeatureIds"});
const DataPath k_ActivePath({k_GeomName, k_FeatureDataName, "Active"});
const DataPath k_MaskPath({k_GeomName, k_CellDataName, "Mask"});
const DataPath k_QuatsPath({k_GeomName, k_CellDataName, "Quats"});
const DataPath k_PhasesPath({k_GeomName, k_CellDataName, "Phases"});
const DataPath k_CrystalStructuresPath({k_GeomName, k_EnsembleName, "CrystalStructures"});

constexpr usize k_SmallDim = 15;
constexpr usize k_SmallBlockSize = 5;
constexpr usize k_LargeDim = 200;
constexpr usize k_LargeBlockSize = 25;

void SetupArgs(Arguments& args, bool useMask, bool isPeriodic = false, float32 tolerance = 5.0f, ChoicesParameter::ValueType neighborScheme = 0, bool randomize = false)
{
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(tolerance));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(useMask ? k_MaskPath : DataPath{}));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(std::string(k_FeatureDataName)));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(randomize));
}
} // namespace

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures: No Valid Voxels Returns Error", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  RunNoValidVoxelsErrorTest<EBSDSegmentFeaturesFilter>([](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, const DataPath& maskPath) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
    BuildOrientationTestData(ds, cellShape, geom.getId(), am.getId(), 1, 3); // Cubic_High

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Quats")));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Phases")));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geom", "CellEnsembleData", "CrystalStructures"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("Grain Data"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  });
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures: Randomize Feature IDs", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // Three Z layers with one merge-pair pillar yield three features.
  constexpr usize k_ExpectedFeatures = 3;
  const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
  const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_GeomName), std::string(k_CellDataName));
  auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_GeomPath);
  BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 1, k_SmallBlockSize); // Cubic_High

  EBSDSegmentFeaturesFilter filter;
  Arguments args;
  SetupArgs(args, /*useMask=*/false, /*isPeriodic=*/false, /*tolerance=*/5.0f, /*neighborScheme=*/0, /*randomize=*/true);

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

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures: High Tolerance Merges All", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
  const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_GeomName), std::string(k_CellDataName));
  auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_GeomPath);
  BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 1, k_SmallBlockSize); // Cubic_High

  EBSDSegmentFeaturesFilter filter;
  Arguments args;
  SetupArgs(args, /*useMask=*/false, /*isPeriodic=*/false, /*tolerance=*/90.0f);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Cubic misorientations do not exceed 62.8 degrees, so tolerance 90 merges
  // all valid cells.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
  const auto& actives = dataStructure.getDataRefAs<UInt8Array>(k_ActivePath);
  REQUIRE(actives.getNumberOfTuples() == 2); // Feature index zero is reserved.

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();
  for(usize i = 0; i < featureStore.getNumberOfTuples(); i++)
  {
    REQUIRE(featureStore.getValue(i) == 1);
  }
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures: FaceEdgeVertex Connectivity", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // The fixture differentiates face, edge, and vertex connectivity at a
  // 5-degree tolerance.
  constexpr float32 k_DegToRad = 3.14159265358979323846f / 180.0f;

  auto setupEBSD = [&](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, ChoicesParameter::ValueType neighborScheme) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);

    // Background cells use 60-degree X rotations. Paired cells use identity
    // quaternions in EbsdLib x,y,z,w order.
    const float32 bgHalf = 60.0f * k_DegToRad * 0.5f;
    auto quatsDS = DataStoreUtilities::CreateDataStore<float32>(ds, cellDataPath.createChildPath("Quats"), cellShape, {4}, IDataAction::Mode::Execute);
    auto* quatsArr = DataArray<float32>::Create(ds, "Quats", quatsDS, am.getId());
    auto& quatsStore = quatsArr->getDataStoreRef();
    for(usize i = 0; i < 27; i++)
    {
      quatsStore[i * 4 + 0] = std::sin(bgHalf);
      quatsStore[i * 4 + 1] = 0.0f;
      quatsStore[i * 4 + 2] = 0.0f;
      quatsStore[i * 4 + 3] = std::cos(bgHalf);
    }
    for(usize idx : {static_cast<usize>(0), static_cast<usize>(1 * 9 + 1 * 3 + 1)})
    {
      quatsStore[idx * 4 + 0] = 0.0f;
      quatsStore[idx * 4 + 1] = 0.0f;
      quatsStore[idx * 4 + 2] = 0.0f;
      quatsStore[idx * 4 + 3] = 1.0f;
    }
    const float32 pairHalf = 30.0f * k_DegToRad * 0.5f;
    for(usize idx : {static_cast<usize>(0 * 9 + 0 * 3 + 2), static_cast<usize>(1 * 9 + 1 * 3 + 2)})
    {
      quatsStore[idx * 4 + 0] = std::sin(pairHalf);
      quatsStore[idx * 4 + 1] = 0.0f;
      quatsStore[idx * 4 + 2] = 0.0f;
      quatsStore[idx * 4 + 3] = std::cos(pairHalf);
    }

    auto phasesDS = DataStoreUtilities::CreateDataStore<int32>(ds, cellDataPath.createChildPath("Phases"), cellShape, {1}, IDataAction::Mode::Execute);
    auto* phasesArr = DataArray<int32>::Create(ds, "Phases", phasesDS, am.getId());
    phasesArr->fill(1);

    const ShapeType ensShape = {2};
    auto* ensAM = AttributeMatrix::Create(ds, "CellEnsembleData", ensShape, geom.getId());
    const DataPath crystStructsPath = geomPath.createChildPath("CellEnsembleData").createChildPath("CrystalStructures");
    auto crystDS = DataStoreUtilities::CreateDataStore<uint32>(ds, crystStructsPath, ensShape, {1}, IDataAction::Mode::Execute);
    auto* crystArr = DataArray<uint32>::Create(ds, "CrystalStructures", crystDS, ensAM->getId());
    auto& crystStore = crystArr->getDataStoreRef();
    crystStore[0] = 999;
    crystStore[1] = 1;

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Quats")));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Phases")));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geom", "CellEnsembleData", "CrystalStructures"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  };

  RunFaceEdgeVertexConnectivityTest<EBSDSegmentFeaturesFilter>([&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupEBSD(args, ds, gp, cp, 0); },
                                                               [&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupEBSD(args, ds, gp, cp, 1); });
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures: Generate Test Data", "[OrientationAnalysis][EBSDSegmentFeatures][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/ebsd_segment_features", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  // The small variants cover base, mask, and periodic configurations.
  {
    const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
    const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

    DataStructure ds;

    auto* amBase = BuildSegmentFeaturesTestGeometry(ds, dims, "Base", std::string(k_CellDataName));
    auto& geomBase = ds.getDataRefAs<ImageGeom>(DataPath({"Base"}));
    BuildOrientationTestData(ds, cellShape, geomBase.getId(), amBase->getId(), 1, k_SmallBlockSize);

    auto* amMasked = BuildSegmentFeaturesTestGeometry(ds, dims, "Masked", std::string(k_CellDataName));
    auto& geomMasked = ds.getDataRefAs<ImageGeom>(DataPath({"Masked"}));
    BuildOrientationTestData(ds, cellShape, geomMasked.getId(), amMasked->getId(), 1, k_SmallBlockSize);
    BuildSphericalMask(ds, cellShape, amMasked->getId());

    auto* amPeriodic = BuildSegmentFeaturesTestGeometry(ds, dims, "Periodic", std::string(k_CellDataName));
    auto& geomPeriodic = ds.getDataRefAs<ImageGeom>(DataPath({"Periodic"}));
    BuildOrientationTestData(ds, cellShape, geomPeriodic.getId(), amPeriodic->getId(), 1, k_SmallBlockSize, true); // wrapBoundary

    UnitTest::WriteTestDataStructure(ds, outputDir / "small_input.dream3d");
  }

  // The large periodic fixture omits a mask so boundary cells can exercise
  // wrapping.
  {
    const ShapeType cellShape = {k_LargeDim, k_LargeDim, k_LargeDim};
    const std::array<usize, 3> dims = {k_LargeDim, k_LargeDim, k_LargeDim};

    DataStructure ds;
    auto* am = BuildSegmentFeaturesTestGeometry(ds, dims, std::string(k_GeomName), std::string(k_CellDataName));
    auto& geom = ds.getDataRefAs<ImageGeom>(k_GeomPath);
    BuildOrientationTestData(ds, cellShape, geom.getId(), am->getId(), 1, k_LargeBlockSize, true); // wrapBoundary

    UnitTest::WriteTestDataStructure(ds, outputDir / "large_input.dream3d");
  }
}

namespace ebsd_segment_features_constants
{
inline constexpr StringLiteral k_InputGeometryName = "DataContainer";
inline const DataPath k_InputGeometryPath({k_InputGeometryName});
inline constexpr StringLiteral k_CellDataName = "CellData";
inline constexpr StringLiteral k_EnsembleName = "CellEnsembleData";
inline const DataPath k_QuatsArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Quats");
inline const DataPath k_PhasesArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Phases");
inline const DataPath k_MaskArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Mask (Y Pos)");

inline const DataPath k_CrystalStructuresArrayPath = k_InputGeometryPath.createChildPath(k_EnsembleName).createChildPath("CrystalStructures");

inline const DataPath k_ActivesArrayPath = k_InputGeometryPath.createChildPath(k_Grain_Data).createChildPath(k_ActiveName);

inline const DataPath k_FeatureIdsArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath(k_FeatureIds);

inline const DataPath k_FeatureIdsFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_Face");
inline const DataPath k_FeatureIdsAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_All");
inline const DataPath k_FeatureIdsMaskFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_Mask_Face");
inline const DataPath k_FeatureIdsMaskAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_Mask_All");
} // namespace ebsd_segment_features_constants

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:Face", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 83);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:All", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 77);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:MaskFace", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 36);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsMaskFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:MaskAll", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 32);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsMaskAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][EBSDSegmentFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "EBSDSegmentFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "EBSDSegmentFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<EBSDSegmentFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<bool>(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key) == true);
      }
      CHECK(args.value<float32>(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key) == 2.5f);
      CHECK(args.value<bool>(EBSDSegmentFeaturesFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: Masked Voxel 0 Seed Validation", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // getSeed() must reject the masked first cell. Otherwise, the driver creates
  // a phantom feature and shifts later identifiers.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
  imageGeom->setDimensions({5, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 5}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* quatsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", ShapeType{1, 1, 5}, {4}, cellAM->getId());
  auto* phasesArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", ShapeType{1, 1, 5}, {1}, cellAM->getId());
  auto* maskArrayPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 5}, {1}, cellAM->getId());
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, imageGeom->getId());
  auto* crystalStructuresArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

  const std::vector<float32> phiDegrees = {0.0f, 20.0f, 22.0f, 0.0f, 90.0f};
  const std::vector<bool> maskValues = {false, true, true, false, true};
  for(usize cellIdx = 0; cellIdx < phiDegrees.size(); cellIdx++)
  {
    const float32 halfAngleRad = (phiDegrees[cellIdx] * 0.5f) * Constants::k_PiOver180F;
    (*quatsArrayPtr)[cellIdx * 4 + 0] = std::sin(halfAngleRad);
    (*quatsArrayPtr)[cellIdx * 4 + 1] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 2] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 3] = std::cos(halfAngleRad);
    (*phasesArrayPtr)[cellIdx] = 1;
    (*maskArrayPtr)[cellIdx] = maskValues[cellIdx];
  }
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

  EBSDSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(10.0F));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Quats"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Phases"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));

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

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: Periodic Boundary Wrap", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // Periodic mode must join the compatible first and last cells across the x
  // boundary.
  auto runFilter = [](bool isPeriodic) -> std::vector<int32> {
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
    imageGeom->setDimensions({4, 1, 1});
    auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 4}, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    auto* quatsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", ShapeType{1, 1, 4}, {4}, cellAM->getId());
    auto* phasesArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", ShapeType{1, 1, 4}, {1}, cellAM->getId());
    auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, imageGeom->getId());
    auto* crystalStructuresArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

    const std::vector<float32> phiDegrees = {0.0f, 30.0f, 30.0f, 2.0f};
    for(usize cellIdx = 0; cellIdx < phiDegrees.size(); cellIdx++)
    {
      const float32 halfAngleRad = (phiDegrees[cellIdx] * 0.5f) * Constants::k_PiOver180F;
      (*quatsArrayPtr)[cellIdx * 4 + 0] = std::sin(halfAngleRad);
      (*quatsArrayPtr)[cellIdx * 4 + 1] = 0.0f;
      (*quatsArrayPtr)[cellIdx * 4 + 2] = 0.0f;
      (*quatsArrayPtr)[cellIdx * 4 + 3] = std::cos(halfAngleRad);
      (*phasesArrayPtr)[cellIdx] = 1;
    }
    (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
    (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

    EBSDSegmentFeaturesFilter filter;
    Arguments args;
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(10.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Quats"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Phases"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellEnsembleData", "CrystalStructures"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));

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

  REQUIRE(runFilter(false) == std::vector<int32>{1, 2, 2, 3});
  REQUIRE(runFilter(true) == std::vector<int32>{1, 2, 2, 1});
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: Execute Error - All Cells Masked (-87000)", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // An all-masked fixture has no seed and must return -87000.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
  imageGeom->setDimensions({3, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 3}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* quatsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", ShapeType{1, 1, 3}, {4}, cellAM->getId());
  auto* phasesArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  auto* maskArrayPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, imageGeom->getId());
  auto* crystalStructuresArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

  for(usize cellIdx = 0; cellIdx < 3; cellIdx++)
  {
    (*quatsArrayPtr)[cellIdx * 4 + 0] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 1] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 2] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 3] = 1.0f;
    (*phasesArrayPtr)[cellIdx] = 1;
    (*maskArrayPtr)[cellIdx] = false;
  }
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

  EBSDSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(10.0F));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Quats"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Phases"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -87000);
}
