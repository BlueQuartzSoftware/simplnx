#include "OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/SegmentFeaturesTestUtils.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_ArchiveName = "segment_features_exemplars.tar.gz";
const std::string k_DataDirName = "segment_features_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_SmallExemplarFile = k_DataDir / "caxis_small.dream3d";
const fs::path k_LargeExemplarFile = k_DataDir / "caxis_large.dream3d";

constexpr StringLiteral k_ExemplarGeomName = "DataContainer";
constexpr StringLiteral k_ExemplarCellDataName = "CellData";
constexpr StringLiteral k_ExemplarFeatureDataName = "CellFeatureData";
constexpr StringLiteral k_ExemplarEnsembleName = "CellEnsembleData";

const DataPath k_ExemplarGeomPath({k_ExemplarGeomName});
const DataPath k_ExemplarFeatureIdsPath({k_ExemplarGeomName, k_ExemplarCellDataName, "FeatureIds"});
const DataPath k_ExemplarActivePath({k_ExemplarGeomName, k_ExemplarFeatureDataName, "Active"});
const DataPath k_ExemplarMaskPath({k_ExemplarGeomName, k_ExemplarCellDataName, "Mask"});
const DataPath k_ExemplarQuatsPath({k_ExemplarGeomName, k_ExemplarCellDataName, "Quats"});
const DataPath k_ExemplarPhasesPath({k_ExemplarGeomName, k_ExemplarCellDataName, "Phases"});
const DataPath k_ExemplarCrystalStructuresPath({k_ExemplarGeomName, k_ExemplarEnsembleName, "CrystalStructures"});

constexpr usize k_SmallDim = 15;
constexpr usize k_SmallBlockSize = 5;
constexpr usize k_LargeDim = 200;
constexpr usize k_LargeBlockSize = 25;

void BuildExemplarArgs(Arguments& args, bool useMask, float32 tolerance = 5.0f, ChoicesParameter::ValueType neighborScheme = 0, bool randomize = false)
{
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(tolerance));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(useMask ? k_ExemplarMaskPath : DataPath{}));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ExemplarGeomPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_ExemplarQuatsPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_ExemplarPhasesPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_ExemplarCrystalStructuresPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(std::string(k_ExemplarFeatureDataName)));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(randomize));
}
} // namespace

namespace caxis_segment_features_constants
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

