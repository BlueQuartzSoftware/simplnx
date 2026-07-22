#include "OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <set>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

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

// ---------------------------------------------------------------------------
// Class 1 (Analytical) oracle derivation
// ---------------------------------------------------------------------------
// Quaternion for a pure Bunge ZXZ Euler rotation (phi1=0, Phi=phiDeg, phi2=0). This is a pure
// rotation about the x-axis by phiDeg degrees, which tilts the crystal c-axis (originally along
// +z in the crystal frame) by phiDeg degrees within the sample y-z plane:
//
//   c_sample = g^T * [0,0,1] = (0, +/-sin(Phi), cos(Phi))
//
// For two cells with pure-Phi tilts of phiA and phiB degrees the dot product of their sample-frame
// c-axes is exactly cos(phiA - phiB), so the c-axis angular distance is exactly |phiA - phiB|
// degrees. The algorithm accepts a pair when w <= tol OR (pi - w) <= tol, i.e. the effective
// metric is min(|phiA - phiB|, 180 - |phiA - phiB|), folded into [0, 90]. Expected segmentations
// below follow in closed form from the per-cell Phi values, the tolerance, and grid adjacency.
// Storage convention (shared with the sibling OA Class 1 fixtures): {x, y, z, w}.
std::array<float32, 4> QuatFromPhiDeg(float32 phiDeg)
{
  const float32 halfAngleRad = (phiDeg * 0.5f) * Constants::k_PiOver180F;
  return {std::sin(halfAngleRad), 0.0f, 0.0f, std::cos(halfAngleRad)};
}

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

