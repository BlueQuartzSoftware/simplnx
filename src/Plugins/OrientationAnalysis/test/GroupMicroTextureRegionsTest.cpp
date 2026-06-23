#include "OrientationAnalysis/Filters/GroupMicroTextureRegionsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <set>

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
const DataPath k_FeatureDataPath = k_ImageGeomPath.createChildPath("CellFeatureData");
const DataPath k_EnsembleDataPath = k_ImageGeomPath.createChildPath("CellEnsembleData");

const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_FeaturePhasesName = "FeaturePhases";
const std::string k_VolumesName = "Volumes";
const std::string k_AvgQuatsName = "AvgQuats";
const std::string k_ContigNeighborListName = "ContiguousNeighborList";
const std::string k_CrystalStructuresName = "CrystalStructures";

const std::string k_NewFeatureAMName = "MicroTextureFeatureData";
const std::string k_CellParentIdsName = "CellParentIds";
const std::string k_FeatureParentIdsName = "FeatureParentIds";
const std::string k_ActiveName = "Active";
const std::string k_SeedArrayName = "GroupMicroTextureRegions_Seed";

const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath(k_FeatureIdsName);
const DataPath k_FeaturePhasesPath = k_FeatureDataPath.createChildPath(k_FeaturePhasesName);
const DataPath k_VolumesPath = k_FeatureDataPath.createChildPath(k_VolumesName);
const DataPath k_AvgQuatsPath = k_FeatureDataPath.createChildPath(k_AvgQuatsName);
const DataPath k_ContigNeighborListPath = k_FeatureDataPath.createChildPath(k_ContigNeighborListName);
const DataPath k_CrystalStructuresPath = k_EnsembleDataPath.createChildPath(k_CrystalStructuresName);
const DataPath k_NewFeatureAMPath = k_ImageGeomPath.createChildPath(k_NewFeatureAMName);

