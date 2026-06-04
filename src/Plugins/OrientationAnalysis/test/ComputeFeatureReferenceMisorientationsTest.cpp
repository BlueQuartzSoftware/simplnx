#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/ComputeFeatureReferenceMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "simplnx/Common/Numbers.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

// =============================================================================
// V&V Class 1 (Analytical) + Class 4 (Invariant) oracle support — added 2026-06-01.
//
// These fixtures replace the regression-against-archive pattern used by the two pre-existing
// exemplar tests. The inputs are built inline as tiny ImageGeoms with hand-derived expected
// outputs computable in closed form from pure phi1 rotations (Bunge ZXZ Euler `(phi1, 0, 0)`).
// Class 4 invariants are asserted alongside the Class 1 values.
//
// Reference: src/Plugins/OrientationAnalysis/vv/ComputeFeatureReferenceMisorientationsFilter.md
// =============================================================================

namespace DataFixtures
{
// Test-side default paths (kept consistent with the existing `Small_IN100`-style fixtures).
const std::string k_GeomName = "DataContainer";
const DataPath k_ImageGeomPath = DataPath({k_GeomName});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath(k_CellData);
const DataPath k_CellFeatureDataPath = k_ImageGeomPath.createChildPath(k_CellFeatureData);
const DataPath k_CellEnsembleDataPath = k_ImageGeomPath.createChildPath(k_CellEnsembleData);

const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_PhasesName = "Phases";
const std::string k_QuatsName = "Quats";
const std::string k_AvgQuatsName = "AvgQuats";
const std::string k_GBEuclideanName = "GBEuclideanDistances";
const std::string k_CrystalStructuresName = "CrystalStructures";
const std::string k_CellMisorientationsOutName = "Cell Reference Misorientations";
const std::string k_FeatureAvgMisorientationsOutName = "Feature Avg Misorientations";
const std::string k_FeatureEuclideanCentersOutName = "Feature Euclidean Centers";

// Build a quaternion representing a pure phi1 rotation about z (Bunge ZXZ Euler `(phi1, 0, 0)`).
// Returned as a 4-component vector in (x, y, z, w) layout matching the simplnx convention.
inline std::array<float32, 4> QuatFromPhi1Deg(float32 phi1Deg)
{
  const float32 halfAngleRad = (phi1Deg * 0.5f) * nx::core::numbers::pi_v<float> / 180.0f;
  return {0.0f, 0.0f, std::sin(halfAngleRad), std::cos(halfAngleRad)};
}

// Build a minimal DataStructure scaffold with ImageGeom + Cell Data AM + Cell Feature Data AM
// + Cell Ensemble AM and the input arrays this filter needs. Caller is responsible for populating
// the array values. Crystal structure ensemble is [UnknownCrystalStructure=999, Cubic_High=1].
struct ToyData
{
  DataStructure ds;
  ImageGeom* imageGeom = nullptr;
  AttributeMatrix* cellDataAM = nullptr;
  AttributeMatrix* cellFeatureDataAM = nullptr;
  AttributeMatrix* cellEnsembleAM = nullptr;
  Int32Array* featureIds = nullptr;
  Int32Array* cellPhases = nullptr;
  Float32Array* quats = nullptr;
  Float32Array* avgQuats = nullptr;             // sized for numFeatures; populated only for Mode 0
  Float32Array* gbEuclideanDistances = nullptr; // only populated for Mode 1
  UInt32Array* crystalStructures = nullptr;
};

inline ToyData CreateScaffold(const usize dimX, const usize dimY, const usize dimZ, const usize numFeatures)
{
  ToyData td;
  td.imageGeom = ImageGeom::Create(td.ds, k_GeomName);
  td.imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  td.imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  td.imageGeom->setDimensions({dimX, dimY, dimZ});
  const ShapeType cellTupleShape{dimZ, dimY, dimX};
  const usize totalVoxels = dimX * dimY * dimZ;

  td.cellDataAM = AttributeMatrix::Create(td.ds, k_CellData, cellTupleShape, td.imageGeom->getId());
  td.imageGeom->setCellData(*td.cellDataAM);

  td.cellFeatureDataAM = AttributeMatrix::Create(td.ds, k_CellFeatureData, ShapeType{numFeatures}, td.imageGeom->getId());
  td.cellEnsembleAM = AttributeMatrix::Create(td.ds, k_CellEnsembleData, ShapeType{2}, td.imageGeom->getId());

  td.featureIds = CreateTestDataArray<int32>(td.ds, k_FeatureIdsName, cellTupleShape, {1}, td.cellDataAM->getId());
  td.cellPhases = CreateTestDataArray<int32>(td.ds, k_PhasesName, cellTupleShape, {1}, td.cellDataAM->getId());
  td.quats = CreateTestDataArray<float32>(td.ds, k_QuatsName, cellTupleShape, {4}, td.cellDataAM->getId());
  td.gbEuclideanDistances = CreateTestDataArray<float32>(td.ds, k_GBEuclideanName, cellTupleShape, {1}, td.cellDataAM->getId());
  td.avgQuats = CreateTestDataArray<float32>(td.ds, k_AvgQuatsName, {numFeatures}, {4}, td.cellFeatureDataAM->getId());

  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {2}, {1}, td.cellEnsembleAM->getId());
  (*td.crystalStructures)[0] = 999u; // UnknownCrystalStructure sentinel
  (*td.crystalStructures)[1] = 1u;   // Cubic_High (EbsdLib LaueOps index 1)