inline const DataPath k_FeatureIdsFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_Face");
inline const DataPath k_FeatureIdsAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_All");
inline const DataPath k_FeatureIdsMaskFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_Mask_Face");
inline const DataPath k_FeatureIdsMaskAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_Mask_All");
} // namespace caxis_segment_features_constants

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

  // Three Z layers with one merge-pair pillar yield three features.
  constexpr usize k_ExpectedFeatures = 3;
  const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
  const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

  DataStructure dataStructure;
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_ExemplarGeomName), std::string(k_ExemplarCellDataName));
  auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_ExemplarGeomPath);
  BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 0, k_SmallBlockSize); // Hexagonal_High

  CAxisSegmentFeaturesFilter filter;
  Arguments args;
  BuildExemplarArgs(args, /*useMask=*/false, /*tolerance=*/5.0f, /*neighborScheme=*/0, /*randomize=*/true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ExemplarActivePath));
  const auto& actives = dataStructure.getDataRefAs<UInt8Array>(k_ExemplarActivePath);
  REQUIRE(actives.getNumberOfTuples() == k_ExpectedFeatures + 1);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ExemplarFeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_ExemplarFeatureIdsPath);
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
  auto* am = BuildSegmentFeaturesTestGeometry(dataStructure, dims, std::string(k_ExemplarGeomName), std::string(k_ExemplarCellDataName));
  auto& geom = dataStructure.getDataRefAs<ImageGeom>(k_ExemplarGeomPath);
  BuildOrientationTestData(dataStructure, cellShape, geom.getId(), am->getId(), 0, k_SmallBlockSize); // Hexagonal_High

  CAxisSegmentFeaturesFilter filter;
  Arguments args;
  BuildExemplarArgs(args, /*useMask=*/false, /*tolerance=*/90.0f);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The c-axis metric folds into 90 degrees, so this tolerance merges all
  // valid cells.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ExemplarActivePath));
  const auto& actives = dataStructure.getDataRefAs<UInt8Array>(k_ExemplarActivePath);
  REQUIRE(actives.getNumberOfTuples() == 2); // Feature index zero is reserved.

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_ExemplarFeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_ExemplarFeatureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();
  for(usize i = 0; i < featureStore.getNumberOfTuples(); i++)
  {
    REQUIRE(featureStore.getValue(i) == 1);
  }
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: FaceEdgeVertex Connectivity", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  constexpr float32 k_DegToRad = 3.14159265358979323846f / 180.0f;

  auto setupCAxis = [&](Arguments& args, DataStructure& ds, const DataPath& geomPath, const DataPath& cellDataPath, ChoicesParameter::ValueType neighborScheme) {
    const ShapeType cellShape = {3, 3, 3};
    auto& am = ds.getDataRefAs<AttributeMatrix>(cellDataPath);
    auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);

    // Quaternions: background = 60° X-rotation, pairs = identity and 30° (EBSDlib order: x,y,z,w)
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
    crystStore[1] = 0; // Hexagonal_High

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Quats")));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellDataPath.createChildPath("Phases")));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geom", "CellEnsembleData", "CrystalStructures"})));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  };

  RunFaceEdgeVertexConnectivityTest<CAxisSegmentFeaturesFilter>([&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupCAxis(args, ds, gp, cp, 0); },
                                                                [&](Arguments& args, DataStructure& ds, const DataPath& gp, const DataPath& cp) { setupCAxis(args, ds, gp, cp, 1); });
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures: Generate Test Data", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/caxis_segment_features", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  // The small variants cover base, mask, and periodic configurations.
  {
    const ShapeType cellShape = {k_SmallDim, k_SmallDim, k_SmallDim};
    const std::array<usize, 3> dims = {k_SmallDim, k_SmallDim, k_SmallDim};

    DataStructure ds;

    auto* amBase = BuildSegmentFeaturesTestGeometry(ds, dims, "Base", std::string(k_ExemplarCellDataName));
    auto& geomBase = ds.getDataRefAs<ImageGeom>(DataPath({"Base"}));
    BuildOrientationTestData(ds, cellShape, geomBase.getId(), amBase->getId(), 0, k_SmallBlockSize); // Hexagonal_High

    auto* amMasked = BuildSegmentFeaturesTestGeometry(ds, dims, "Masked", std::string(k_ExemplarCellDataName));
    auto& geomMasked = ds.getDataRefAs<ImageGeom>(DataPath({"Masked"}));
    BuildOrientationTestData(ds, cellShape, geomMasked.getId(), amMasked->getId(), 0, k_SmallBlockSize);
    BuildSphericalMask(ds, cellShape, amMasked->getId());

    UnitTest::WriteTestDataStructure(ds, outputDir / "small_input.dream3d");
  }

  // The large fixture exercises bounded data access with a mask.
  {
    const ShapeType cellShape = {k_LargeDim, k_LargeDim, k_LargeDim};
    const std::array<usize, 3> dims = {k_LargeDim, k_LargeDim, k_LargeDim};

    DataStructure ds;
    auto* am = BuildSegmentFeaturesTestGeometry(ds, dims, std::string(k_ExemplarGeomName), std::string(k_ExemplarCellDataName));
    auto& geom = ds.getDataRefAs<ImageGeom>(k_ExemplarGeomPath);
    BuildOrientationTestData(ds, cellShape, geom.getId(), am->getId(), 0, k_LargeBlockSize); // Hexagonal_High
    BuildSphericalMask(ds, cellShape, am->getId());

    UnitTest::WriteTestDataStructure(ds, outputDir / "large_input.dream3d");
  }
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:Face", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 57);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:All", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 37);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:MaskFace", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 31);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsMaskFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:MaskAll", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 25);
  }

  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsMaskAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

