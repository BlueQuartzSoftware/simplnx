#include "OrientationAnalysis/Filters/ComputeFeatureNeighborMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

// =============================================================================
// V&V Class 1 (Analytical) + Class 4 (Invariant) oracle support — added 2026-06-02.
//
// These fixtures replace the regression-against-archive pattern (the exemplar tests that consume
// `6_6_stats_test_v2.tar.gz`) with hand-derived analytical inputs and expected per-neighbor
// misorientation lists + per-feature averages. The fixtures specifically include a "bug-exposing"
// configuration that surfaces the divisor bug at algorithm `.cpp` line 75 (`tempMisoList =
// featureNeighborList.size();` inside the inner j-loop, clobbering the per-mismatch decrement).
//
// Reference: src/Plugins/OrientationAnalysis/vv/ComputeFeatureNeighborMisorientationsFilter.md
// =============================================================================

// Wrapped in an anonymous namespace so every symbol below has internal linkage. Sibling
// OrientationAnalysis test TUs declare their own `DataFixtures` namespace with same-named
// (and same-signature) entities such as `CreateScaffold`; without internal linkage the linker
// merges those duplicate definitions into one, silently giving this TU another file's scaffold.
namespace
{
namespace DataFixtures
{
const std::string k_GeomName = "ImageGeometry";
const DataPath k_ImageGeomPath = DataPath({k_GeomName});
const DataPath k_FeatureDataPath = k_ImageGeomPath.createChildPath("FeatureData");
const DataPath k_EnsembleDataPath = k_ImageGeomPath.createChildPath("EnsembleData");

const std::string k_FeaturePhasesName = "FeaturePhases";
const std::string k_AvgQuatsName = "AvgQuats";
const std::string k_NeighborListName = "NeighborList";
const std::string k_CrystalStructuresName = "CrystalStructures";

const std::string k_MisorientationListOutName = "MisorientationListOut";
const std::string k_AvgMisorientationsOutName = "AvgMisorientationsOut";

inline std::array<float32, 4> QuatFromPhi1Deg(float32 phi1Deg)
{
  const float32 halfAngleRad = (phi1Deg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {0.0f, 0.0f, std::sin(halfAngleRad), std::cos(halfAngleRad)};
}

struct ToyData
{
  DataStructure ds;
  ImageGeom* geom = nullptr;
  AttributeMatrix* featureAM = nullptr;
  AttributeMatrix* ensembleAM = nullptr;
  Int32Array* featurePhases = nullptr;
  Float32Array* avgQuats = nullptr;
  NeighborList<int32>* neighborList = nullptr;
  UInt32Array* crystalStructures = nullptr;
};

// Build a scaffold with a tiny ImageGeom + a feature AM (size numFeatures) + ensemble AM
// (size numCrystalStructures). All input arrays are initialized; caller populates per-feature values.
inline ToyData CreateScaffold(usize numFeatures, usize numCrystalStructures)
{
  ToyData td;
  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({1, 1, 1}); // unused at the cell level; only present so the geom container has shape

  td.featureAM = AttributeMatrix::Create(td.ds, "FeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "EnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  td.featurePhases = CreateTestDataArray<int32>(td.ds, k_FeaturePhasesName, {numFeatures}, {1}, td.featureAM->getId());
  td.avgQuats = CreateTestDataArray<float32>(td.ds, k_AvgQuatsName, {numFeatures}, {4}, td.featureAM->getId());
  td.neighborList = NeighborList<int32>::Create(td.ds, k_NeighborListName, ShapeType{numFeatures}, td.featureAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numCrystalStructures}, {1}, td.ensembleAM->getId());

  // Default: feature 0 sentinel; all other features phase=0 (unassigned); identity quats.
  for(usize i = 0; i < numFeatures; ++i)
  {
    (*td.featurePhases)[i] = 0;
    (*td.avgQuats)[i * 4 + 0] = 0.0f;
    (*td.avgQuats)[i * 4 + 1] = 0.0f;
    (*td.avgQuats)[i * 4 + 2] = 0.0f;
    (*td.avgQuats)[i * 4 + 3] = 1.0f;
    td.neighborList->setList(i, std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  }
  // Default crystal structures: index 0 sentinel; index 1 Cubic_High; subsequent left as zeros to be set by caller.
  (*td.crystalStructures)[0] = 999u;
  if(numCrystalStructures > 1)
  {
    (*td.crystalStructures)[1] = 1u; // Cubic_High (EbsdLib LaueOps index)
  }
  return td;
}

inline void SetAvgQuat(ToyData& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avgQuats)[featureIdx * 4 + 0] = q[0];
  (*td.avgQuats)[featureIdx * 4 + 1] = q[1];
  (*td.avgQuats)[featureIdx * 4 + 2] = q[2];
  (*td.avgQuats)[featureIdx * 4 + 3] = q[3];
}

inline Arguments BuildArgs(bool computeAvgMisors)
{
  Arguments args;
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_ComputeAvgMisors_Key, std::make_any<bool>(computeAvgMisors));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(k_FeatureDataPath.createChildPath(k_NeighborListName)));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_FeatureDataPath.createChildPath(k_AvgQuatsName)));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeatureDataPath.createChildPath(k_FeaturePhasesName)));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_EnsembleDataPath.createChildPath(k_CrystalStructuresName)));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_MisorientationListArrayName_Key, std::make_any<std::string>(k_MisorientationListOutName));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_AvgMisorientationsArrayName_Key, std::make_any<std::string>(k_AvgMisorientationsOutName));
  return args;
}