  // Initialize default zero values
  for(usize i = 0; i < totalVoxels; ++i)
  {
    (*td.featureIds)[i] = 0;
    (*td.cellPhases)[i] = 0;
    (*td.gbEuclideanDistances)[i] = 0.0f;
    (*td.quats)[i * 4 + 0] = 0.0f;
    (*td.quats)[i * 4 + 1] = 0.0f;
    (*td.quats)[i * 4 + 2] = 0.0f;
    (*td.quats)[i * 4 + 3] = 1.0f; // identity by default
  }
  for(usize f = 0; f < numFeatures; ++f)
  {
    (*td.avgQuats)[f * 4 + 0] = 0.0f;
    (*td.avgQuats)[f * 4 + 1] = 0.0f;
    (*td.avgQuats)[f * 4 + 2] = 0.0f;
    (*td.avgQuats)[f * 4 + 3] = 1.0f;
  }

  return td;
}

// Build standard Arguments for the filter from a ToyData scaffold.
inline Arguments BuildArgs(int32 referenceOrientation)
{
  Arguments args;
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<ChoicesParameter::ValueType>(referenceOrientation));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_FeatureIdsName)));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_PhasesName)));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_QuatsName)));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CellEnsembleDataPath.createChildPath(k_CrystalStructuresName)));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_AvgQuatsName)));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_GBEuclideanName)));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellMisorientationsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CellMisorientationsOutName));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_FeatureAvgMisorientationsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureAvgMisorientationsOutName));
  args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_FeatureEuclideanCenterArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FeatureEuclideanCentersOutName));
  return args;
}

// Helper: assert per-voxel FRM matches expected value within tolerance.
inline void RequireFRMClose(const DataStructure& ds, usize voxelIdx, float32 expectedDeg, float32 tolDeg = 1e-3f)
{
  const auto& frm = ds.getDataRefAs<Float32Array>(k_CellDataPath.createChildPath(k_CellMisorientationsOutName));
  const float32 actual = frm[voxelIdx];
  const float32 diff = std::abs(actual - expectedDeg);
  if(diff > tolDeg)
  {
    CAPTURE(voxelIdx);
    CAPTURE(expectedDeg);
    CAPTURE(actual);
    CAPTURE(diff);
    REQUIRE(diff <= tolDeg);
  }
}

inline void RequireAvgClose(const DataStructure& ds, usize featureIdx, float32 expectedDeg, float32 tolDeg = 1e-3f, bool isMode1 = false)
{
  const std::string& avgName = k_FeatureAvgMisorientationsOutName;
  const auto avgPath = k_CellFeatureDataPath.createChildPath(avgName);
  const auto& avg = ds.getDataRefAs<Float32Array>(avgPath);
  const float32 actual = avg[featureIdx];
  const float32 diff = std::abs(actual - expectedDeg);
  if(diff > tolDeg)
  {
    CAPTURE(featureIdx);
    CAPTURE(expectedDeg);
    CAPTURE(actual);
    CAPTURE(diff);
    REQUIRE(diff <= tolDeg);
  }
}