// Build a minimal ImageGeom of the given (x, y, z) dimensions with a Cell AttributeMatrix holding
// Quats (identity rotation) + Phases (all phase 1) and a CellEnsembleData AttributeMatrix holding
// CrystalStructures. CrystalStructures[0] is the conventional "unknown" sentinel (999);
// CrystalStructures[1..] default to Hexagonal_High. Tests override per-cell Phi tilts, phases,
// ensemble entries, and add mask arrays as needed.
FixtureData CreateScaffold(usize dimX, usize dimY, usize dimZ, usize numEnsembles = 2)
{
  FixtureData td;
  const usize numCells = dimX * dimY * dimZ;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({dimX, dimY, dimZ});

  // AttributeMatrix tuple shape is (z, y, x)
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

// Preflight + execute; returns the execute result for error-path tests.
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

// Verifies the Class 4 invariants shared by every successful run: the feature AttributeMatrix has
// (numFeatures + 1) tuples, Active[0] == 0 (index 0 is reserved for "unassigned"), and every real
// feature is flagged active.
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Pure-Phi Chain, Face)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // 8x1x1 chain, tolerance 10 degrees. Phi per cell: [0, 5, 8, 45, 50, 120, 124, 90].
  // Pairwise folded c-axis distances along the chain:
  //   0-1: 5   (group)   1-2: 3  (group)   2-3: 37 (break)
  //   3-4: 5   (group)   4-5: 70 (break)
  //   5-6: 4   (group)   6-7: 34 (break)
  // Expected features: F1={0,1,2}, F2={3,4}, F3={5,6}, F4={7}.
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Pi-Fold Antiparallel C-Axes)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // The c-axis is a direction, not a vector: the algorithm folds the angle via (pi - w) <= tol.
  // Phi = 2 and Phi = 176 give nearly antiparallel c-axes (174 apart) whose folded distance is
  // 6 degrees -> same feature at tolerance 10. Phi = 88 vs 176: folded distance 88 -> break.
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Neighbor Scheme Face vs All)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // 2x2x1 grid (cell index = y*2 + x):
  //   cell 0 (0,0): Phi 0     cell 1 (1,0): Phi 45
  //   cell 2 (0,1): Phi 90    cell 3 (1,1): Phi 0
  // All face-adjacent pairs differ by >= 45 degrees -> 4 singleton features under the Face scheme.
  // The diagonal pair 0-3 is identical (0 degrees) -> the All scheme merges them across the corner.
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Mask Excludes Voxel 0)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Regression pin: the first seed must be validated by getSeed() exactly like every later seed.
  // 5x1x1, Phi = [0, 20, 22, 0, 90], mask = [0, 1, 1, 0, 1], tolerance 10.
  // Voxel 0 is masked out, so the first real seed is cell 1: F1={1,2} (distance 2), cell 3 is
  // masked, F2={4}. Masked cells keep FeatureId 0. A driver loop that bursts from the raw index 0
  // without consulting getSeed() produces a phantom empty feature 1 and shifted ids [0,2,2,0,3].
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Phase Separation)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Cells only group when their phases match, even with identical orientations. 4x1x1, all
  // Phi = 0, phases = [1, 1, 2, 2] -> two features split at the phase boundary. Phase 2 uses
  // Hexagonal_Low to also exercise the 6/m acceptance branch of the crystal-structure validation.
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (RectGrid Geometry)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Regression pin: the geometry parameter accepts RectGrid in addition to Image, so the
  // algorithm must fetch the geometry as IGridGeometry (a stale ImageGeom cast produced a null
  // pointer and crashed). 3x1x1 RectGrid with non-uniform x bounds, Phi = [0, 5, 45], tol 10:
  // cells 0-1 group (distance 5), cell 2 breaks (distance 40).
  DataStructure dataStructure;
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 4 Invariants (RandomizeFeatureIds)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Randomization must relabel, never repartition. Re-uses the Pure-Phi Chain fixture whose
  // ground-truth partition is {0,1,2} {3,4} {5,6} {7}. Invariants: the partition survives, the
  // ids used are exactly a permutation of {1..4}, and the shuffle is deterministic (static seed).
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

  // Partition invariant: cells designed to share a feature still do; boundaries still hold.
  REQUIRE(firstRun[0] == firstRun[1]);
  REQUIRE(firstRun[1] == firstRun[2]);
  REQUIRE(firstRun[3] == firstRun[4]);
  REQUIRE(firstRun[5] == firstRun[6]);
  const std::set<int32> distinctIds = {firstRun[0], firstRun[3], firstRun[5], firstRun[7]};
  REQUIRE(distinctIds.size() == 4);

  // Permutation invariant: the relabeled ids are exactly {1, 2, 3, 4}.
  REQUIRE(distinctIds == std::set<int32>{1, 2, 3, 4});

  // Determinism invariant: the shuffle uses a fixed seed, so a second run is bit-identical.
  const std::vector<int32> secondRun = runOnce();
  REQUIRE(firstRun == secondRun);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Phase 0 (Unindexed) Cells Tolerated", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Regression pin: EBSD datasets conventionally store unindexed points as phase 0, and
  // CrystalStructures[0] is the 999 "unknown" sentinel. The crystal-structure validation must skip
  // phase-0 cells instead of rejecting the whole dataset. Phase-0 cells can never seed a feature
  // (getSeed requires phase > 0) nor join one (grouping requires equal phases), so cell 0 keeps
  // FeatureId 0 and the remaining identical-orientation cells form one feature.
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

  // A user may legitimately mask out a non-hexagonal phase and segment only the hexagonal cells.
  // The crystal-structure validation must not reject cells that the mask already excludes.
  // 4x1x1, phases = [1, 1, 2, 2] with phase 2 = Cubic_High, mask = [1, 1, 0, 0].
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

  // Unmasked cubic cells must be rejected: c-axis misalignment is only defined for hexagonal
  // (6/m or 6/mmm) Laue classes.
  FixtureData td = CreateScaffold(2, 1, 1);
  (*td.crystalStructures)[1] = ebsdlib::CrystalStructure::Cubic_High;

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -8363);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - Phase Out of Ensemble Bounds (-8364)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // A cell phase value with no corresponding CrystalStructures tuple must produce a clean error,
  // not an out-of-bounds read.
  FixtureData td = CreateScaffold(2, 1, 1);
  (*td.phases)[1] = 7;

  auto executeResult = RunFilter(td.ds, BuildArgs(10.0f, segment_features::k_6NeighborIndex, false));
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -8364);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - No Features Found (-87000)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Regression pin: with every cell masked out no seed exists, so the filter must fail with
  // -87000 instead of reporting a phantom feature from an unvalidated first seed.
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

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Cell array tuple count mismatch (-651)", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Build a minimal synthetic DataStructure where the two cell-level arrays that are
  // validated together (Quats and CellPhases) do NOT share the same tuple count. This
  // drives the validateNumberOfTuples() guard in preflightImpl that emits error -651.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAM->getId());

  // CellPhases lives in a separate AttributeMatrix with a deliberately different tuple
  // count (9 != 10) so the cross-array tuple-count check fails.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  CAxisSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Quats"})));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));

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