namespace
{
namespace AnalyticalFixtures
{
const std::string k_GeomName = "ImageGeometry";
const DataPath k_ImageGeomPath = DataPath({k_GeomName});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath("CellData");
const DataPath k_EnsembleDataPath = k_ImageGeomPath.createChildPath("CellEnsembleData");

const std::string k_QuatsName = "Quats";
const std::string k_PhasesName = "Phases";
const std::string k_MaskName = "Mask";
const std::string k_CrystalStructuresName = "CrystalStructures";
const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_CellFeatureAMName = "CellFeatureData";
const std::string k_ActiveName = "Active";

const DataPath k_QuatsPath = k_CellDataPath.createChildPath(k_QuatsName);
const DataPath k_PhasesPath = k_CellDataPath.createChildPath(k_PhasesName);
const DataPath k_MaskPath = k_CellDataPath.createChildPath(k_MaskName);
const DataPath k_CrystalStructuresPath = k_EnsembleDataPath.createChildPath(k_CrystalStructuresName);
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath(k_FeatureIdsName);
const DataPath k_CellFeatureAMPath = k_ImageGeomPath.createChildPath(k_CellFeatureAMName);
const DataPath k_ActivePath = k_CellFeatureAMPath.createChildPath(k_ActiveName);

// Pure-Phi Bunge rotations tilt the c-axis by phiDeg. The c-axis metric folds
// antiparallel directions into [0,90], so each fixture has a closed-form angle.
std::array<float32, 4> QuatFromPhiDeg(float32 phiDeg)
{
  const float32 halfAngleRad = (phiDeg * 0.5f) * Constants::k_PiOver180F;
  return {std::sin(halfAngleRad), 0.0f, 0.0f, std::cos(halfAngleRad)};
}

/**
 * @struct FixtureData
 * @brief Holds arrays for one analytical c-axis fixture.
 */
struct FixtureData
{
  DataStructure ds;
  ImageGeom* geom = nullptr;
  AttributeMatrix* cellAM = nullptr;
  AttributeMatrix* ensembleAM = nullptr;
  Float32Array* quats = nullptr;
  Int32Array* phases = nullptr;
  UInt32Array* crystalStructures = nullptr;
};

// Build valid image, cell, and ensemble arrays. Cells start in phase one with
// identity quaternions. Valid ensembles use Hexagonal_High.
FixtureData CreateScaffold(usize dimX, usize dimY, usize dimZ, usize numEnsembles = 2)
{
  FixtureData td;
  const usize numCells = dimX * dimY * dimZ;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({dimX, dimY, dimZ});

  // ShapeType uses Z, Y, X order.
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", cellTupleShape, td.geom->getId());
  td.geom->setCellData(*td.cellAM);
  td.ensembleAM = AttributeMatrix::Create(td.ds, "CellEnsembleData", ShapeType{numEnsembles}, td.geom->getId());

  td.quats = CreateTestDataArray<float32>(td.ds, k_QuatsName, cellTupleShape, {4}, td.cellAM->getId());
  td.phases = CreateTestDataArray<int32>(td.ds, k_PhasesName, cellTupleShape, {1}, td.cellAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numEnsembles}, {1}, td.ensembleAM->getId());

  for(usize cellIdx = 0; cellIdx < numCells; cellIdx++)
  {
    const std::array<float32, 4> quat = QuatFromPhiDeg(0.0f);
    for(usize comp = 0; comp < 4; comp++)
    {
      (*td.quats)[cellIdx * 4 + comp] = quat[comp];
    }
    (*td.phases)[cellIdx] = 1;
  }

  (*td.crystalStructures)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  for(usize ensembleIdx = 1; ensembleIdx < numEnsembles; ensembleIdx++)
  {
    (*td.crystalStructures)[ensembleIdx] = ebsdlib::CrystalStructure::Hexagonal_High;
  }