// Class 4 invariant predicates — assert every invariant on the filter's output.
inline void AssertClass4Invariants(const DataStructure& ds, bool isMode1)
{
  const auto& featureIds = ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_FeatureIdsName));
  const auto& cellPhases = ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_PhasesName));
  const auto& frm = ds.getDataRefAs<Float32Array>(k_CellDataPath.createChildPath(k_CellMisorientationsOutName));
  const auto& avg = ds.getDataRefAs<Float32Array>(k_CellFeatureDataPath.createChildPath(k_FeatureAvgMisorientationsOutName));

  const usize totalVoxels = featureIds.getNumberOfTuples();
  const usize numFeatures = avg.getNumberOfTuples();

  // Invariant 1: FRM[i] >= 0 for all voxels
  // Invariant 2: FRM[i] <= 62.8 deg (cubic max symmetry-reduced misorientation)
  // Invariant 3: FRM[i] == 0 when featureIds[i] == 0 OR cellPhases[i] == 0 (skip path)
  for(usize i = 0; i < totalVoxels; ++i)
  {
    REQUIRE(frm[i] >= 0.0f);
    REQUIRE(frm[i] <= 62.8f);
    if(featureIds[i] == 0 || cellPhases[i] == 0)
    {
      REQUIRE(frm[i] == Approx(0.0f).margin(1e-6f));
    }
  }

  // Invariant 4: avg[0] == 0 (background feature)
  REQUIRE(avg[0] == Approx(0.0f).margin(1e-6f));

  // Invariant 5: avg[fid] = sum(FRM[v in feature fid, phase>0]) / count(v in feature fid, phase>0)
  //              avg[fid] = 0 when count == 0
  for(usize fid = 1; fid < numFeatures; ++fid)
  {
    float64 sum = 0.0;
    usize count = 0;
    for(usize i = 0; i < totalVoxels; ++i)
    {
      if(static_cast<usize>(featureIds[i]) == fid && cellPhases[i] > 0)
      {
        sum += static_cast<float64>(frm[i]);
        count++;
      }
    }
    const float32 expectedAvg = (count == 0) ? 0.0f : static_cast<float32>(sum / static_cast<float64>(count));
    const float32 diff = std::abs(avg[fid] - expectedAvg);
    if(diff > 1e-4f)
    {
      CAPTURE(fid);
      CAPTURE(expectedAvg);
      CAPTURE(avg[fid]);
      CAPTURE(diff);
      REQUIRE(diff <= 1e-4f);
    }
  }
}

} // namespace ToyFixtures

// Retired 2026-06-01 (V&V cycle): the legacy anonymous namespace of array name constants and
// the two TEST_CASEs `_AverageMisorientation` and `_EuclideanDistance` that consumed the
// `compute_feature_reference_misorientation.tar.gz` Small-IN100 exemplar archive were removed.
// The exemplar arrays in the archive were generated from pre-EbsdLib-2.4.1 SIMPLNX output;
// the EbsdLib 2.4.1 CubicOps precision fix shifted the per-feature averages by 2x to 10x the
// 1e-4 epsilon used in the regression check, surfacing the underlying circular-oracle pattern.
// The Class 1 + Class 4 toy fixtures below replace this regression-against-archive coverage
// with hand-derived analytical assertions. See
// `vv/provenance/ComputeFeatureReferenceMisorientationsFilter.md` for the retirement details.

TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: SIMPL Backwards Compatibility",
          "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureReferenceMisorientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureReferenceMisorientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureReferenceMisorientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ComputeFeatureReferenceMisorientationsFilter::k_ReferenceOrientation_Key) == 0);
      CHECK(args.value<DataPath>(ComputeFeatureReferenceMisorientationsFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceMisorientationsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceMisorientationsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceMisorientationsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceMisorientationsFilter::k_GBEuclideanDistancesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceMisorientationsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceMisorientationsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeatureReferenceMisorientationsFilter::k_CellMisorientationsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureReferenceMisorientationsFilter::k_FeatureAvgMisorientationsArrayName_Key) == "TestName");
    }
  }
}

// =============================================================================
// V&V Class 1 (Analytical) + Class 4 (Invariant) toy fixtures — added 2026-06-01.
// =============================================================================

