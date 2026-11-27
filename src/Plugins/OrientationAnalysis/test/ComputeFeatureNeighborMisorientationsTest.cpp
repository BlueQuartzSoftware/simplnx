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

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::constants;
using namespace nx::core::unit_test;

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
namespace analytical_fixtures
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
  AttributeMatrix* feature_am = nullptr;
  AttributeMatrix* ensemble_am = nullptr;
  Int32ArrayType* feature_phases = nullptr;
  Float32ArrayType* avg_quats = nullptr;
  NeighborList<int32>* neighbor_list = nullptr;
  UInt32ArrayType* crystal_structures = nullptr;
};

// Build a scaffold with a tiny ImageGeom + a feature AM (size numFeatures) + ensemble AM
// (size numCrystalStructures). All input arrays are initialized; caller populates per-feature values.
FixtureData CreateScaffold(usize numFeatures, usize numCrystalStructures)
{
  FixtureData td;
  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  REQUIRE(td.geom->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(td.geom->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  td.geom->setDimensions({1, 1, 1}); // unused at the cell level; only present so the geom container has shape

  td.feature_am = AttributeMatrix::Create(td.ds, "FeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensemble_am = AttributeMatrix::Create(td.ds, "EnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  auto featurePhasesStore = data_store_utilities::CreateDataStore<int32>(td.ds, k_FeatureDataPath.createChildPath(k_FeaturePhasesName), {numFeatures}, {1});
  td.feature_phases = Int32ArrayType::Create(td.ds, k_FeaturePhasesName, featurePhasesStore, td.feature_am->getId());
  auto avgQuatsStore = data_store_utilities::CreateDataStore<float32>(td.ds, k_FeatureDataPath.createChildPath(k_AvgQuatsName), {numFeatures}, {4});
  td.avg_quats = Float32ArrayType::Create(td.ds, k_AvgQuatsName, avgQuatsStore, td.feature_am->getId());
  td.neighbor_list = NeighborList<int32>::Create(td.ds, k_NeighborListName, ShapeType{numFeatures}, td.feature_am->getId());
  auto crystalStructuresStore = data_store_utilities::CreateDataStore<uint32>(td.ds, k_EnsembleDataPath.createChildPath(k_CrystalStructuresName), {numCrystalStructures}, {1});
  td.crystal_structures = UInt32ArrayType::Create(td.ds, k_CrystalStructuresName, crystalStructuresStore, td.ensemble_am->getId());

  // Default: feature 0 sentinel; all other features phase=0 (unassigned); identity quats.
  for(usize i = 0; i < numFeatures; ++i)
  {
    (*td.feature_phases)[i] = 0;
    (*td.avg_quats)[i * 4 + 0] = 0.0f;
    (*td.avg_quats)[i * 4 + 1] = 0.0f;
    (*td.avg_quats)[i * 4 + 2] = 0.0f;
    (*td.avg_quats)[i * 4 + 3] = 1.0f;
    td.neighbor_list->setList(i, std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  }
  // Default crystal structures: index 0 sentinel; index 1 Cubic_High; subsequent left as zeros to be set by caller.
  (*td.crystal_structures)[0] = 999u;
  if(numCrystalStructures > 1)
  {
    (*td.crystal_structures)[1] = 1u; // Cubic_High (EbsdLib LaueOps index)
  }
  return td;
}

void SetAvgQuat(FixtureData& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avg_quats)[featureIdx * 4 + 0] = q[0];
  (*td.avg_quats)[featureIdx * 4 + 1] = q[1];
  (*td.avg_quats)[featureIdx * 4 + 2] = q[2];
  (*td.avg_quats)[featureIdx * 4 + 3] = q[3];
}

void PopulateAveragesDisabledFixture(FixtureData& td)
{
  REQUIRE(td.feature_phases != nullptr);
  REQUIRE(td.avg_quats != nullptr);
  REQUIRE(td.neighbor_list != nullptr);
  REQUIRE(td.crystal_structures != nullptr);
  REQUIRE(td.feature_phases->getDataStore() != nullptr);
  REQUIRE(td.avg_quats->getDataStore() != nullptr);
  REQUIRE(td.neighbor_list->getIListStore() != nullptr);
  REQUIRE(td.crystal_structures->getDataStore() != nullptr);

  const std::array<int32, 5> featurePhases = {0, 1, 1, 1, 2};
  const std::array<float32, 5> phi1Degrees = {0.0f, 0.0f, 5.0f, 10.0f, 99.0f};
  for(usize featureIdx = 0; featureIdx < featurePhases.size(); featureIdx++)
  {
    (*td.feature_phases)[featureIdx] = featurePhases[featureIdx];
    SetAvgQuat(td, featureIdx, QuatFromPhi1Deg(phi1Degrees[featureIdx]));
    td.neighbor_list->setList(featureIdx, std::vector<int32>{});
  }
  (*td.crystal_structures)[1] = ebsdlib::CrystalStructure::Cubic_High;
  (*td.crystal_structures)[2] = ebsdlib::CrystalStructure::Hexagonal_High;
  td.neighbor_list->setList(1, std::vector<int32>{2, 4, 3});
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

const NeighborList<float32>& GetOutputMisorientationList(const DataStructure& dataStructure)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_MisorientationListOutName)));
  return dataStructure.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_MisorientationListOutName));
}

