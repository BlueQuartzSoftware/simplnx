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
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

// =============================================================================
// V&V Class 1 analytical and Class 4 invariant oracle support.
//
// These fixtures replace archive regression with hand-derived inputs and expected neighbor misorientations and feature averages.
// A defect-sensitive configuration detects incorrect reassignment of the per-feature divisor inside the neighbor loop.
//
// Reference: src/Plugins/OrientationAnalysis/vv/ComputeFeatureNeighborMisorientationsFilter.md
// =============================================================================

namespace
{
namespace AnalyticalFixtures
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

std::array<float32, 4> QuatFromPhi1Deg(float32 phi1Deg)
{
  const float32 halfAngleRad = (phi1Deg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {0.0f, 0.0f, std::sin(halfAngleRad), std::cos(halfAngleRad)};
}

struct FixtureData
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
FixtureData CreateScaffold(usize numFeatures, usize numCrystalStructures)
{
  FixtureData td;
  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({1, 1, 1}); // unused at the cell level; only present so the geom container has shape

  td.featureAM = AttributeMatrix::Create(td.ds, "FeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "EnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(td.ds, k_FeatureDataPath.createChildPath(k_FeaturePhasesName), {numFeatures}, {1});
  td.featurePhases = Int32Array::Create(td.ds, k_FeaturePhasesName, featurePhasesStore, td.featureAM->getId());
  auto avgQuatsStore = DataStoreUtilities::CreateDataStore<float32>(td.ds, k_FeatureDataPath.createChildPath(k_AvgQuatsName), {numFeatures}, {4});
  td.avgQuats = Float32Array::Create(td.ds, k_AvgQuatsName, avgQuatsStore, td.featureAM->getId());
  td.neighborList = NeighborList<int32>::Create(td.ds, k_NeighborListName, ShapeType{numFeatures}, td.featureAM->getId());
  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(td.ds, k_EnsembleDataPath.createChildPath(k_CrystalStructuresName), {numCrystalStructures}, {1});
  td.crystalStructures = UInt32Array::Create(td.ds, k_CrystalStructuresName, crystalStructuresStore, td.ensembleAM->getId());

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

void SetAvgQuat(FixtureData& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avgQuats)[featureIdx * 4 + 0] = q[0];
  (*td.avgQuats)[featureIdx * 4 + 1] = q[1];
  (*td.avgQuats)[featureIdx * 4 + 2] = q[2];
  (*td.avgQuats)[featureIdx * 4 + 3] = q[3];
}

Arguments BuildArgs(bool computeAvgMisors)
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

const NeighborList<float32>& GetOutputMisorientationList(const DataStructure& ds)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_MisorientationListOutName)));
  return ds.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_MisorientationListOutName));
}

const Float32Array& GetOutputAvgMisorientations(const DataStructure& ds)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgMisorientationsOutName)));
  return ds.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgMisorientationsOutName));
}
} // namespace AnalyticalFixtures
} // namespace

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Preflight Error - Feature array tuple count mismatch (-34501)",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Build a minimal synthetic DataStructure where the feature-level objects validated together
  // (AvgQuats, FeaturePhases, NeighborList) do NOT all share the same tuple count. This drives the
  // validateNumberOfTuples() guard in preflightImpl that emits error -34501.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* featureAM = AttributeMatrix::Create(dataStructure, "Feature Data", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "AvgQuats", {10}, {4}, featureAM->getId());
  NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{10}, featureAM->getId());

  // FeaturePhases lives in a separate AttributeMatrix with a deliberately different tuple count
  // (9 != 10) so the cross-array tuple-count check fails.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_ComputeAvgMisors_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "Feature Data", "NeighborList"})));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "Feature Data", "AvgQuats"})));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "Cell Ensemble Data", "CrystalStructures"})));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_MisorientationListArrayName_Key, std::make_any<std::string>("MisorientationList"));
  args.insertOrAssign(ComputeFeatureNeighborMisorientationsFilter::k_AvgMisorientationsArrayName_Key, std::make_any<std::string>("AvgMisorientations"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -34501);
}

// The retired archive exemplar was a circular oracle generated from earlier SIMPLNX output.
// Its unimplemented average-misorientation stub provided no coverage for the divisor defect.
// The Class 1 and Class 4 fixtures replace both retired tests.
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
// V&V Class 1 + Class 4 data fixtures (added 2026-06-02 during V&V cycle).
// =============================================================================