// Fixture A: Mode 0, single 2x2x2 grain, all identity quats. Expected: FRM = 0, avg = 0.
// Covers code paths: 1 (Mode 0), 4 (compute branch), 8 (avg finalize, non-zero count).
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 SingleGrainIdentity", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(2, 2, 2, 2);

  // 8 voxels, all featureId=1, all phase=1, all quats = identity (already initialized).
  for(usize i = 0; i < 8; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
  }
  // AvgQuats[1] = identity (already initialized).

  ComputeFeatureReferenceMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(0);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  for(usize i = 0; i < 8; ++i)
  {
    DataFixtures::RequireFRMClose(td.ds, i, 0.0f);
  }
  DataFixtures::RequireAvgClose(td.ds, 1, 0.0f);
  DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/false);
}

// Fixture B: Mode 0, single 2x2x2 grain, all quats identical (5 deg about z), AvgQuats[1] = identity.
// Expected: FRM = 5 deg for all voxels (cubic 4-fold about z does not reduce 5 deg), avg = 5 deg.
// Covers paths: 1, 4, 8 (verifies magnitude with known non-zero misorientation).
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 KnownAngle5deg", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(2, 2, 2, 2);

  const auto qVoxel = DataFixtures::QuatFromPhi1Deg(5.0f);
  for(usize i = 0; i < 8; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
    (*td.quats)[i * 4 + 0] = qVoxel[0];
    (*td.quats)[i * 4 + 1] = qVoxel[1];
    (*td.quats)[i * 4 + 2] = qVoxel[2];
    (*td.quats)[i * 4 + 3] = qVoxel[3];
  }
  // AvgQuats[1] = identity (already initialized).

  ComputeFeatureReferenceMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(0);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  for(usize i = 0; i < 8; ++i)
  {
    DataFixtures::RequireFRMClose(td.ds, i, 5.0f);
  }
  DataFixtures::RequireAvgClose(td.ds, 1, 5.0f);
  DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/false);
}