const Float32ArrayType& GetOutputAvgMisorientations(const DataStructure& dataStructure)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(k_FeatureDataPath.createChildPath(k_AvgMisorientationsOutName)));
  return dataStructure.getDataRefAs<Float32ArrayType>(k_FeatureDataPath.createChildPath(k_AvgMisorientationsOutName));
}
} // namespace analytical_fixtures
} // namespace

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Preflight Error - Feature array tuple count mismatch (-34501)",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter][preflight]")
{
  unit_test::LoadPlugins();

  // Build a minimal synthetic DataStructure where the feature-level objects validated together
  // (AvgQuats, FeaturePhases, NeighborList) do NOT all share the same tuple count. This drives the
  // validateNumberOfTuples() guard in preflightImpl that emits error -34501.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeomPtr->setDimensions({10, 1, 1});

  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Feature Data", {10}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<float32>(dataStructure, "AvgQuats", {10}, {4}, featureAmPtr->getId());
  NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{10}, featureAmPtr->getId());

  // FeaturePhases lives in a separate AttributeMatrix with a deliberately different tuple count
  // (9 != 10) so the cross-array tuple-count check fails.
  auto* mismatchAmPtr = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAmPtr->getId());

  auto* ensembleAmPtr = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", {2}, imageGeomPtr->getId());
  unit_test::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAmPtr->getId());

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
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors()[0].code == -34501);
}