  return td;
}

void SetPhi(FixtureData& td, usize cellIdx, float32 phiDeg)
{
  const std::array<float32, 4> quat = QuatFromPhiDeg(phiDeg);
  for(usize comp = 0; comp < 4; comp++)
  {
    (*td.quats)[cellIdx * 4 + comp] = quat[comp];
  }
}

Arguments BuildArgs(float32 toleranceDeg, ChoicesParameter::ValueType neighborScheme, bool useMask, bool randomizeIds = false)
{
  Arguments args;
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(toleranceDeg));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborScheme));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIdsName));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_CellFeatureAMName));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(randomizeIds));
  return args;
}

// Error fixtures use DataContainer paths instead of the analytical scaffold.
Arguments BuildManualPreflightArgs(const DataPath& quatsPath, const DataPath& phasesPath)
{
  Arguments args;
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(quatsPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  return args;
}

// Run valid preflight before the execute-error checks.
IFilter::ExecuteResult RunFilter(DataStructure& dataStructure, const Arguments& args)
{
  CAxisSegmentFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  return filter.execute(dataStructure, args);
}

void CheckFeatureIds(const DataStructure& dataStructure, const std::vector<int32>& expectedFeatureIds)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
  REQUIRE(featureIdsRef.getNumberOfTuples() == expectedFeatureIds.size());
  for(usize cellIdx = 0; cellIdx < expectedFeatureIds.size(); cellIdx++)
  {
    INFO(fmt::format("cell index {}", cellIdx));
    REQUIRE(featureIdsRef[cellIdx] == expectedFeatureIds[cellIdx]);
  }
}

// Active has one reserved zero entry and one true entry for each feature.
void CheckActiveArray(const DataStructure& dataStructure, usize expectedNumFeatures)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_ActivePath));
  const auto& activeRef = dataStructure.getDataRefAs<UInt8Array>(k_ActivePath).getDataStoreRef();
  REQUIRE(activeRef.getNumberOfTuples() == expectedNumFeatures + 1);
  REQUIRE(activeRef[0] == 0);
  for(usize featureIdx = 1; featureIdx <= expectedNumFeatures; featureIdx++)
  {
    INFO(fmt::format("feature index {}", featureIdx));
    REQUIRE(activeRef[featureIdx] == 1);
  }
}
} // namespace AnalyticalFixtures
} // namespace