// Fixture C: Mode 0, 4x3x1 image with 5 features. Tests:
//  - Background voxel (featureId == 0) -> FRM = 0 (skip path 5)
//  - Un-phased voxel (phase == 0) within a valid feature -> FRM = 0 (skip path 5)
//  - Feature with non-zero misorientation -> path 4 + 8
//  - Feature with all un-phased voxels (count == 0) -> avg = 0 (path 7)
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 MultiGrain EdgeCases", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(4, 3, 1, 5);

  const auto qIdentity = DataFixtures::QuatFromPhi1Deg(0.0f);
  const auto q5 = DataFixtures::QuatFromPhi1Deg(5.0f);
  const auto q10 = DataFixtures::QuatFromPhi1Deg(10.0f);
  auto setQuat = [&](usize voxelIdx, const std::array<float32, 4>& q) {
    (*td.quats)[voxelIdx * 4 + 0] = q[0];
    (*td.quats)[voxelIdx * 4 + 1] = q[1];
    (*td.quats)[voxelIdx * 4 + 2] = q[2];
    (*td.quats)[voxelIdx * 4 + 3] = q[3];
  };
  auto setAvg = [&](usize featureIdx, const std::array<float32, 4>& q) {
    (*td.avgQuats)[featureIdx * 4 + 0] = q[0];
    (*td.avgQuats)[featureIdx * 4 + 1] = q[1];
    (*td.avgQuats)[featureIdx * 4 + 2] = q[2];
    (*td.avgQuats)[featureIdx * 4 + 3] = q[3];
  };

  // Voxel 0: featureId=0 (background), phase=0 -> FRM = 0
  (*td.featureIds)[0] = 0;
  (*td.cellPhases)[0] = 0;
  setQuat(0, qIdentity);

  // Voxels 1, 2: featureId=1, phase=1, quats = identity. AvgQuats[1] = 5deg -> FRM = 5deg.
  (*td.featureIds)[1] = 1;
  (*td.cellPhases)[1] = 1;
  setQuat(1, qIdentity);
  (*td.featureIds)[2] = 1;
  (*td.cellPhases)[2] = 1;
  setQuat(2, qIdentity);
  setAvg(1, q5);

  // Voxels 3, 4: featureId=2, phase=1, quats = identity. AvgQuats[2] = identity -> FRM = 0.
  (*td.featureIds)[3] = 2;
  (*td.cellPhases)[3] = 1;
  setQuat(3, qIdentity);
  (*td.featureIds)[4] = 2;
  (*td.cellPhases)[4] = 1;
  setQuat(4, qIdentity);
  setAvg(2, qIdentity);

  // Voxel 5: featureId=3, phase=1, quats = 10deg. AvgQuats[3] = 5deg -> FRM = 5deg.
  (*td.featureIds)[5] = 3;
  (*td.cellPhases)[5] = 1;
  setQuat(5, q10);
  // Voxels 6, 7: featureId=3, phase=0 (un-phased) -> FRM = 0 (skip path).
  (*td.featureIds)[6] = 3;
  (*td.cellPhases)[6] = 0;
  setQuat(6, qIdentity);
  (*td.featureIds)[7] = 3;
  (*td.cellPhases)[7] = 0;
  setQuat(7, qIdentity);
  setAvg(3, q5);

  // Voxels 8-11: featureId=4, phase=0 (entire feature un-phased) -> all FRM = 0,
  // avgMisorientationCounts[4] == 0 -> avg[4] = 0 (path 7).
  (*td.featureIds)[8] = 4;
  (*td.cellPhases)[8] = 0;
  setQuat(8, qIdentity);
  (*td.featureIds)[9] = 4;
  (*td.cellPhases)[9] = 0;
  setQuat(9, qIdentity);
  (*td.featureIds)[10] = 4;
  (*td.cellPhases)[10] = 0;
  setQuat(10, qIdentity);
  (*td.featureIds)[11] = 4;
  (*td.cellPhases)[11] = 0;
  setQuat(11, qIdentity);
  setAvg(4, qIdentity);

  ComputeFeatureReferenceMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(0);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Per-voxel expected FRMs.
  DataFixtures::RequireFRMClose(td.ds, 0, 0.0f); // background
  DataFixtures::RequireFRMClose(td.ds, 1, 5.0f); // feature 1
  DataFixtures::RequireFRMClose(td.ds, 2, 5.0f);
  DataFixtures::RequireFRMClose(td.ds, 3, 0.0f); // feature 2
  DataFixtures::RequireFRMClose(td.ds, 4, 0.0f);
  DataFixtures::RequireFRMClose(td.ds, 5, 5.0f); // feature 3, only valid voxel
  DataFixtures::RequireFRMClose(td.ds, 6, 0.0f); // feature 3, un-phased
  DataFixtures::RequireFRMClose(td.ds, 7, 0.0f);
  DataFixtures::RequireFRMClose(td.ds, 8, 0.0f); // feature 4, all un-phased
  DataFixtures::RequireFRMClose(td.ds, 9, 0.0f);
  DataFixtures::RequireFRMClose(td.ds, 10, 0.0f);
  DataFixtures::RequireFRMClose(td.ds, 11, 0.0f);

  // Per-feature expected averages.
  DataFixtures::RequireAvgClose(td.ds, 0, 0.0f); // background (no voxels contribute)
  DataFixtures::RequireAvgClose(td.ds, 1, 5.0f); // (5 + 5) / 2
  DataFixtures::RequireAvgClose(td.ds, 2, 0.0f); // (0 + 0) / 2
  DataFixtures::RequireAvgClose(td.ds, 3, 5.0f); // 5 / 1 (only voxel 5 valid)
  DataFixtures::RequireAvgClose(td.ds, 4, 0.0f); // count==0 -> avg=0 (path 7)

  DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/false);
}