// Quaternion for a pure Bunge ZXZ Euler rotation (phi1=0, Phi=phiDeg, phi2=0). This is a pure
// rotation about the x-axis by phiDeg degrees, which tilts the crystal c-axis (originally along
// +z in crystal frame) by phiDeg degrees in the sample y-z plane. For two features with pure-Phi
// tilts of phiA and phiB degrees, the c-axis angular distance is |phiA - phiB| degrees, folded
// into [0, 90] via the (pi - w) symmetry in the algorithm. Storage convention: {x, y, z, w}.
std::array<float32, 4> QuatFromPhiDeg(float32 phiDeg)
{
  const float32 halfAngleRad = (phiDeg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {std::sin(halfAngleRad), 0.0f, 0.0f, std::cos(halfAngleRad)};
}

struct FixtureData
{
  DataStructure ds;
  ImageGeom* geom = nullptr;
  AttributeMatrix* cellAM = nullptr;
  AttributeMatrix* featureAM = nullptr;
  AttributeMatrix* ensembleAM = nullptr;
  Int32Array* featureIds = nullptr;
  Int32Array* featurePhases = nullptr;
  Float32Array* volumes = nullptr;
  Float32Array* avgQuats = nullptr;
  NeighborList<int32>* neighborList = nullptr;
  UInt32Array* crystalStructures = nullptr;
};

// Build a minimal {nX,1,1} ImageGeom with one cell per feature (FeatureIds[i] = i+1, so cell 0
// belongs to feature 1, cell 1 to feature 2, etc.). Background feature 0 is allocated but no
// cell is assigned to it; it exists only so the FeaturePhases / NeighborList arrays have a 0-th
// tuple available. CrystalStructures[0] is a sentinel; CrystalStructures[1] is Hexagonal_High.
FixtureData CreateScaffold(usize numFeatures)
{
  FixtureData td;
  const usize nX = numFeatures - 1; // one cell per real feature
  const usize numCells = nX;
  const usize numCrystalStructures = 2;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({nX, 1, 1});

  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", ShapeType{1, 1, nX}, td.geom->getId());
  td.featureAM = AttributeMatrix::Create(td.ds, "CellFeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "CellEnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  td.featureIds = CreateTestDataArray<int32>(td.ds, k_FeatureIdsName, {1, 1, nX}, {1}, td.cellAM->getId());
  td.featurePhases = CreateTestDataArray<int32>(td.ds, k_FeaturePhasesName, {numFeatures}, {1}, td.featureAM->getId());
  td.volumes = CreateTestDataArray<float32>(td.ds, k_VolumesName, {numFeatures}, {1}, td.featureAM->getId());
  td.avgQuats = CreateTestDataArray<float32>(td.ds, k_AvgQuatsName, {numFeatures}, {4}, td.featureAM->getId());
  td.neighborList = NeighborList<int32>::Create(td.ds, k_ContigNeighborListName, ShapeType{numFeatures}, td.featureAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numCrystalStructures}, {1}, td.ensembleAM->getId());

  // One cell per real feature (cell k -> feature k+1).
  for(usize k = 0; k < numCells; k++)
  {
    (*td.featureIds)[k] = static_cast<int32>(k + 1);
  }

  // Feature 0 is the background; real features get phase 1 (Hex_High).
  (*td.featurePhases)[0] = 0;
  for(usize f = 1; f < numFeatures; f++)
  {
    (*td.featurePhases)[f] = 1;
  }

  // Default volumes to 1.0 (uniform). Caller overrides if running-average behaviour matters.
  for(usize f = 0; f < numFeatures; f++)
  {
    (*td.volumes)[f] = 1.0f;
  }

  // Identity quaternion {x=0, y=0, z=0, w=1} everywhere by default — caller overrides per feature.
  for(usize f = 0; f < numFeatures; f++)
  {
    (*td.avgQuats)[f * 4 + 0] = 0.0f;
    (*td.avgQuats)[f * 4 + 1] = 0.0f;
    (*td.avgQuats)[f * 4 + 2] = 0.0f;
    (*td.avgQuats)[f * 4 + 3] = 1.0f;
  }

  // Empty neighbor list per feature by default — caller overrides.
  for(usize f = 0; f < numFeatures; f++)
  {
    td.neighborList->setList(static_cast<int32>(f), std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  }

  (*td.crystalStructures)[0] = 999u; // sentinel
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);

  return td;
}

void SetAvgQuat(FixtureData& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avgQuats)[featureIdx * 4 + 0] = q[0];
  (*td.avgQuats)[featureIdx * 4 + 1] = q[1];
  (*td.avgQuats)[featureIdx * 4 + 2] = q[2];
  (*td.avgQuats)[featureIdx * 4 + 3] = q[3];
}

void SetNeighbors(FixtureData& td, int32 featureIdx, std::vector<int32> neighbors)
{
  td.neighborList->setList(featureIdx, std::make_shared<std::vector<int32>>(std::move(neighbors)));
}

// Build the canonical 5-feature pure-Phi Bunge fixture used by both the Pure-Phi Class 1 test
// and the RandomizeParentIds invariance test. 5 real features (1..5) + background feature 0.
// Phi: F1=0, F2=5, F3=60, F4=63, F5=25 (degrees). Contiguous neighbor adjacency:
//   F1 -- F2 -- F3 -- F4    and    F5 (isolated)
// Under a 10 deg tolerance the expected groupings are: {F1,F2}, {F3,F4}, {F5}.
FixtureData Build5FeaturePureBunge()
{
  FixtureData td = CreateScaffold(/*numFeatures=*/6);

  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(5.0f));
  SetAvgQuat(td, 3, QuatFromPhiDeg(60.0f));
  SetAvgQuat(td, 4, QuatFromPhiDeg(63.0f));
  SetAvgQuat(td, 5, QuatFromPhiDeg(25.0f));

  SetNeighbors(td, 1, {2});
  SetNeighbors(td, 2, {1, 3});
  SetNeighbors(td, 3, {2, 4});
  SetNeighbors(td, 4, {3});
  SetNeighbors(td, 5, {});

  return td;
}