// The retired archive exemplar was a circular oracle generated from earlier SIMPLNX output.
// Its unimplemented average-misorientation stub provided no coverage for the divisor defect.
// The Class 1 and Class 4 fixtures replace both retired tests.
// See `vv/provenance/ComputeFeatureNeighborMisorientationsFilter.md` for retirement details.
TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: SIMPL Backwards Compatibility",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureNeighborMisorientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureNeighborMisorientationsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeFeatureNeighborMisorientationsFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
  unit_test::LoadPlugins();
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*numFeatures=*/4, /*numCrystalStructures=*/2);

  (*td.feature_phases)[1] = 1;
  (*td.feature_phases)[2] = 1;
  (*td.feature_phases)[3] = 1;
  analytical_fixtures::SetAvgQuat(td, 1, analytical_fixtures::QuatFromPhi1Deg(0.0f));
  analytical_fixtures::SetAvgQuat(td, 2, analytical_fixtures::QuatFromPhi1Deg(5.0f));
  analytical_fixtures::SetAvgQuat(td, 3, analytical_fixtures::QuatFromPhi1Deg(10.0f));
  td.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisorientations(td.ds);
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
  unit_test::LoadPlugins();
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  // Crystal structures: index 0 sentinel (set by scaffold); index 1 Cubic_High (set by scaffold);
  // index 2 Hex_High (different Laue class -> filter treats as a phase mismatch).
  (*td.crystal_structures)[2] = 0u; // Hex_High

  // Feature 1 (phase 1, identity) - the focal feature with neighbors [2, 4, 3]
  (*td.feature_phases)[1] = 1;
  analytical_fixtures::SetAvgQuat(td, 1, analytical_fixtures::QuatFromPhi1Deg(0.0f));
  // Feature 2 (phase 1, 5deg) - phase MATCH -> misorientation = 5.0deg
  (*td.feature_phases)[2] = 1;
  analytical_fixtures::SetAvgQuat(td, 2, analytical_fixtures::QuatFromPhi1Deg(5.0f));
  // Feature 4 (phase 2, Hex_High) - phase MISMATCH -> NaN; should NOT count toward avg divisor
  (*td.feature_phases)[4] = 2;
  analytical_fixtures::SetAvgQuat(td, 4, analytical_fixtures::QuatFromPhi1Deg(99.0f)); // value irrelevant; quat will be skipped
  // Feature 3 (phase 1, 10deg) - phase MATCH -> misorientation = 10.0deg
  (*td.feature_phases)[3] = 1;
  analytical_fixtures::SetAvgQuat(td, 3, analytical_fixtures::QuatFromPhi1Deg(10.0f));

  // Neighbor order: [2 (match), 4 (mismatch), 3 (match)] -> LAST neighbor is a match -> bug fires.
  td.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 4, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisorientations(td.ds);
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
  unit_test::LoadPlugins();
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystal_structures)[2] = 0u; // Hex_High
  (*td.feature_phases)[1] = 1;
  analytical_fixtures::SetAvgQuat(td, 1, analytical_fixtures::QuatFromPhi1Deg(0.0f));
  (*td.feature_phases)[2] = 1;
  analytical_fixtures::SetAvgQuat(td, 2, analytical_fixtures::QuatFromPhi1Deg(5.0f));
  (*td.feature_phases)[3] = 1;
  analytical_fixtures::SetAvgQuat(td, 3, analytical_fixtures::QuatFromPhi1Deg(10.0f));
  (*td.feature_phases)[4] = 2;
  analytical_fixtures::SetAvgQuat(td, 4, analytical_fixtures::QuatFromPhi1Deg(99.0f));
  td.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3, 4}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisorientations(td.ds);
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
  unit_test::LoadPlugins();
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystal_structures)[2] = 0u; // Hex_High
  (*td.feature_phases)[1] = 1;
  analytical_fixtures::SetAvgQuat(td, 1, analytical_fixtures::QuatFromPhi1Deg(0.0f));
  (*td.feature_phases)[2] = 1;
  analytical_fixtures::SetAvgQuat(td, 2, analytical_fixtures::QuatFromPhi1Deg(7.5f));
  (*td.feature_phases)[3] = 1;
  analytical_fixtures::SetAvgQuat(td, 3, analytical_fixtures::QuatFromPhi1Deg(12.0f));
  (*td.feature_phases)[4] = 2;
  analytical_fixtures::SetAvgQuat(td, 4, analytical_fixtures::QuatFromPhi1Deg(99.0f));
  td.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{4, 2, 3}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisorientationList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisorientations(td.ds);
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

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Averages Disabled - Resident Lists", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);
  analytical_fixtures::PopulateAveragesDisabledFixture(td);

  REQUIRE(td.feature_phases->getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(td.avg_quats->getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(td.crystal_structures->getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE_FALSE(td.neighbor_list->getIListStore()->isOutOfCore());
  const std::array<std::vector<int32>, 5> inputNeighborLists = {td.neighbor_list->at(0), td.neighbor_list->at(1), td.neighbor_list->at(2), td.neighbor_list->at(3), td.neighbor_list->at(4)};

  ComputeFeatureNeighborMisorientationsFilter filter;
  const Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/false);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisorientationList(td.ds);
  REQUIRE(misoList.getIListStore() != nullptr);
  REQUIRE_FALSE(misoList.getIListStore()->isOutOfCore());
  REQUIRE(misoList.getNumberOfTuples() == 5);
  const std::array<usize, 5> expectedListSizes = {0, 3, 0, 0, 0};
  for(usize featureIdx = 0; featureIdx < expectedListSizes.size(); featureIdx++)
  {
    CAPTURE(featureIdx);
    REQUIRE(misoList.at(featureIdx).size() == expectedListSizes[featureIdx]);
    REQUIRE(td.neighbor_list->at(featureIdx) == inputNeighborLists[featureIdx]);
  }
  const auto feature1List = misoList.at(1);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(std::isnan(feature1List[1]));
  REQUIRE(feature1List[2] == Approx(10.0f).margin(1e-3f));
  REQUIRE(td.ds.getData(analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_AvgMisorientationsOutName)) == nullptr);

  unit_test::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Averages Disabled - HDF5 Lists", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter][.OocStoreContract]")
{
  unit_test::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*numFeatures=*/5, /*numCrystalStructures=*/3);

  // The scaffold creates a resident list. Replace its store before filling the fixture.
  const DataPath neighborListPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_NeighborListName);
  const auto neighborListStore = data_store_utilities::CreateListStore<int32>(td.ds, neighborListPath, {5}, IDataAction::Mode::Execute, "HDF5-OOC");
  REQUIRE(neighborListStore != nullptr);
  REQUIRE(td.neighbor_list != nullptr);
  td.neighbor_list->setStore(neighborListStore);
  analytical_fixtures::PopulateAveragesDisabledFixture(td);

  REQUIRE(td.feature_phases->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(td.avg_quats->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(td.crystal_structures->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(td.neighbor_list->getIListStore()->isOutOfCore());
  const std::array<std::vector<int32>, 5> inputNeighborLists = {td.neighbor_list->at(0), td.neighbor_list->at(1), td.neighbor_list->at(2), td.neighbor_list->at(3), td.neighbor_list->at(4)};

  ComputeFeatureNeighborMisorientationsFilter filter;
  const Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/false);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisorientationList(td.ds);
  REQUIRE(misoList.getIListStore() != nullptr);
  REQUIRE(misoList.getIListStore()->isOutOfCore());
  REQUIRE(misoList.getNumberOfTuples() == 5);
  const std::array<usize, 5> expectedListSizes = {0, 3, 0, 0, 0};
  for(usize featureIdx = 0; featureIdx < expectedListSizes.size(); featureIdx++)
  {
    CAPTURE(featureIdx);
    REQUIRE(misoList.at(featureIdx).size() == expectedListSizes[featureIdx]);
    REQUIRE(td.neighbor_list->at(featureIdx) == inputNeighborLists[featureIdx]);
  }
  const auto feature1List = misoList.at(1);
  REQUIRE(feature1List[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(std::isnan(feature1List[1]));
  REQUIRE(feature1List[2] == Approx(10.0f).margin(1e-3f));
  REQUIRE(td.ds.getData(analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_AvgMisorientationsOutName)) == nullptr);

  unit_test::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: genuine HDF5 1MiB-chunk tail oracle",
          "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter][.OocStoreContract]")
{
  unit_test::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);

  // Each four-component float32 quaternion tuple is 16 bytes. The 1 MiB HDF5
  // chunk policy therefore places 65,536 tuples in one physical chunk.
  constexpr usize k_ChunkTuples = 65536;
  constexpr usize k_FeatureTuples = k_ChunkTuples + 1;
  constexpr usize k_LastFullChunkTuple = k_ChunkTuples - 1;
  constexpr usize k_TailTuple = k_ChunkTuples;

  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  DataStructure dataStructure;
  auto* dataGroupPtr = DataGroup::Create(dataStructure, analytical_fixtures::k_GeomName);
  REQUIRE(dataGroupPtr != nullptr);
  auto* featureDataPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_FeatureDataPath.getTargetName(), {k_FeatureTuples}, dataGroupPtr->getId());
  auto* ensembleDataPtr = AttributeMatrix::Create(dataStructure, analytical_fixtures::k_EnsembleDataPath.getTargetName(), {2}, dataGroupPtr->getId());
  REQUIRE(featureDataPtr != nullptr);
  REQUIRE(ensembleDataPtr != nullptr);

  const DataPath featurePhasesPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_FeaturePhasesName);
  const DataPath avgQuatsPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_AvgQuatsName);
  const DataPath neighborListPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_NeighborListName);
  const DataPath crystalStructuresPath = analytical_fixtures::k_EnsembleDataPath.createChildPath(analytical_fixtures::k_CrystalStructuresName);
  const DataPath misalignmentListPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_MisorientationListOutName);
  const DataPath avgMisalignmentsPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_AvgMisorientationsOutName);

  auto featurePhasesStore = data_store_utilities::CreateDataStore<int32>(dataStructure, featurePhasesPath, {k_FeatureTuples}, {1});
  auto avgQuatsStore = data_store_utilities::CreateDataStore<float32>(dataStructure, avgQuatsPath, {k_FeatureTuples}, {4});
  auto crystalStructuresStore = data_store_utilities::CreateDataStore<uint32>(dataStructure, crystalStructuresPath, {2}, {1});
  auto* featurePhasesPtr = Int32ArrayType::Create(dataStructure, featurePhasesPath.getTargetName(), featurePhasesStore, featureDataPtr->getId());
  auto* avgQuatsPtr = Float32ArrayType::Create(dataStructure, avgQuatsPath.getTargetName(), avgQuatsStore, featureDataPtr->getId());
  auto* neighborListPtr = NeighborList<int32>::Create(dataStructure, neighborListPath.getTargetName(), {k_FeatureTuples}, featureDataPtr->getId());
  auto* crystalStructuresPtr = UInt32ArrayType::Create(dataStructure, crystalStructuresPath.getTargetName(), crystalStructuresStore, ensembleDataPtr->getId());
  REQUIRE(featurePhasesPtr != nullptr);
  REQUIRE(avgQuatsPtr != nullptr);
  REQUIRE(neighborListPtr != nullptr);
  REQUIRE(crystalStructuresPtr != nullptr);

  std::vector<int32> featurePhaseValues(k_FeatureTuples, 1);
  featurePhaseValues[0] = 0;
  std::vector<float32> avgQuatValues(k_FeatureTuples * 4, 0.0F);
  for(usize featureIdx = 0; featureIdx < k_FeatureTuples; featureIdx++)
  {
    avgQuatValues[featureIdx * 4 + 3] = 1.0F;
  }
  const auto lastFullChunkQuat = analytical_fixtures::QuatFromPhi1Deg(15.0F);
  const auto tailQuat = analytical_fixtures::QuatFromPhi1Deg(30.0F);
  std::copy(lastFullChunkQuat.cbegin(), lastFullChunkQuat.cend(), avgQuatValues.begin() + k_LastFullChunkTuple * 4);
  std::copy(tailQuat.cbegin(), tailQuat.cend(), avgQuatValues.begin() + k_TailTuple * 4);
  const std::array<uint32, 2> crystalStructureValues = {ebsdlib::CrystalStructure::UnknownCrystalStructure, ebsdlib::CrystalStructure::Cubic_High};

  auto featurePhasesWriteResult = featurePhasesPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(featurePhaseValues.data(), featurePhaseValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(featurePhasesWriteResult);
  auto avgQuatsWriteResult = avgQuatsPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(avgQuatValues.data(), avgQuatValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(avgQuatsWriteResult);
  auto crystalStructuresWriteResult = crystalStructuresPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint32>(crystalStructureValues.data(), crystalStructureValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(crystalStructuresWriteResult);

  neighborListPtr->setList(k_LastFullChunkTuple, std::vector<int32>{1});
  neighborListPtr->setList(k_TailTuple, std::vector<int32>{1});

  REQUIRE(featurePhasesPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(avgQuatsPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  const auto quaternionMetadata = avgQuatsPtr->getDataStoreRef().getRecoveryMetadata();
  REQUIRE(quaternionMetadata.at("OocChunkShape") == "65536");
  REQUIRE(crystalStructuresPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/true);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(executeResult.result.warnings().empty());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<float32>>(misalignmentListPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(avgMisalignmentsPath));
  const auto& misalignmentList = dataStructure.getDataRefAs<NeighborList<float32>>(misalignmentListPath);
  const auto& avgMisalignments = dataStructure.getDataRefAs<Float32ArrayType>(avgMisalignmentsPath);
  REQUIRE(avgMisalignments.getDataStoreRef().getDataFormat() == "HDF5-OOC");

  const auto lastFullChunkValues = misalignmentList.at(k_LastFullChunkTuple);
  REQUIRE(lastFullChunkValues.size() == 1);
  REQUIRE(lastFullChunkValues[0] == Approx(15.0F).margin(1.0E-3F));
  REQUIRE(avgMisalignments[k_LastFullChunkTuple] == Approx(15.0F).margin(1.0E-3F));

  const auto tailValues = misalignmentList.at(k_TailTuple);
  REQUIRE(tailValues.size() == 1);
  REQUIRE(tailValues[0] == Approx(30.0F).margin(1.0E-3F));
  REQUIRE(avgMisalignments[k_TailTuple] == Approx(30.0F).margin(1.0E-3F));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Phase Index Bounds", "[OrientationAnalysis][ComputeFeatureNeighborMisorientationsFilter]")
{
  unit_test::LoadPlugins();

  const int32 invalidPhaseIdx = GENERATE(-1, 2);
  CAPTURE(invalidPhaseIdx);
  const unit_test::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  analytical_fixtures::FixtureData fixture = analytical_fixtures::CreateScaffold(/*numFeatures=*/3, /*numCrystalStructures=*/2);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(fixture.feature_phases->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  (*fixture.feature_phases)[1] = 1;
  (*fixture.feature_phases)[2] = 1;
  fixture.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2}));

  ComputeFeatureNeighborMisorientationsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*computeAvgMisors=*/true);

  SECTION("Current Feature Phase returns an error")
  {
    (*fixture.feature_phases)[1] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -34502);
  }

  SECTION("Neighbor Feature Phase returns an error")
  {
    (*fixture.feature_phases)[2] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -34502);
  }

  SECTION("Feature zero is ignored")
  {
    (*fixture.feature_phases)[0] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  unit_test::CheckArraysInheritTupleDims(fixture.ds);
}