// Fixture D: Mode 1, 3x3x1 single grain. Center voxel (4) has max GBEuclideanDistance and
// identity quat; perimeter voxels have 5deg quat. Expected: m_Centers[1]=4, EuclideanCenters[1]
// = coords of voxel 4, FRM[4]=0, FRM[other]=5deg, avg = 40/9 deg.
// Covers paths: 2 (Mode 1 Pass 1 centers), 3 (Mode 1 Pass 2 coords), 4, 8.
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 KnownCenter", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(3, 3, 1, 2);

  const auto qIdentity = DataFixtures::QuatFromPhi1Deg(0.0f);
  const auto q5 = DataFixtures::QuatFromPhi1Deg(5.0f);

  for(usize i = 0; i < 9; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
  }
  // Distances: corners=0.25, edges=1.0, center=1.5. Center voxel (index 4) has max.
  (*td.gbEuclideanDistances)[0] = 0.25f;
  (*td.gbEuclideanDistances)[1] = 1.0f;
  (*td.gbEuclideanDistances)[2] = 0.25f;
  (*td.gbEuclideanDistances)[3] = 1.0f;
  (*td.gbEuclideanDistances)[4] = 1.5f;
  (*td.gbEuclideanDistances)[5] = 1.0f;
  (*td.gbEuclideanDistances)[6] = 0.25f;
  (*td.gbEuclideanDistances)[7] = 1.0f;
  (*td.gbEuclideanDistances)[8] = 0.25f;

  // Quats: voxel 4 = identity (this is the reference); others = 5deg.
  for(usize i = 0; i < 9; ++i)
  {
    const auto& q = (i == 4) ? qIdentity : q5;
    (*td.quats)[i * 4 + 0] = q[0];
    (*td.quats)[i * 4 + 1] = q[1];
    (*td.quats)[i * 4 + 2] = q[2];
    (*td.quats)[i * 4 + 3] = q[3];
  }

  ComputeFeatureReferenceMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(1);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(td.ds, fs::path(fmt::format("{}/find_feature_reference_misorientations_1_1.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  //  FRM expected: voxel 4 = 0 (vs itself), others = 5deg.
  for(usize i = 0; i < 9; ++i)
  {
    DataFixtures::RequireFRMClose(td.ds, i, (i == 4) ? 0.0f : 5.0f);
  }
  // avg = (0 + 5*8) / 9 = 40/9 deg
  DataFixtures::RequireAvgClose(td.ds, 1, 40.0f / 9.0f);

  // EuclideanCenters[1] should be coords of voxel 4 = (1.5, 1.5, 0.5) with spacing 1 and origin 0.
  const auto& centers = td.ds.getDataRefAs<Float32Array>(DataFixtures::k_CellFeatureDataPath.createChildPath(DataFixtures::k_FeatureEuclideanCentersOutName));
  REQUIRE(centers[1 * 3 + 0] == Approx(1.5f).margin(1e-5f));
  REQUIRE(centers[1 * 3 + 1] == Approx(1.5f).margin(1e-5f));
  REQUIRE(centers[1 * 3 + 2] == Approx(0.5f).margin(1e-5f));

  DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/true);
}

// Fixture E: Mode 1, 2x3x1 image with 2 features (3 voxels each). Verifies that m_Centers[fid]
// is correctly isolated per feature (one feature's max-distance voxel does NOT leak into the
// other feature's center selection). Tied distances (>= comparison) -> later voxel wins.
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 MultiGrain CenterIsolation", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(2, 3, 1, 3);

  // Layout: 2x3x1 = 6 voxels, row-major (z=0 plane).
  //   voxel 0,1 (row 0): feature 1
  //   voxel 2,3 (row 1): one of each
  //   voxel 4,5 (row 2): feature 2
  // For per-feature center isolation we need a contiguous-feature layout instead. Use:
  //   voxels 0,1,2 = feature 1; voxels 3,4,5 = feature 2.
  for(usize i = 0; i < 3; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
  }
  for(usize i = 3; i < 6; ++i)
  {
    (*td.featureIds)[i] = 2;
    (*td.cellPhases)[i] = 1;
  }

  // Distances chosen so that:
  //   feature 1 center = voxel 1 (max=1.0)
  //   feature 2 center = voxel 5 (tied 1.5; >= comparison picks later)
  (*td.gbEuclideanDistances)[0] = 0.5f;
  (*td.gbEuclideanDistances)[1] = 1.0f;
  (*td.gbEuclideanDistances)[2] = 0.5f;
  (*td.gbEuclideanDistances)[3] = 1.5f;
  (*td.gbEuclideanDistances)[4] = 0.5f;
  (*td.gbEuclideanDistances)[5] = 1.5f;

  const auto qIdentity = DataFixtures::QuatFromPhi1Deg(0.0f);
  const auto q5 = DataFixtures::QuatFromPhi1Deg(5.0f);
  auto setQuat = [&](usize voxelIdx, const std::array<float32, 4>& q) {
    (*td.quats)[voxelIdx * 4 + 0] = q[0];
    (*td.quats)[voxelIdx * 4 + 1] = q[1];
    (*td.quats)[voxelIdx * 4 + 2] = q[2];
    (*td.quats)[voxelIdx * 4 + 3] = q[3];
  };
  // Feature 1: voxel 1 (center) = identity; voxels 0, 2 = 5deg.
  setQuat(0, q5);
  setQuat(1, qIdentity);
  setQuat(2, q5);
  // Feature 2: voxel 5 (center per >= tie-break) = identity; voxels 3, 4 = 5deg.
  setQuat(3, q5);
  setQuat(4, q5);
  setQuat(5, qIdentity);

  ComputeFeatureReferenceMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(1);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Expected: FRM[1]=0, FRM[0]=5, FRM[2]=5, FRM[5]=0, FRM[3]=5, FRM[4]=5.
  DataFixtures::RequireFRMClose(td.ds, 0, 5.0f);
  DataFixtures::RequireFRMClose(td.ds, 1, 0.0f);
  DataFixtures::RequireFRMClose(td.ds, 2, 5.0f);
  DataFixtures::RequireFRMClose(td.ds, 3, 5.0f);
  DataFixtures::RequireFRMClose(td.ds, 4, 5.0f);
  DataFixtures::RequireFRMClose(td.ds, 5, 0.0f);
  // avg[1] = (5+0+5)/3 = 10/3; avg[2] = (5+5+0)/3 = 10/3.
  DataFixtures::RequireAvgClose(td.ds, 1, 10.0f / 3.0f);
  DataFixtures::RequireAvgClose(td.ds, 2, 10.0f / 3.0f);

  // Per-feature EuclideanCenters: feature 1 = voxel 1 coords; feature 2 = voxel 5 coords.
  // Voxel 1 in a 2x3x1 grid is at (x=1, y=0, z=0); voxel 5 is at (x=1, y=2, z=0). With spacing 1 and
  // origin 0, getCoordsf returns center-of-cell coordinates: voxel 1 -> (1.5, 0.5, 0.5); voxel 5 -> (1.5, 2.5, 0.5).
  const auto& centers = td.ds.getDataRefAs<Float32Array>(DataFixtures::k_CellFeatureDataPath.createChildPath(DataFixtures::k_FeatureEuclideanCentersOutName));
  REQUIRE(centers[1 * 3 + 0] == Approx(1.5f).margin(1e-5f));
  REQUIRE(centers[1 * 3 + 1] == Approx(0.5f).margin(1e-5f));
  REQUIRE(centers[1 * 3 + 2] == Approx(0.5f).margin(1e-5f));
  REQUIRE(centers[2 * 3 + 0] == Approx(1.5f).margin(1e-5f));
  REQUIRE(centers[2 * 3 + 1] == Approx(2.5f).margin(1e-5f));
  REQUIRE(centers[2 * 3 + 2] == Approx(0.5f).margin(1e-5f));

  DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/true);
}