inline const NeighborList<float32>& GetOutputMisorientationList(const DataStructure& ds)
{
  return ds.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_MisorientationListOutName));
}

inline const Float32Array& GetOutputAvgMisorientations(const DataStructure& ds)
{
  return ds.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgMisorientationsOutName));
}
} // namespace DataFixtures
} // namespace

// Retired 2026-06-02 (V&V cycle): the main exemplar-comparison TEST_CASE that consumed
// `6_6_stats_test_v2.tar.gz` and the `[.][UNIMPLEMENTED][!mayfail]` stub TEST_CASE for
// `Misorientation Per Feature` were removed. The exemplar arrays in the archive were a circular
// oracle (regenerated from pre-EbsdLib-2.4.1 SIMPLNX output); the precision shift surfaced on the
// failing `ComputeFeatureNeighborMisorientationsFilter` ctest (test 1602 in the prior numbering).
// The UNIMPLEMENTED stub left `ComputeAvgMisors=true` with zero CI coverage, which is why the
// `tempMisoList` divisor bug at algorithm.cpp:75 (reassigning the divisor inside the inner j-loop)
// went undetected for so long. The Class 1 + Class 4 toy fixtures below replace both retirements.
// See `vv/provenance/ComputeFeatureNeighborMisorientationsFilter.md` for retirement details.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: SIMPL Backwards Compatibility",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureNeighborMisorientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureNeighborMisorientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureNeighborMisorientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeFeatureNeighborMisorientationsFilter::k_ComputeAvgMisors_Key) == true);
      CHECK(args.value<DataPath>(ComputeFeatureNeighborMisorientationsFilter::k_NeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborMisorientationsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborMisorientationsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborMisorientationsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeatureNeighborMisorientationsFilter::k_MisorientationListArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureNeighborMisorientationsFilter::k_AvgMisorientationsArrayName_Key) == "TestName");
    }
  }
}

// =============================================================================
// V&V Class 1 + Class 4 toy fixtures (added 2026-06-02 during V&V cycle).
// =============================================================================

// Fixture A: single-phase, single-feature with 2 neighbors. Verifies the per-neighbor
// MisorientationList values and the per-feature average computation (Mode: ComputeAvgMisors=true).
// Closed-form: pure phi1 rotations about z, cubic 4-fold doesn't reduce phi1 in [0, 45deg], so
// misorientation between (0deg) and (5deg) is 5.0deg; between (0deg) and (10deg) is 10.0deg.
// Expected avg = (5 + 10) / 2 = 7.5deg.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Single Phase Two Neighbors", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(/*numFeatures=*/4, /*numCrystalStructures=*/2);

  (*td.featurePhases)[1] = 1;
  (*td.featurePhases)[2] = 1;
  (*td.featurePhases)[3] = 1;
  DataFixtures::SetAvgQuat(td, 1, DataFixtures::QuatFromPhi1Deg(0.0f));
  DataFixtures::SetAvgQuat(td, 2, DataFixtures::QuatFromPhi1Deg(5.0f));
  DataFixtures::SetAvgQuat(td, 3, DataFixtures::QuatFromPhi1Deg(10.0f));
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = DataFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = DataFixtures::GetOutputAvgMisorientations(td.ds);
  const auto& feature1List = misoList.at(1);
  REQUIRE(feature1List.size() == 2);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(feature1List[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
}

// Fixture B: BUG-EXPOSING — mixed-phase neighbor list with phase-mismatch in the MIDDLE.
// Neighbors are processed in order [match, mismatch, match]. The bug at algorithm.cpp:75
// reassigns `tempMisoList = featureNeighborList.size()` every j-iteration; the per-mismatch
// decrement at line 90 is therefore clobbered by the NEXT j-iteration's reassignment, and the
// final divisor equals the full list size (3) instead of the number of phase-matched neighbors (2).
//   BUGGY  result: avg = (5 + 10) / 3 = 5.0deg  (FAILS this assertion)
//   FIXED  result: avg = (5 + 10) / 2 = 7.5deg  (PASSES this assertion)
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Mixed Phase Neighbors (exposes divisor bug)",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  // Crystal structures: index 0 sentinel (set by scaffold); index 1 Cubic_High (set by scaffold);
  // index 2 Hex_High (different Laue class -> filter treats as a phase mismatch).
  (*td.crystalStructures)[2] = 0u; // Hex_High

  // Feature 1 (phase 1, identity) - the focal feature with neighbors [2, 4, 3]
  (*td.featurePhases)[1] = 1;
  DataFixtures::SetAvgQuat(td, 1, DataFixtures::QuatFromPhi1Deg(0.0f));
  // Feature 2 (phase 1, 5deg) - phase MATCH -> misorientation = 5.0deg
  (*td.featurePhases)[2] = 1;
  DataFixtures::SetAvgQuat(td, 2, DataFixtures::QuatFromPhi1Deg(5.0f));
  // Feature 4 (phase 2, Hex_High) - phase MISMATCH -> NaN; should NOT count toward avg divisor
  (*td.featurePhases)[4] = 2;
  DataFixtures::SetAvgQuat(td, 4, DataFixtures::QuatFromPhi1Deg(99.0f)); // value irrelevant; quat will be skipped
  // Feature 3 (phase 1, 10deg) - phase MATCH -> misorientation = 10.0deg
  (*td.featurePhases)[3] = 1;
  DataFixtures::SetAvgQuat(td, 3, DataFixtures::QuatFromPhi1Deg(10.0f));

  // Neighbor order: [2 (match), 4 (mismatch), 3 (match)] -> LAST neighbor is a match -> bug fires.
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 4, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = DataFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = DataFixtures::GetOutputAvgMisorientations(td.ds);
  const auto& feature1List = misoList.at(1);
  REQUIRE(feature1List.size() == 3);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(std::isnan(feature1List[1]));
  REQUIRE(feature1List[2] == Approx(10.0f).margin(1e-3f));
  // The correct average is sum-of-non-NaN / count-of-non-NaN = (5 + 10) / 2 = 7.5.
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
}