Arguments BuildArgs(float32 cAxisToleranceDeg, bool useRunningAverage, bool randomizeParentIds, uint64 seed)
{
  Arguments args;
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(k_ContigNeighborListPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(k_ContigNeighborListPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_UseRunningAverage_Key, std::make_any<bool>(useRunningAverage));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_CAxisTolerance_Key, std::make_any<float32>(cAxisToleranceDeg));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_VolumesArrayPath_Key, std::make_any<DataPath>(k_VolumesPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_AvgQuatsPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_NewCellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(k_NewFeatureAMPath));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_CellParentIdsArrayName_Key, std::make_any<std::string>(k_CellParentIdsName));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_FeatureParentIdsArrayName_Key, std::make_any<std::string>(k_FeatureParentIdsName));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_RandomizeParentIds_Key, std::make_any<bool>(randomizeParentIds));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_SeedValue_Key, std::make_any<uint64>(seed));
  args.insertOrAssign(GroupMicroTextureRegionsFilter::k_SeedArrayName_Key, std::make_any<std::string>(k_SeedArrayName));
  return args;
}
} // namespace AnalyticalFixtures
} // namespace

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Pure-Phi Bunge)", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class1]")
{
  using namespace AnalyticalFixtures;

  // See Build5FeaturePureBunge() for the layout: 5 real features arranged so that the expected
  // groupings under a 10 deg tolerance are {F1,F2}, {F3,F4}, {F5}.
  FixtureData td = Build5FeaturePureBunge();

  // Tolerance 10 deg, UseRunningAverage=false (compare against seed's c-axis), RandomizeParentIds=false
  // (deterministic parent-id assignment), Seed=42 (deterministic seed-iteration order).
  Arguments args = BuildArgs(/*cAxisToleranceDeg=*/10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/false, /*seed=*/42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName)));
  const auto& featureParentIds = td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_CellParentIdsName)));
  const auto& cellParentIds = td.ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_CellParentIdsName));

  // Class 1 (Analytical) — grouping outcome: paired features share a parent, non-paired don't.
  CHECK(featureParentIds[1] == featureParentIds[2]); // F1, F2 grouped
  CHECK(featureParentIds[3] == featureParentIds[4]); // F3, F4 grouped
  CHECK(featureParentIds[1] != featureParentIds[3]); // F1/F2 group != F3/F4 group
  CHECK(featureParentIds[5] != featureParentIds[1]); // F5 is alone, different from F1/F2 group
  CHECK(featureParentIds[5] != featureParentIds[3]); // F5 is alone, different from F3/F4 group

  // Class 4 (Invariant) — all real features assigned a positive parent id; three distinct groups.
  CHECK(featureParentIds[1] > 0);
  CHECK(featureParentIds[2] > 0);
  CHECK(featureParentIds[3] > 0);
  CHECK(featureParentIds[4] > 0);
  CHECK(featureParentIds[5] > 0);
  std::set<int32> distinctParents{featureParentIds[1], featureParentIds[2], featureParentIds[3], featureParentIds[4], featureParentIds[5]};
  CHECK(distinctParents.size() == 3);

  // Class 4 (Invariant) — cell parent ids must equal feature parent ids of the underlying feature.
  REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath));
  const auto& featureIds = td.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath);
  for(usize k = 0; k < featureIds.getNumberOfTuples(); k++)
  {
    CHECK(cellParentIds[k] == featureParentIds[featureIds[k]]);
  }

  // New feature attribute matrix should be sized to >= maxParent + 1 (index 0 reserved).
  const auto& newFeatureAM = td.ds.getDataRefAs<AttributeMatrix>(k_NewFeatureAMPath);
  int32 maxParent = 0;
  for(usize f = 1; f < featureParentIds.getNumberOfTuples(); f++)
  {
    if(featureParentIds[f] > maxParent)
    {
      maxParent = featureParentIds[f];
    }
  }
  CHECK(newFeatureAM.getNumberOfTuples() == static_cast<usize>(maxParent + 1));

  // Active array: present and sized to AM, default value true everywhere.
  REQUIRE_NOTHROW(td.ds.getDataRefAs<BoolArray>(k_NewFeatureAMPath.createChildPath(k_ActiveName)));
  const auto& active = td.ds.getDataRefAs<BoolArray>(k_NewFeatureAMPath.createChildPath(k_ActiveName));
  CHECK(active.getNumberOfTuples() == newFeatureAM.getNumberOfTuples());

  // Seed value array: written and contains the seed we asked for.
  REQUIRE_NOTHROW(td.ds.getDataRefAs<UInt64Array>(DataPath({k_SeedArrayName})));
  CHECK(td.ds.getDataRefAs<UInt64Array>(DataPath({k_SeedArrayName}))[0] == 42ULL);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: RandomizeParentIds invariants", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class4]")
{
  // Randomization shuffles the parent-id LABELS but cannot change the GROUPING. The Class 4
  // (Invariant) assertions below are what randomization must preserve:
  //   (a) Same equivalence classes — features that grouped before still group together after.
  //   (b) Same number of distinct groups.
  //   (c) Cell parent ids agree with feature parent ids of the underlying feature.
  //   (d) Parent ids stay positive (0 is reserved for unassigned).
  //   (e) Same seed -> identical shuffle across runs.
  // We do NOT assert that "the shuffle differs from the identity permutation" because some
  // small-N seed combinations may legitimately yield the identity; the framework property is
  // determinism, not non-identity. The non-identity sanity check is done loosely at the end
  // by comparing against the non-randomized baseline.
  using namespace AnalyticalFixtures;

  // Baseline run: deterministic parent ids (no shuffle).
  FixtureData tdA = Build5FeaturePureBunge();
  Arguments argsA = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/false, /*seed=*/42ULL);
  GroupMicroTextureRegionsFilter filter;
  {
    auto preflightResult = filter.preflight(tdA.ds, argsA);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(tdA.ds, argsA);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const auto& parentIdsA = tdA.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));

  // Randomized run with seed=42.
  FixtureData tdB = Build5FeaturePureBunge();
  Arguments argsB = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/true, /*seed=*/42ULL);
  {
    auto preflightResult = filter.preflight(tdB.ds, argsB);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(tdB.ds, argsB);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const auto& parentIdsB = tdB.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  const auto& cellParentIdsB = tdB.ds.getDataRefAs<Int32Array>(k_CellDataPath.createChildPath(k_CellParentIdsName));
  const auto& featureIdsB = tdB.ds.getDataRefAs<Int32Array>(k_FeatureIdsPath);

  // Second randomized run with seed=42 — for the determinism invariant.
  FixtureData tdC = Build5FeaturePureBunge();
  Arguments argsC = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/true, /*seed=*/42ULL);
  {
    auto preflightResult = filter.preflight(tdC.ds, argsC);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(tdC.ds, argsC);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const auto& parentIdsC = tdC.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));

  // (a) Equivalence classes preserved between A (no shuffle) and B (shuffled): for every pair
  //     of features, they share a parent id in A iff they share one in B.
  for(int32 i = 1; i <= 5; i++)
  {
    for(int32 j = i + 1; j <= 5; j++)
    {
      const bool sameInA = parentIdsA[i] == parentIdsA[j];
      const bool sameInB = parentIdsB[i] == parentIdsB[j];
      CHECK(sameInA == sameInB);
    }
  }

  // (b) Same number of distinct groups before and after the shuffle (must equal the 3 hand-derived).
  std::set<int32> distinctA{parentIdsA[1], parentIdsA[2], parentIdsA[3], parentIdsA[4], parentIdsA[5]};
  std::set<int32> distinctB{parentIdsB[1], parentIdsB[2], parentIdsB[3], parentIdsB[4], parentIdsB[5]};
  CHECK(distinctA.size() == 3);
  CHECK(distinctB.size() == 3);

  // (c) Cell parent ids agree with feature parent ids of the underlying feature, post-shuffle.
  for(usize k = 0; k < featureIdsB.getNumberOfTuples(); k++)
  {
    CHECK(cellParentIdsB[k] == parentIdsB[featureIdsB[k]]);
  }

  // (d) Positivity preserved post-shuffle.
  for(int32 f = 1; f <= 5; f++)
  {
    CHECK(parentIdsB[f] > 0);
  }

  // (e) Determinism: same seed -> identical shuffle result. Compare every feature's parent id.
  for(usize f = 0; f < parentIdsB.getNumberOfTuples(); f++)
  {
    CHECK(parentIdsB[f] == parentIdsC[f]);
  }

  // Loose non-identity check: at least one feature's parent id changed after shuffling. With the
  // 5-feature / 3-group setup and the std::mt19937_64 default-seed-driven shuffle, the identity
  // permutation is exceedingly unlikely; a hit here would mean the shuffle didn't run at all.
  bool anyDifferent = false;
  for(int32 f = 1; f <= 5; f++)
  {
    if(parentIdsA[f] != parentIdsB[f])
    {
      anyDifferent = true;
      break;
    }
  }
  CHECK(anyDifferent);

  UnitTest::CheckArraysInheritTupleDims(tdB.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Tolerance Boundary)", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Class1]")
{
  using namespace AnalyticalFixtures;

  // 3 real features on a chain F1 -- F2 -- F3. F1 c-axis at Phi=0, F2 at Phi=8, F3 at Phi=20.
  //   - F1 -- F2 : 8 deg  -- under 10 deg tolerance -> GROUP
  //   - F2 -- F3 : 12 deg -- over 10 deg tolerance using F2's c-axis (since UseRunningAverage=false,
  //                          the algorithm compares each candidate to the BFS seed's c-axis, but
  //                          inside the BFS walk over already-grouped features, comparison is from
  //                          THAT feature's c-axis, not the original seed's) -> DO NOT BRIDGE
  // Expected: 2 distinct groups -> {F1, F2}, {F3}.
  FixtureData td = CreateScaffold(/*numFeatures=*/4);

  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(8.0f));
  SetAvgQuat(td, 3, QuatFromPhiDeg(20.0f));

  SetNeighbors(td, 1, {2});
  SetNeighbors(td, 2, {1, 3});
  SetNeighbors(td, 3, {2});

  Arguments args = BuildArgs(10.0f, /*useRunningAverage=*/false, /*randomizeParentIds=*/false, /*seed=*/42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& featureParentIds = td.ds.getDataRefAs<Int32Array>(k_FeatureDataPath.createChildPath(k_FeatureParentIdsName));
  CHECK(featureParentIds[1] == featureParentIds[2]);
  CHECK(featureParentIds[2] != featureParentIds[3]);
  CHECK(featureParentIds[1] > 0);
  CHECK(featureParentIds[3] > 0);
  std::set<int32> distinctParents{featureParentIds[1], featureParentIds[2], featureParentIds[3]};
  CHECK(distinctParents.size() == 2);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: Regression — runs in default UseNonContiguousNeighbors=false mode", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][Regression]")
{
  // Pins the defect-A fix: prior to the fix, execute() unconditionally returned error -99345 when
  // UseNonContiguousNeighbors=false because the null-pointer guard on the non-contiguous list was
  // outside the if-block that populated it. Filter could not run in its primary mode.
  using namespace AnalyticalFixtures;

  FixtureData td = CreateScaffold(/*numFeatures=*/3);
  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(2.0f));
  SetNeighbors(td, 1, {2});
  SetNeighbors(td, 2, {1});

  Arguments args = BuildArgs(10.0f, false, false, 42ULL);

  GroupMicroTextureRegionsFilter filter;
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

TEST_CASE("OrientationAnalysis::GroupMicroTextureRegionsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][GroupMicroTextureRegionsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "GroupMicroTextureRegionsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "GroupMicroTextureRegionsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<GroupMicroTextureRegionsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(GroupMicroTextureRegionsFilter::k_ActiveArrayName_Key) == "TestName");
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<float32>(GroupMicroTextureRegionsFilter::k_CAxisTolerance_Key) == 2.5f);
      CHECK(args.value<std::string>(GroupMicroTextureRegionsFilter::k_CellParentIdsArrayName_Key) == "TestName");
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_ContiguousNeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(GroupMicroTextureRegionsFilter::k_FeatureParentIdsArrayName_Key) == "TestName");
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_NewCellFeatureAttributeMatrixName_Key) == DataPath({"TestName"}));
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_NonContiguousNeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<bool>(GroupMicroTextureRegionsFilter::k_UseNonContiguousNeighbors_Key) == true);
      CHECK(args.value<bool>(GroupMicroTextureRegionsFilter::k_UseRunningAverage_Key) == true);
      CHECK(args.value<DataPath>(GroupMicroTextureRegionsFilter::k_VolumesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