using namespace AnalyticalFixtures;

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Pure-Phi Chain, Face)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // This 10-degree chain yields four partitions: {0,1,2}, {3,4}, {5,6},
  // and {7}.
  FixtureData td = CreateScaffold(8, 1, 1);
  const std::vector<float32> phiValues = {0.0f, 5.0f, 8.0f, 45.0f, 50.0f, 120.0f, 124.0f, 90.0f};
  for(usize cellIdx = 0; cellIdx < phiValues.size(); cellIdx++)
  {
    SetPhi(td, cellIdx, phiValues[cellIdx]);
  }

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(td.ds, {1, 1, 1, 2, 2, 3, 3, 4});
  CheckActiveArray(td.ds, 4);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Pi-Fold Antiparallel C-Axes)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // The c-axis fold groups Phi 2 and 176 at tolerance 10. Phi 88 remains
  // separate.
  FixtureData td = CreateScaffold(3, 1, 1);
  SetPhi(td, 0, 2.0f);
  SetPhi(td, 1, 176.0f);
  SetPhi(td, 2, 88.0f);

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(td.ds, {1, 1, 2});
  CheckActiveArray(td.ds, 2);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Neighbor Scheme Face vs All)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // Only all-neighbor connectivity can join identical diagonal cells zero and
  // three.
  /**
   * @struct SchemeExpectation
   * @brief Defines expected segmentation for one neighbor scheme.
   */
  struct SchemeExpectation
  {
    std::string label;
    ChoicesParameter::ValueType scheme;
    std::vector<int32> expectedFeatureIds;
    usize expectedNumFeatures;
  };
  const std::vector<SchemeExpectation> expectations = {
      {"Face Neighbors", segment_features::k_6NeighborIndex, {1, 2, 3, 4}, 4},
      {"All Connected Neighbors", segment_features::k_26NeighborIndex, {1, 2, 3, 1}, 3},
  };

  for(const auto& expectation : expectations)
  {
    DYNAMIC_SECTION(expectation.label)
    {
      FixtureData td = CreateScaffold(2, 2, 1);
      SetPhi(td, 0, 0.0f);
      SetPhi(td, 1, 45.0f);
      SetPhi(td, 2, 90.0f);
      SetPhi(td, 3, 0.0f);

      auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, expectation.scheme, false));
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      CheckFeatureIds(td.ds, expectation.expectedFeatureIds);
      CheckActiveArray(td.ds, expectation.expectedNumFeatures);
      UnitTest::CheckArraysInheritTupleDims(td.ds);
    }
  }
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Mask Excludes Voxel 0)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // getSeed() must reject the masked first cell. Otherwise, the driver creates
  // a phantom feature and shifts later identifiers.
  /**
   * @struct MaskVariant
   * @brief Defines one supported mask representation.
   */
  struct MaskVariant
  {
    std::string label;
    DataType maskType;
  };
  const std::vector<MaskVariant> variants = {
      {"bool mask", DataType::boolean},
      {"uint8 mask", DataType::uint8},
  };

  for(const auto& variant : variants)
  {
    DYNAMIC_SECTION(variant.label)
    {
      FixtureData td = CreateScaffold(5, 1, 1);
      const std::vector<float32> phiValues = {0.0f, 20.0f, 22.0f, 0.0f, 90.0f};
      const std::vector<uint8> maskValues = {0, 1, 1, 0, 1};
      for(usize cellIdx = 0; cellIdx < phiValues.size(); cellIdx++)
      {
        SetPhi(td, cellIdx, phiValues[cellIdx]);
      }
      if(variant.maskType == DataType::boolean)
      {
        auto* maskArrayPtr = CreateTestDataArray<bool>(td.ds, k_MaskName, ShapeType{1, 1, 5}, {1}, td.cellAM->getId());
        for(usize cellIdx = 0; cellIdx < maskValues.size(); cellIdx++)
        {
          (*maskArrayPtr)[cellIdx] = (maskValues[cellIdx] != 0);
        }
      }
      else
      {
        auto* maskArrayPtr = CreateTestDataArray<uint8>(td.ds, k_MaskName, ShapeType{1, 1, 5}, {1}, td.cellAM->getId());
        for(usize cellIdx = 0; cellIdx < maskValues.size(); cellIdx++)
        {
          (*maskArrayPtr)[cellIdx] = maskValues[cellIdx];
        }
      }

      auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, true));
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      CheckFeatureIds(td.ds, {0, 1, 1, 0, 2});
      CheckActiveArray(td.ds, 2);
      UnitTest::CheckArraysInheritTupleDims(td.ds);
    }
  }
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Phase Separation)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // Equal orientations in different phases must remain separate. Phase two
  // also exercises Hexagonal_Low validation.
  FixtureData td = CreateScaffold(4, 1, 1, 3);
  (*td.phases)[2] = 2;
  (*td.phases)[3] = 2;
  (*td.crystalStructures)[2] = ebsdlib::CrystalStructure::Hexagonal_Low;

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(td.ds, {1, 1, 2, 2});
  CheckActiveArray(td.ds, 2);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (3D Linearization, 3x2x2)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // An axis-asymmetric 3x2x2 fixture detects x, y, or z stride swaps in the
  // neighbor decode.
  FixtureData td = CreateScaffold(3, 2, 2);
  const std::vector<float32> phiValues = {0.0f, 5.0f, 40.0f, 8.0f, 90.0f, 44.0f, 3.0f, 60.0f, 130.0f, 12.0f, 85.0f, 170.0f};
  for(usize cellIdx = 0; cellIdx < phiValues.size(); cellIdx++)
  {
    SetPhi(td, cellIdx, phiValues[cellIdx]);
  }

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(td.ds, {1, 1, 2, 1, 3, 2, 1, 4, 5, 1, 3, 6});
  CheckActiveArray(td.ds, 6);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (RectGrid Geometry)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // CAxis accepts RectGridGeom through IGridGeometry. This fixture prevents a
  // stale ImageGeom cast.
  DataStructure dataStructure;
  // Reuse k_GeomName so shared arguments resolve the RectGrid path.
  auto* rectGridGeom = RectGridGeom::Create(dataStructure, k_GeomName);
  rectGridGeom->setDimensions(SizeVec3{3, 1, 1});
  auto* xBoundsArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "xBounds", ShapeType{4}, ShapeType{1}, rectGridGeom->getId());
  auto* yBoundsArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "yBounds", ShapeType{2}, ShapeType{1}, rectGridGeom->getId());
  auto* zBoundsArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "zBounds", ShapeType{2}, ShapeType{1}, rectGridGeom->getId());
  const std::vector<float32> xBoundValues = {0.0f, 0.5f, 2.0f, 10.0f};
  for(usize boundIdx = 0; boundIdx < xBoundValues.size(); boundIdx++)
  {
    xBoundsArrayPtr->setValue(boundIdx, xBoundValues[boundIdx]);
  }
  yBoundsArrayPtr->setValue(0, 0.0f);
  yBoundsArrayPtr->setValue(1, 1.0f);
  zBoundsArrayPtr->setValue(0, 0.0f);
  zBoundsArrayPtr->setValue(1, 1.0f);
  rectGridGeom->setBounds(xBoundsArrayPtr, yBoundsArrayPtr, zBoundsArrayPtr);

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 3}, rectGridGeom->getId());
  rectGridGeom->setCellData(*cellAM);
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, rectGridGeom->getId());
  auto* quatsArrayPtr = CreateTestDataArray<float32>(dataStructure, k_QuatsName, ShapeType{1, 1, 3}, {4}, cellAM->getId());
  auto* phasesArrayPtr = CreateTestDataArray<int32>(dataStructure, k_PhasesName, ShapeType{1, 1, 3}, {1}, cellAM->getId());
  auto* crystalStructuresArrayPtr = CreateTestDataArray<uint32>(dataStructure, k_CrystalStructuresName, ShapeType{2}, {1}, ensembleAM->getId());

  const std::vector<float32> phiValues = {0.0f, 5.0f, 45.0f};
  for(usize cellIdx = 0; cellIdx < phiValues.size(); cellIdx++)
  {
    const std::array<float32, 4> quat = QuatFromPhiDeg(phiValues[cellIdx]);
    for(usize comp = 0; comp < 4; comp++)
    {
      (*quatsArrayPtr)[cellIdx * 4 + comp] = quat[comp];
    }
    (*phasesArrayPtr)[cellIdx] = 1;
  }
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

  auto executeResult = RunFilter(dataStructure, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(dataStructure, {1, 1, 2});
  CheckActiveArray(dataStructure, 2);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Quats Outside Cell AttributeMatrix)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class1]")
{
  UnitTest::LoadPlugins();

  // Selected arrays can be outside CellData. FeatureIds must remain in the
  // geometry cell matrix.
  FixtureData td = CreateScaffold(3, 1, 1);
  auto* orientationAM = AttributeMatrix::Create(td.ds, "OrientationData", ShapeType{1, 1, 3}, td.geom->getId());
  auto* quatsArrayPtr = CreateTestDataArray<float32>(td.ds, "QuatsElsewhere", ShapeType{1, 1, 3}, {4}, orientationAM->getId());
  auto* phasesArrayPtr = CreateTestDataArray<int32>(td.ds, "PhasesElsewhere", ShapeType{1, 1, 3}, {1}, orientationAM->getId());
  const std::vector<float32> phiValues = {0.0f, 5.0f, 45.0f};
  for(usize cellIdx = 0; cellIdx < phiValues.size(); cellIdx++)
  {
    const std::array<float32, 4> quat = QuatFromPhiDeg(phiValues[cellIdx]);
    for(usize comp = 0; comp < 4; comp++)
    {
      (*quatsArrayPtr)[cellIdx * 4 + comp] = quat[comp];
    }
    (*phasesArrayPtr)[cellIdx] = 1;
  }

  Arguments args = BuildArgs(10.0f, segment_features::k_6NeighborIndex, false);
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_ImageGeomPath.createChildPath("OrientationData").createChildPath("QuatsElsewhere")));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_ImageGeomPath.createChildPath("OrientationData").createChildPath("PhasesElsewhere")));

  auto executeResult = RunFilter(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(td.ds, {1, 1, 2});
  CheckActiveArray(td.ds, 2);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 4 Invariants (RandomizeFeatureIds)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class4]")
{
  UnitTest::LoadPlugins();

  // Randomization must relabel without changing partitions and must be
  // deterministic.
  const std::vector<float32> phiValues = {0.0f, 5.0f, 8.0f, 45.0f, 50.0f, 120.0f, 124.0f, 90.0f};

  auto runOnce = [&]() -> std::vector<int32> {
    FixtureData td = CreateScaffold(8, 1, 1);
    for(usize cellIdx = 0; cellIdx < phiValues.size(); cellIdx++)
    {
      SetPhi(td, cellIdx, phiValues[cellIdx]);
    }
    auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false, true));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath));
    const auto& featureIdsRef = td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
    std::vector<int32> featureIds(featureIdsRef.getNumberOfTuples());
    for(usize cellIdx = 0; cellIdx < featureIds.size(); cellIdx++)
    {
      featureIds[cellIdx] = featureIdsRef[cellIdx];
    }
    CheckActiveArray(td.ds, 4);
    UnitTest::CheckArraysInheritTupleDims(td.ds);
    return featureIds;
  };

  const std::vector<int32> firstRun = runOnce();

  REQUIRE(firstRun[0] == firstRun[1]);
  REQUIRE(firstRun[1] == firstRun[2]);
  REQUIRE(firstRun[3] == firstRun[4]);
  REQUIRE(firstRun[5] == firstRun[6]);
  const std::set<int32> distinctIds = {firstRun[0], firstRun[3], firstRun[5], firstRun[7]};
  REQUIRE(distinctIds.size() == 4);

  REQUIRE(distinctIds == std::set<int32>{1, 2, 3, 4});

  // A no-op randomizer would preserve the canonical labels.
  REQUIRE(firstRun != std::vector<int32>{1, 1, 1, 2, 2, 3, 3, 4});

  const std::vector<int32> secondRun = runOnce();
  REQUIRE(firstRun == secondRun);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 4 Invariants (RandomizeFeatureIds Preserves Masked Zeros)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][Class4]")
{
  UnitTest::LoadPlugins();

  // Randomization must preserve reserved identifier zero for masked cells.
  FixtureData td = CreateScaffold(5, 1, 1);
  const std::vector<float32> phiValues = {0.0f, 20.0f, 22.0f, 0.0f, 90.0f};
  for(usize cellIdx = 0; cellIdx < phiValues.size(); cellIdx++)
  {
    SetPhi(td, cellIdx, phiValues[cellIdx]);
  }
  auto* maskArrayPtr = CreateTestDataArray<bool>(td.ds, k_MaskName, ShapeType{1, 1, 5}, {1}, td.cellAM->getId());
  const std::vector<uint8> maskValues = {0, 1, 1, 0, 1};
  for(usize cellIdx = 0; cellIdx < maskValues.size(); cellIdx++)
  {
    (*maskArrayPtr)[cellIdx] = (maskValues[cellIdx] != 0);
  }

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIdsRef = td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
  REQUIRE(featureIdsRef[0] == 0);
  REQUIRE(featureIdsRef[3] == 0);
  REQUIRE(featureIdsRef[1] == featureIdsRef[2]);
  REQUIRE(featureIdsRef[1] != featureIdsRef[4]);
  REQUIRE(std::set<int32>{featureIdsRef[1], featureIdsRef[4]} == std::set<int32>{1, 2});
  CheckActiveArray(td.ds, 2);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Phase 0 (Unindexed) Cells Tolerated", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // EBSD phase zero is unindexed and must remain outside segmentation.
  FixtureData td = CreateScaffold(4, 1, 1);
  (*td.phases)[0] = 0;

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(td.ds, {0, 1, 1, 1});
  CheckActiveArray(td.ds, 1);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Masked Non-Hexagonal Cells Tolerated", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // The mask can exclude a non-hexagonal phase before c-axis validation.
  FixtureData td = CreateScaffold(4, 1, 1, 3);
  (*td.phases)[2] = 2;
  (*td.phases)[3] = 2;
  (*td.crystalStructures)[2] = ebsdlib::CrystalStructure::Cubic_High;
  auto* maskArrayPtr = CreateTestDataArray<bool>(td.ds, k_MaskName, ShapeType{1, 1, 4}, {1}, td.cellAM->getId());
  (*maskArrayPtr)[0] = true;
  (*maskArrayPtr)[1] = true;
  (*maskArrayPtr)[2] = false;
  (*maskArrayPtr)[3] = false;

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, true));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  CheckFeatureIds(td.ds, {1, 1, 0, 0});
  CheckActiveArray(td.ds, 1);
  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - Non-Hexagonal Crystal Structure (-8363)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // C-axis misalignment accepts only hexagonal Laue classes.
  FixtureData td = CreateScaffold(2, 1, 1);
  (*td.crystalStructures)[1] = ebsdlib::CrystalStructure::Cubic_High;

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -8363);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - Phase Out of Ensemble Bounds (-8364)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // An out-of-range phase must return an error before an out-of-bounds read.
  FixtureData td = CreateScaffold(2, 1, 1);
  (*td.phases)[1] = 7;

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -8364);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - No Features Found (-87000)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // An all-masked fixture has no seed and must return -87000.
  FixtureData td = CreateScaffold(3, 1, 1);
  auto* maskArrayPtr = CreateTestDataArray<bool>(td.ds, k_MaskName, ShapeType{1, 1, 3}, {1}, td.cellAM->getId());
  for(usize cellIdx = 0; cellIdx < 3; cellIdx++)
  {
    (*maskArrayPtr)[cellIdx] = false;
  }

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -87000);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Zero Tolerance (-655)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][preflight]")
{
  UnitTest::LoadPlugins();

  FixtureData td = CreateScaffold(2, 1, 1);

  CAxisSegmentFeaturesFilter filter;
  auto preflightResult = filter.preflight(td.ds, BuildArgs(0.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -655);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Cell Arrays Smaller Than Geometry (-652)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Nine-tuple arrays pass cross-array validation but fail the 10-cell geometry
  // validation.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {9}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {9}, {4}, cellAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, cellAM->getId());
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  CAxisSegmentFeaturesFilter filter;
  const Arguments args = BuildManualPreflightArgs(DataPath({"DataContainer", "CellData", "Quats"}), DataPath({"DataContainer", "CellData", "Phases"}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -652);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Cell AttributeMatrix Smaller Than Geometry (-653)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // The selected arrays have ten tuples, but the FeatureIds parent has nine.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {9}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* orientationAM = AttributeMatrix::Create(dataStructure, "OrientationData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, orientationAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {10}, {1}, orientationAM->getId());
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  CAxisSegmentFeaturesFilter filter;
  const Arguments args = BuildManualPreflightArgs(DataPath({"DataContainer", "OrientationData", "Quats"}), DataPath({"DataContainer", "OrientationData", "Phases"}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -653);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Cell array tuple count mismatch (-651)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Quats and CellPhases have different tuple counts and must report -651.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAM->getId());

  // The separate group gives CellPhases nine tuples instead of ten.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  CAxisSegmentFeaturesFilter filter;
  const Arguments args = BuildManualPreflightArgs(DataPath({"DataContainer", "CellData", "Quats"}), DataPath({"DataContainer", "MismatchData", "Phases"}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -651);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CAxisSegmentFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CAxisSegmentFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CAxisSegmentFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<float32>(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key) == 2.5f);
      CHECK(args.value<bool>(CAxisSegmentFeaturesFilter::k_UseMask_Key) == true);
      CHECK(args.value<bool>(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key) == true);
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key) == "TestName");
    }
  }
}