// Fixture C: Same neighbor composition as Fixture B, but the phase-mismatch is the LAST neighbor.
// Bug doesn't fire in this ordering because the decrement at algorithm.cpp:90 is the last write
// to tempMisoList (no subsequent inner-loop iteration to clobber it). Both buggy and fixed code
// produce avg = (5 + 10) / 2 = 7.5deg.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Mismatch Last Order", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystalStructures)[2] = 0u; // Hex_High
  (*td.featurePhases)[1] = 1;
  DataFixtures::SetAvgQuat(td, 1, DataFixtures::QuatFromPhi1Deg(0.0f));
  (*td.featurePhases)[2] = 1;
  DataFixtures::SetAvgQuat(td, 2, DataFixtures::QuatFromPhi1Deg(5.0f));
  (*td.featurePhases)[3] = 1;
  DataFixtures::SetAvgQuat(td, 3, DataFixtures::QuatFromPhi1Deg(10.0f));
  (*td.featurePhases)[4] = 2;
  DataFixtures::SetAvgQuat(td, 4, DataFixtures::QuatFromPhi1Deg(99.0f));
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3, 4}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = DataFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = DataFixtures::GetOutputAvgMisorientations(td.ds);
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
  const auto& feature1List = misoList.at(1);
  REQUIRE(feature1List.size() == 3);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(feature1List[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(std::isnan(feature1List[2]));
}

// Fixture D: Class 4 invariants — runs the bug-exposing fixture and asserts only the invariants
// (no specific avg value), so this test catches a future regression that preserves specific values
// but breaks the invariants. Use a different neighbor order from Fixture B so we sample a different
// path through the per-feature loop.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 4 - Invariants", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  DataFixtures::ToyData td = DataFixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystalStructures)[2] = 0u; // Hex_High
  (*td.featurePhases)[1] = 1;
  DataFixtures::SetAvgQuat(td, 1, DataFixtures::QuatFromPhi1Deg(0.0f));
  (*td.featurePhases)[2] = 1;
  DataFixtures::SetAvgQuat(td, 2, DataFixtures::QuatFromPhi1Deg(7.5f));
  (*td.featurePhases)[3] = 1;
  DataFixtures::SetAvgQuat(td, 3, DataFixtures::QuatFromPhi1Deg(12.0f));
  (*td.featurePhases)[4] = 2;
  DataFixtures::SetAvgQuat(td, 4, DataFixtures::QuatFromPhi1Deg(99.0f));
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{4, 2, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = DataFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = DataFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = DataFixtures::GetOutputAvgMisorientations(td.ds);
  const auto& feature1List = misoList.at(1);
  REQUIRE(feature1List.size() == 3);

  // Invariant 1: Each list entry is either NaN (phase mismatch) or a non-negative misorientation
  //              bounded above by the cubic max symmetry-reduced misorientation (~62.8 deg).
  // Invariant 2: avg[fid] equals sum-of-non-NaN-entries / count-of-non-NaN-entries.
  float64 sum = 0.0;
  usize count = 0;
  for(const auto& entry : feature1List)
  {
    if(std::isnan(entry))
    {
      continue;
    }
    REQUIRE(entry >= 0.0f);
    REQUIRE(entry <= 62.8f);
    sum += static_cast<float64>(entry);
    count++;
  }
  REQUIRE(count > 0);
  const float32 expectedAvg = static_cast<float32>(sum / static_cast<float64>(count));
  REQUIRE(avg[1] == Approx(expectedAvg).margin(1e-4f));
}