// Fixture A has one Phase, one Feature, and two neighbors.
// It verifies neighbor misorientations and the feature average when ComputeAvgMisors is true.
// Cubic four-fold symmetry does not reduce pure phi1 rotations in [0, 45] degrees.
// The two misorientations are 5 and 10 degrees.
// Expected avg = (5 + 10) / 2 = 7.5deg.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Single Phase Two Neighbors", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(/*numFeatures=*/4, /*numCrystalStructures=*/2);

  (*td.featurePhases)[1] = 1;
  (*td.featurePhases)[2] = 1;
  (*td.featurePhases)[3] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetAvgQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(5.0f));
  AnalyticalFixtures::SetAvgQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisorientations(td.ds);
  const auto& feature1List = misoList.at(1);
  REQUIRE(feature1List.size() == 2);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(feature1List[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
}

// Fixture B has a Phase mismatch between two matching neighbors.
// The neighbor order is [match, mismatch, match].
// Reassigning the divisor during each iteration loses the mismatch decrement.
// The incorrect divisor is three instead of the two Phase-matched neighbors.
//   BUGGY  result: avg = (5 + 10) / 3 = 5.0deg  (FAILS this assertion)
//   FIXED  result: avg = (5 + 10) / 2 = 7.5deg  (PASSES this assertion)
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Mixed Phase Neighbors (exposes divisor bug)",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  // Crystal structures: index 0 sentinel (set by scaffold); index 1 Cubic_High (set by scaffold);
  // index 2 Hex_High (different Laue class -> filter treats as a phase mismatch).
  (*td.crystalStructures)[2] = 0u; // Hex_High

  // Feature 1 (phase 1, identity) - the focal feature with neighbors [2, 4, 3]
  (*td.featurePhases)[1] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  // Feature 2 (phase 1, 5deg) - phase MATCH -> misorientation = 5.0deg
  (*td.featurePhases)[2] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(5.0f));
  // Feature 4 (phase 2, Hex_High) - phase MISMATCH -> NaN; should NOT count toward avg divisor
  (*td.featurePhases)[4] = 2;
  AnalyticalFixtures::SetAvgQuat(td, 4, AnalyticalFixtures::QuatFromPhi1Deg(99.0f)); // value irrelevant; quat will be skipped
  // Feature 3 (phase 1, 10deg) - phase MATCH -> misorientation = 10.0deg
  (*td.featurePhases)[3] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));

  // Neighbor order: [2 (match), 4 (mismatch), 3 (match)] -> LAST neighbor is a match -> bug fires.
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 4, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisorientations(td.ds);
  const auto& feature1List = misoList.at(1);
  REQUIRE(feature1List.size() == 3);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(std::isnan(feature1List[1]));
  REQUIRE(feature1List[2] == Approx(10.0f).margin(1e-3f));
  // The correct average is sum-of-non-NaN / count-of-non-NaN = (5 + 10) / 2 = 7.5.
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
}

// Fixture C uses the same neighbors as Fixture B, but the Phase mismatch is last.
// The final decrement cannot be overwritten by a later loop iteration.
// Both implementations produce an average of 7.5 degrees.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Mismatch Last Order", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystalStructures)[2] = 0u; // Hex_High
  (*td.featurePhases)[1] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  (*td.featurePhases)[2] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(5.0f));
  (*td.featurePhases)[3] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
  (*td.featurePhases)[4] = 2;
  AnalyticalFixtures::SetAvgQuat(td, 4, AnalyticalFixtures::QuatFromPhi1Deg(99.0f));
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3, 4}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisorientations(td.ds);
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
  const auto& feature1List = misoList.at(1);
  REQUIRE(feature1List.size() == 3);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(feature1List[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(std::isnan(feature1List[2]));
}

// Fixture D applies Class 4 invariants to the defect-sensitive arrangement without exact average values.
// It uses a different neighbor order to exercise another per-feature loop path.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 4 - Invariants", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystalStructures)[2] = 0u; // Hex_High
  (*td.featurePhases)[1] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  (*td.featurePhases)[2] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(7.5f));
  (*td.featurePhases)[3] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(12.0f));
  (*td.featurePhases)[4] = 2;
  AnalyticalFixtures::SetAvgQuat(td, 4, AnalyticalFixtures::QuatFromPhi1Deg(99.0f));
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{4, 2, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisorientations(td.ds);
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

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Phase Index Bounds", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  const int32 invalidPhaseIdx = GENERATE(-1, 2);
  CAPTURE(invalidPhaseIdx);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  AnalyticalFixtures::FixtureData fixture = AnalyticalFixtures::CreateScaffold(/*numFeatures=*/3, /*numCrystalStructures=*/2);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(fixture.featurePhases->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  (*fixture.featurePhases)[1] = 1;
  (*fixture.featurePhases)[2] = 1;
  fixture.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*computeAvgMisors=*/true);

  SECTION("Current Feature Phase returns an error")
  {
    (*fixture.featurePhases)[1] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -34502);
  }

  SECTION("Neighbor Feature Phase returns an error")
  {
    (*fixture.featurePhases)[2] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -34502);
  }

  SECTION("Feature zero is ignored")
  {
    (*fixture.featurePhases)[0] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(fixture.ds);
}