// Fixture F: Mode 1, 3D (3x3x2 = 18 voxels) single grain. Verifies the linear voxelIdx -> 3D coord
// conversion that getCoordsf() performs when dimZ > 1. Other Mode 1 fixtures are 2D (Z=1) and
// therefore don't exercise the z-multiplier in the index arithmetic. Center voxel is in the
// middle of layer 1: voxelIdx = 1*9 + 1*3 + 1 = 13.
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 3D Volume", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(3, 3, 2, 2);

  const auto qIdentity = DataFixtures::QuatFromPhi1Deg(0.0f);
  const auto q5 = DataFixtures::QuatFromPhi1Deg(5.0f);

  for(usize i = 0; i < 18; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
    (*td.gbEuclideanDistances)[i] = 0.5f;
  }
  // Make voxel 13 (layer 1, y=1, x=1) the unique max-distance voxel.
  (*td.gbEuclideanDistances)[13] = 1.5f;

  // Quats: voxel 13 = identity (reference); others = 5deg.
  for(usize i = 0; i < 18; ++i)
  {
    const auto& q = (i == 13) ? qIdentity : q5;
    (*td.quats)[i * 4 + 0] = q[0];
    (*td.quats)[i * 4 + 1] = q[1];
    (*td.quats)[i * 4 + 2] = q[2];
    (*td.quats)[i * 4 + 3] = q[3];
  }

  ComputeFeatureReferenceMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(1);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(td.ds, fs::path(fmt::format("{}/find_feature_reference_misorientations_1_1_3D.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif
  // FRM expected: voxel 13 = 0 (vs itself), others = 5deg.
  for(usize i = 0; i < 18; ++i)
  {
    DataFixtures::RequireFRMClose(td.ds, i, (i == 13) ? 0.0f : 5.0f);
  }
  // avg = (0 + 5*17) / 18 = 85/18 deg.
  DataFixtures::RequireAvgClose(td.ds, 1, 85.0f / 18.0f);

  // EuclideanCenters[1] should be coords of voxel 13. Spacing=1, origin=0, getCoordsf returns
  // cell-center coords: voxel 13 is at (x=1, y=1, z=1) -> center coords (1.5, 1.5, 1.5).
  const auto& centers = td.ds.getDataRefAs<Float32Array>(DataFixtures::k_CellFeatureDataPath.createChildPath(DataFixtures::k_FeatureEuclideanCentersOutName));
  REQUIRE(centers[1 * 3 + 0] == Approx(1.5f).margin(1e-5f));
  REQUIRE(centers[1 * 3 + 1] == Approx(1.5f).margin(1e-5f));
  REQUIRE(centers[1 * 3 + 2] == Approx(1.5f).margin(1e-5f));

  DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/true);
}

// Class 4 sweep: re-run each Class 1 fixture with the goal of asserting Class 4 invariants only,
// without per-value comparison. Catches future regressions where specific values shift but
// invariants still hold (vs. the value-specific Class 1 fixtures above which would catch the
// specific shift but might be more brittle to legitimate refactors).
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 4 - Invariants Sweep", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Sweep 1: Mode 0 with a 3x3x1 grid mixing valid voxels, a background voxel, and un-phased voxels
  // across 3 features (different from the value-specific Mode 0 fixture above).
  {
    DataFixtures::ToyData td = DataFixtures::CreateScaffold(3, 3, 1, 3);
    const auto q0 = DataFixtures::QuatFromPhi1Deg(0.0f);
    const auto q7 = DataFixtures::QuatFromPhi1Deg(7.0f);
    // Voxels: bg, f1, f1, f1, f2(un-phased), f2, bg, f2, f1
    const std::array<std::pair<int32, int32>, 9> layout = {{{0, 0}, {1, 1}, {1, 1}, {1, 1}, {2, 0}, {2, 1}, {0, 0}, {2, 1}, {1, 1}}};
    for(usize i = 0; i < 9; ++i)
    {
      (*td.featureIds)[i] = layout[i].first;
      (*td.cellPhases)[i] = layout[i].second;
      const auto& q = (i % 2 == 0) ? q0 : q7;
      (*td.quats)[i * 4 + 0] = q[0];
      (*td.quats)[i * 4 + 1] = q[1];
      (*td.quats)[i * 4 + 2] = q[2];
      (*td.quats)[i * 4 + 3] = q[3];
    }
    (*td.avgQuats)[1 * 4 + 0] = q0[0];
    (*td.avgQuats)[1 * 4 + 1] = q0[1];
    (*td.avgQuats)[1 * 4 + 2] = q0[2];
    (*td.avgQuats)[1 * 4 + 3] = q0[3];
    (*td.avgQuats)[2 * 4 + 0] = q7[0];
    (*td.avgQuats)[2 * 4 + 1] = q7[1];
    (*td.avgQuats)[2 * 4 + 2] = q7[2];
    (*td.avgQuats)[2 * 4 + 3] = q7[3];

    ComputeFeatureReferenceMisorientationsFilter filter;
    Arguments args = DataFixtures::BuildArgs(0);
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/false);
  }

  // Sweep 2: Mode 1 with the Fixture-D config (re-run, assert invariants only).
  {
    DataFixtures::ToyData td = DataFixtures::CreateScaffold(3, 3, 1, 2);
    const auto qIdentity = DataFixtures::QuatFromPhi1Deg(0.0f);
    const auto q5 = DataFixtures::QuatFromPhi1Deg(5.0f);
    for(usize i = 0; i < 9; ++i)
    {
      (*td.featureIds)[i] = 1;
      (*td.cellPhases)[i] = 1;
      const auto& q = (i == 4) ? qIdentity : q5;
      (*td.quats)[i * 4 + 0] = q[0];
      (*td.quats)[i * 4 + 1] = q[1];
      (*td.quats)[i * 4 + 2] = q[2];
      (*td.quats)[i * 4 + 3] = q[3];
    }
    (*td.gbEuclideanDistances)[4] = 1.5f;
    for(usize i = 0; i < 9; ++i)
    {
      if(i != 4)
      {
        (*td.gbEuclideanDistances)[i] = 0.5f;
      }
    }

    ComputeFeatureReferenceMisorientationsFilter filter;
    Arguments args = DataFixtures::BuildArgs(1);
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    DataFixtures::AssertClass4Invariants(td.ds, /*isMode1=*/true);
  }
}
