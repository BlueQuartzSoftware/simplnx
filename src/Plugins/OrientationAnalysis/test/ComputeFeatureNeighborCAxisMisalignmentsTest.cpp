#include "OrientationAnalysis/Filters/ComputeFeatureNeighborCAxisMisalignmentsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;
namespace
{
namespace analytical_fixtures
{
const std::string k_GeomName = "ImageGeometry";
const DataPath k_ImageGeomPath = DataPath({k_GeomName});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath("CellData");
const DataPath k_FeatureDataPath = k_ImageGeomPath.createChildPath("CellFeatureData");
const DataPath k_EnsembleDataPath = k_ImageGeomPath.createChildPath("CellEnsembleData");

const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_CellPhasesName = "Phases";
const std::string k_FeaturePhasesName = "FeaturePhases";
const std::string k_AvgQuatsName = "AvgQuats";
const std::string k_NeighborListName = "NeighborList";
const std::string k_CrystalStructuresName = "CrystalStructures";

const std::string k_CAxisMisalignmentListOutName = "CAxisMisalignmentList";
const std::string k_AvgCAxisMisalignmentsOutName = "AvgCAxisMisalignments";

// A pure Bunge ZXZ Euler rotation (0, phiDeg, 0) rotates about the x-axis.
// The rotation tilts the crystal c-axis by phiDeg from the global z-axis.
// Two pure-Phi tilts have c-axis misalignment |phiA - phiB|, folded into [0, 90] degrees.
// The V&V provenance document contains the closed-form derivation.
std::array<float32, 4> QuatFromPhiDeg(float32 phiDeg)
{
  const float32 halfAngleRad = (phiDeg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {std::sin(halfAngleRad), 0.0f, 0.0f, std::cos(halfAngleRad)};
}

struct FixtureData
{
  DataStructure ds;
  ImageGeom* geom = nullptr;
  AttributeMatrix* cell_am = nullptr;
  AttributeMatrix* feature_am = nullptr;
  AttributeMatrix* ensemble_am = nullptr;
  Int32Array* feature_ids = nullptr;
  Int32Array* cell_phases = nullptr;
  Int32Array* feature_phases = nullptr;
  Float32Array* avg_quats = nullptr;
  NeighborList<int32>* neighbor_list = nullptr;
  UInt32Array* crystal_structures = nullptr;
  usize total_cells = 0;
  usize total_features = 0;
};

// Build an ImageGeom-backed scaffold. Cell-level arrays (FeatureIds, Phases) are sized
// {nZ, nY, nX}; feature-level arrays (FeaturePhases, AvgQuats, NeighborList) are sized
// {numFeatures}; ensemble-level arrays (CrystalStructures) are sized {numCrystalStructures}.
// Defaults: every cell assigned to feature 1 / phase 1; every feature phase 0 (caller to set);
// identity quats; empty neighbor lists; CrystalStructures[0] = sentinel 999.
FixtureData CreateScaffold(usize nX, usize nY, usize nZ, usize numFeatures, usize numCrystalStructures)
{
  FixtureData td;
  td.total_cells = nX * nY * nZ;
  td.total_features = numFeatures;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  REQUIRE(td.geom->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(td.geom->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  td.geom->setDimensions({nX, nY, nZ});

  td.cell_am = AttributeMatrix::Create(td.ds, "CellData", ShapeType{nZ, nY, nX}, td.geom->getId());
  td.feature_am = AttributeMatrix::Create(td.ds, "CellFeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensemble_am = AttributeMatrix::Create(td.ds, "CellEnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  const ShapeType cellTupleShape = {nZ, nY, nX};
  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(td.ds, k_CellDataPath.createChildPath(k_FeatureIdsName), cellTupleShape, {1});
  td.feature_ids = Int32Array::Create(td.ds, k_FeatureIdsName, featureIdsStore, td.cell_am->getId());
  auto cellPhasesStore = DataStoreUtilities::CreateDataStore<int32>(td.ds, k_CellDataPath.createChildPath(k_CellPhasesName), cellTupleShape, {1});
  td.cell_phases = Int32Array::Create(td.ds, k_CellPhasesName, cellPhasesStore, td.cell_am->getId());
  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(td.ds, k_FeatureDataPath.createChildPath(k_FeaturePhasesName), {numFeatures}, {1});
  td.feature_phases = Int32Array::Create(td.ds, k_FeaturePhasesName, featurePhasesStore, td.feature_am->getId());
  auto avgQuatsStore = DataStoreUtilities::CreateDataStore<float32>(td.ds, k_FeatureDataPath.createChildPath(k_AvgQuatsName), {numFeatures}, {4});
  td.avg_quats = Float32Array::Create(td.ds, k_AvgQuatsName, avgQuatsStore, td.feature_am->getId());
  td.neighbor_list = NeighborList<int32>::Create(td.ds, k_NeighborListName, ShapeType{numFeatures}, td.feature_am->getId());
  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(td.ds, k_EnsembleDataPath.createChildPath(k_CrystalStructuresName), {numCrystalStructures}, {1});
  td.crystal_structures = UInt32Array::Create(td.ds, k_CrystalStructuresName, crystalStructuresStore, td.ensemble_am->getId());

  for(usize i = 0; i < td.total_cells; ++i)
  {
    (*td.feature_ids)[i] = 1;
    (*td.cell_phases)[i] = 1;
  }
  for(usize i = 0; i < numFeatures; ++i)
  {
    (*td.feature_phases)[i] = 0;
    (*td.avg_quats)[i * 4 + 0] = 0.0f;
    (*td.avg_quats)[i * 4 + 1] = 0.0f;
    (*td.avg_quats)[i * 4 + 2] = 0.0f;
    (*td.avg_quats)[i * 4 + 3] = 1.0f;
    td.neighbor_list->setList(i, std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  }
  (*td.crystal_structures)[0] = 999u;
  return td;
}

void SetAvgQuat(FixtureData& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avg_quats)[featureIdx * 4 + 0] = q[0];
  (*td.avg_quats)[featureIdx * 4 + 1] = q[1];
  (*td.avg_quats)[featureIdx * 4 + 2] = q[2];
  (*td.avg_quats)[featureIdx * 4 + 3] = q[3];
}

void PopulateMixedHexNeighbors(FixtureData& fixture)
{
  REQUIRE(fixture.total_features == 5);
  REQUIRE(fixture.feature_phases != nullptr);
  REQUIRE(fixture.avg_quats != nullptr);
  REQUIRE(fixture.neighbor_list != nullptr);
  REQUIRE(fixture.crystal_structures != nullptr);

  (*fixture.crystal_structures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*fixture.crystal_structures)[2] = static_cast<uint32>(ebsdlib::CrystalStructure::Cubic_High);
  (*fixture.feature_phases)[0] = 0;
  (*fixture.feature_phases)[1] = 1;
  (*fixture.feature_phases)[2] = 1;
  (*fixture.feature_phases)[3] = 1;
  (*fixture.feature_phases)[4] = 2;
  SetAvgQuat(fixture, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(fixture, 2, QuatFromPhiDeg(5.0f));
  SetAvgQuat(fixture, 3, QuatFromPhiDeg(10.0f));
  SetAvgQuat(fixture, 4, QuatFromPhiDeg(20.0f));

  // The cubic neighbor separates two valid hexagonal neighbors in the output list.
  fixture.neighbor_list->setList(1, std::vector<int32>{2, 4, 3});
}

Arguments BuildArgs(bool findAvgMisals)
{
  Arguments args;
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FindAvgMisals_Key, std::make_any<bool>(findAvgMisals));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(k_FeatureDataPath.createChildPath(k_NeighborListName)));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_FeatureDataPath.createChildPath(k_AvgQuatsName)));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeatureDataPath.createChildPath(k_FeaturePhasesName)));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_EnsembleDataPath.createChildPath(k_CrystalStructuresName)));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CAxisMisalignmentListArrayName_Key, std::make_any<std::string>(k_CAxisMisalignmentListOutName));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgCAxisMisalignmentsArrayName_Key, std::make_any<std::string>(k_AvgCAxisMisalignmentsOutName));
  return args;
}

const NeighborList<float32>& GetOutputMisalignmentList(const DataStructure& ds)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_CAxisMisalignmentListOutName)));
  return ds.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_CAxisMisalignmentListOutName));
}

const Float32Array& GetOutputAvgMisalignments(const DataStructure& ds)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgCAxisMisalignmentsOutName)));
  return ds.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgCAxisMisalignmentsOutName));
}

// Helper to construct the 10x10x1 realistic-microstructure scaffold used by Fixtures B and D.
// See `vv/provenance/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` for the cell-by-feature
// layout diagram and the per-feature hand-derived expected values.
FixtureData BuildRealisticMicrostructure()
{
  // 6 real features (1-6) + 1 sentinel (0). 2 phases: 1 = Hexagonal_High, 2 = Cubic_High.
  FixtureData td = CreateScaffold(/*nX=*/10, /*nY=*/10, /*nZ=*/1, /*numFeatures=*/7, /*numCrystalStructures=*/3);

  // CrystalStructures: [0]=sentinel 999, [1]=Hex_High (0), [2]=Cubic_High (1) — EbsdLib LaueOps indices.
  (*td.crystal_structures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.crystal_structures)[2] = static_cast<uint32>(ebsdlib::CrystalStructure::Cubic_High);

  // Feature phases: F1, F2, F4, F5, F6 hex; F3 cubic (non-hex — exposes the per-mismatch decrement).
  (*td.feature_phases)[1] = 1;
  (*td.feature_phases)[2] = 1;
  (*td.feature_phases)[3] = 2;
  (*td.feature_phases)[4] = 1;
  (*td.feature_phases)[5] = 1;
  (*td.feature_phases)[6] = 1;

  // Feature average quaternions — pure Phi rotations about x by [0, 5, 10, 15, 20, 25] degrees.
  SetAvgQuat(td, 1, QuatFromPhiDeg(0.0f));
  SetAvgQuat(td, 2, QuatFromPhiDeg(5.0f));
  SetAvgQuat(td, 3, QuatFromPhiDeg(10.0f)); // ignored — F3 is non-hex
  SetAvgQuat(td, 4, QuatFromPhiDeg(15.0f));
  SetAvgQuat(td, 5, QuatFromPhiDeg(20.0f));
  SetAvgQuat(td, 6, QuatFromPhiDeg(25.0f));

  // Cell-by-cell FeatureIds layout (rows=y, cols=x):
  //   y=0..3: x=0..2->F1, x=3..6->F2, x=7..9->F3
  //   y=4..9: x=0..3->F4, x=4..7->F5, x=8..9->F6
  // This produces face-adjacencies: F1-F2, F1-F4, F2-F3, F2-F4 (corner), F2-F5, F3-F5 (1 cell),
  // F3-F6, F4-F5, F5-F6. Drawn in vv/provenance/<filter>.md.
  for(usize y = 0; y < 10; ++y)
  {
    for(usize x = 0; x < 10; ++x)
    {
      int32 fid = 0;
      if(y < 4)
      {
        if(x < 3)
        {
          fid = 1;
        }
        else if(x < 7)
        {
          fid = 2;
        }
        else
        {
          fid = 3;
        }
      }
      else
      {
        if(x < 4)
        {
          fid = 4;
        }
        else if(x < 8)
        {
          fid = 5;
        }
        else
        {
          fid = 6;
        }
      }
      const usize idx = y * 10 + x;
      (*td.feature_ids)[idx] = fid;
      (*td.cell_phases)[idx] = (fid == 3) ? 2 : 1;
    }
  }

  // Per-feature neighbor lists (face-adjacencies derived from the layout above).
  td.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 4}));
  td.neighbor_list->setList(2, std::make_shared<std::vector<int32>>(std::vector<int32>{1, 3, 4, 5}));
  td.neighbor_list->setList(3, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 5, 6}));
  td.neighbor_list->setList(4, std::make_shared<std::vector<int32>>(std::vector<int32>{1, 2, 5}));
  td.neighbor_list->setList(5, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3, 4, 6}));
  td.neighbor_list->setList(6, std::make_shared<std::vector<int32>>(std::vector<int32>{3, 5}));

  return td;
}
} // namespace AnalyticalFixtures
} // namespace

// The retired exemplar comparison consumed `compute_feature_neighbor_caxis_misalignments.tar.gz`.
// Its hex-only data did not exercise mismatch decrements and could not detect the divisor defect.
// Four hand-derived fixtures cover six algorithm paths and three defect-sensitive feature configurations.
// See `vv/provenance/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` for retirement details.

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: SIMPL Backwards Compatibility",
          "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureNeighborCAxisMisalignmentsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureNeighborCAxisMisalignmentsFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeFeatureNeighborCAxisMisalignmentsFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<bool>(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FindAvgMisals_Key) == true);
      CHECK(args.value<DataPath>(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_NeighborListArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CAxisMisalignmentListArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgCAxisMisalignmentsArrayName_Key) == "TestName");
    }
  }
}

// =====================================================================================
// Class 1 (Analytical) data fixtures + Class 4 (Invariant) companion.
//
// Class 1 fixtures use pure Bunge ZXZ rotations (0, Phi, 0) about the x-axis.
// Two rotations have c-axis misalignment |phiA - phiB|, folded into [0, 90] degrees.
// The V&V provenance document contains the closed-form derivation.
// =====================================================================================

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 1 - Simple Hex Pair", "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();

  // 3 features total: 0=sentinel, 1=Hex Phi=0deg, 2=Hex Phi=10deg.
  // Neighbor lists: F1 <-> F2 (single hex-hex pair, no mismatches).
  // Expected misalignmentList: F1=[10deg], F2=[10deg]. Expected avg: F1=10, F2=10.
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*nX=*/1, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/3, /*numCrystalStructures=*/2);
  (*td.crystal_structures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.feature_phases)[1] = 1;
  (*td.feature_phases)[2] = 1;
  analytical_fixtures::SetAvgQuat(td, 1, analytical_fixtures::QuatFromPhiDeg(0.0f));
  analytical_fixtures::SetAvgQuat(td, 2, analytical_fixtures::QuatFromPhiDeg(10.0f));
  td.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2}));
  td.neighbor_list->setList(2, std::make_shared<std::vector<int32>>(std::vector<int32>{1}));

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisalignments(td.ds);
  REQUIRE(misoList.at(1).size() == 1);
  REQUIRE(misoList.at(1)[0] == Approx(10.0f).margin(1e-3f));
  REQUIRE(misoList.at(2).size() == 1);
  REQUIRE(misoList.at(2)[0] == Approx(10.0f).margin(1e-3f));
  REQUIRE(avg[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(avg[2] == Approx(10.0f).margin(1e-3f));
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 1 - Realistic Microstructure (exposes divisor bug)",
          "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();

  // 10x10x1 image, 6 features arranged in 2 rows of 3 grains each. F3 is Cubic (non-hex); the rest
  // are Hex with pure-Phi tilts [0, 5, _, 15, 20, 25] degrees. Neighbor lists are derived from the
  // cell-by-cell layout (see AnalyticalFixtures::BuildRealisticMicrostructure for the diagram).
  //
  // Expected per-feature misalignmentList + avg:
  //   F1 ([F2, F4]):                  [5, 15]                     divisor=2 sum=20 avg=10.000
  //   F2 ([F1, F3, F4, F5]):          [5, NaN, 10, 15]            divisor=3 sum=30 avg=10.000   <- bug-exposing
  //   F3 ([F2, F5, F6]):              [NaN, NaN, NaN]             divisor=0       avg=NaN
  //   F4 ([F1, F2, F5]):              [15, 10, 5]                 divisor=3 sum=30 avg=10.000
  //   F5 ([F2, F3, F4, F6]):          [15, NaN, 5, 5]             divisor=3 sum=25 avg=8.3333   <- bug-exposing
  //   F6 ([F3, F5]):                  [NaN, 5]                    divisor=1 sum=5  avg=5.000    <- bug-exposing
  //
  // The bug-exposing features (F2, F5, F6) have a non-hex neighbor followed by at least one hex
  // neighbor — the pre-fix code reassigned `hexNeighborListSize = currentNeighborList.size()` on
  // every j-iteration, clobbering the per-mismatch decrement. Under the bug, F2 avg would be
  // 30/4=7.5, F5 avg would be 25/4=6.25, F6 avg would be 5/2=2.5. Post-fix produces the correct
  // hex-only divisor.
  analytical_fixtures::FixtureData td = analytical_fixtures::BuildRealisticMicrostructure();

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisalignments(td.ds);

  // F1 ([F2, F4])
  REQUIRE(misoList.at(1).size() == 2);
  REQUIRE(misoList.at(1)[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(misoList.at(1)[1] == Approx(15.0f).margin(1e-3f));
  REQUIRE(avg[1] == Approx(10.0f).margin(1e-3f));

  // F2 ([F1, F3, F4, F5]) — bug-exposing
  REQUIRE(misoList.at(2).size() == 4);
  REQUIRE(misoList.at(2)[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(std::isnan(misoList.at(2)[1]));
  REQUIRE(misoList.at(2)[2] == Approx(10.0f).margin(1e-3f));
  REQUIRE(misoList.at(2)[3] == Approx(15.0f).margin(1e-3f));
  REQUIRE(avg[2] == Approx(10.0f).margin(1e-3f));

  // F3 ([F2, F5, F6]) — non-hex focal, all entries NaN, avg is NaN
  REQUIRE(misoList.at(3).size() == 3);
  REQUIRE(std::isnan(misoList.at(3)[0]));
  REQUIRE(std::isnan(misoList.at(3)[1]));
  REQUIRE(std::isnan(misoList.at(3)[2]));
  REQUIRE(std::isnan(avg[3]));

  // F4 ([F1, F2, F5])
  REQUIRE(misoList.at(4).size() == 3);
  REQUIRE(misoList.at(4)[0] == Approx(15.0f).margin(1e-3f));
  REQUIRE(misoList.at(4)[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(misoList.at(4)[2] == Approx(5.0f).margin(1e-3f));
  REQUIRE(avg[4] == Approx(10.0f).margin(1e-3f));

  // F5 ([F2, F3, F4, F6]) — bug-exposing
  REQUIRE(misoList.at(5).size() == 4);
  REQUIRE(misoList.at(5)[0] == Approx(15.0f).margin(1e-3f));
  REQUIRE(std::isnan(misoList.at(5)[1]));
  REQUIRE(misoList.at(5)[2] == Approx(5.0f).margin(1e-3f));
  REQUIRE(misoList.at(5)[3] == Approx(5.0f).margin(1e-3f));
  REQUIRE(avg[5] == Approx(25.0f / 3.0f).margin(1e-3f));

  // F6 ([F3, F5]) — bug-exposing (mismatch-first ordering)
  REQUIRE(misoList.at(6).size() == 2);
  REQUIRE(std::isnan(misoList.at(6)[0]));
  REQUIRE(misoList.at(6)[1] == Approx(5.0f).margin(1e-3f));
  REQUIRE(avg[6] == Approx(5.0f).margin(1e-3f));

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 1 - Mismatch Last Order", "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();

  // 5 features: 0=sentinel, 1=Hex 0deg, 2=Hex 5deg, 3=Hex 10deg, 4=Cubic (non-hex).
  // F1's NeighborList = [F2, F3, F4] — order [match, match, mismatch].
  // Expected misalignmentList[F1] = [5, 10, NaN], divisor = 2, sum = 15, avg = 7.500.
  // Pre-fix code produces the SAME 7.500 result on this ordering — this is the "control" fixture
  // showing that the fix does not regress the case where the bug happened to give the right answer.
  analytical_fixtures::FixtureData td = analytical_fixtures::CreateScaffold(/*nX=*/1, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystal_structures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.crystal_structures)[2] = static_cast<uint32>(ebsdlib::CrystalStructure::Cubic_High);
  (*td.feature_phases)[1] = 1;
  (*td.feature_phases)[2] = 1;
  (*td.feature_phases)[3] = 1;
  (*td.feature_phases)[4] = 2; // Cubic
  analytical_fixtures::SetAvgQuat(td, 1, analytical_fixtures::QuatFromPhiDeg(0.0f));
  analytical_fixtures::SetAvgQuat(td, 2, analytical_fixtures::QuatFromPhiDeg(5.0f));
  analytical_fixtures::SetAvgQuat(td, 3, analytical_fixtures::QuatFromPhiDeg(10.0f));
  analytical_fixtures::SetAvgQuat(td, 4, analytical_fixtures::QuatFromPhiDeg(20.0f)); // ignored — F4 is non-hex
  td.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3, 4}));

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisalignments(td.ds);
  REQUIRE(misoList.at(1).size() == 3);
  REQUIRE(misoList.at(1)[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(misoList.at(1)[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(std::isnan(misoList.at(1)[2]));
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Averages Disabled - Resident Lists", "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, 1);

  analytical_fixtures::FixtureData fixture = analytical_fixtures::CreateScaffold(/*nX=*/1, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/5, /*numCrystalStructures=*/3);
  analytical_fixtures::PopulateMixedHexNeighbors(fixture);

  REQUIRE(fixture.feature_phases->getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(fixture.avg_quats->getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(fixture.crystal_structures->getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(fixture.neighbor_list->getIListStore() != nullptr);
  REQUIRE_FALSE(fixture.neighbor_list->getIListStore()->isOutOfCore());
  const auto inputLists = fixture.neighbor_list->getVectors();
  REQUIRE(inputLists.size() == 5);

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  const Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/false);
  auto preflightResult = filter.preflight(fixture.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(fixture.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(std::any_of(executeResult.result.warnings().begin(), executeResult.result.warnings().end(), [](const Warning& warning) { return warning.code == -1563; }));

  const auto& misalignmentList = analytical_fixtures::GetOutputMisalignmentList(fixture.ds);
  REQUIRE(misalignmentList.getIListStore() != nullptr);
  REQUIRE_FALSE(misalignmentList.getIListStore()->isOutOfCore());
  REQUIRE(misalignmentList.getNumberOfLists() == 5);
  const std::array<usize, 5> expectedListLengths = {0, 3, 0, 0, 0};
  for(usize featureIdx = 0; featureIdx < expectedListLengths.size(); ++featureIdx)
  {
    CAPTURE(featureIdx);
    REQUIRE(misalignmentList.at(featureIdx).size() == expectedListLengths[featureIdx]);
  }
  const auto focalMisalignments = misalignmentList.at(1);
  REQUIRE(focalMisalignments[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(std::isnan(focalMisalignments[1]));
  REQUIRE(focalMisalignments[2] == Approx(10.0f).margin(1e-3f));
  REQUIRE(fixture.ds.getData(analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_AvgCAxisMisalignmentsOutName)) == nullptr);
  REQUIRE(fixture.neighbor_list->getVectors() == inputLists);

  UnitTest::CheckArraysInheritTupleDims(fixture.ds);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Averages Disabled - HDF5 Lists",
          "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  analytical_fixtures::FixtureData fixture = analytical_fixtures::CreateScaffold(/*nX=*/1, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/5, /*numCrystalStructures=*/3);
  REQUIRE(fixture.neighbor_list != nullptr);
  const DataPath neighborListPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_NeighborListName);
  auto neighborListStore = DataStoreUtilities::CreateListStore<int32>(fixture.ds, neighborListPath, {5}, IDataAction::Mode::Execute, "HDF5-OOC");
  REQUIRE(neighborListStore != nullptr);
  // The scaffold creates a resident list store, so replace the store before list population.
  fixture.neighbor_list->setStore(neighborListStore);
  analytical_fixtures::PopulateMixedHexNeighbors(fixture);

  REQUIRE(fixture.feature_phases->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(fixture.avg_quats->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(fixture.crystal_structures->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(fixture.neighbor_list->getIListStore() != nullptr);
  REQUIRE(fixture.neighbor_list->getIListStore()->isOutOfCore());
  const auto inputLists = fixture.neighbor_list->getVectors();
  REQUIRE(inputLists.size() == 5);

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  const Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/false);
  auto preflightResult = filter.preflight(fixture.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(fixture.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(std::any_of(executeResult.result.warnings().begin(), executeResult.result.warnings().end(), [](const Warning& warning) { return warning.code == -1563; }));

  const auto& misalignmentList = analytical_fixtures::GetOutputMisalignmentList(fixture.ds);
  REQUIRE(misalignmentList.getIListStore() != nullptr);
  REQUIRE(misalignmentList.getIListStore()->isOutOfCore());
  REQUIRE(misalignmentList.getNumberOfLists() == 5);
  const std::array<usize, 5> expectedListLengths = {0, 3, 0, 0, 0};
  for(usize featureIdx = 0; featureIdx < expectedListLengths.size(); ++featureIdx)
  {
    CAPTURE(featureIdx);
    REQUIRE(misalignmentList.at(featureIdx).size() == expectedListLengths[featureIdx]);
  }
  const auto focalMisalignments = misalignmentList.at(1);
  REQUIRE(focalMisalignments[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(std::isnan(focalMisalignments[1]));
  REQUIRE(focalMisalignments[2] == Approx(10.0f).margin(1e-3f));
  REQUIRE(fixture.ds.getData(analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_AvgCAxisMisalignmentsOutName)) == nullptr);
  REQUIRE(fixture.neighbor_list->getVectors() == inputLists);

  UnitTest::CheckArraysInheritTupleDims(fixture.ds);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 4 - Invariants", "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();

  // Class 4 invariants asserted on the realistic 10x10x1 microstructure. These invariants are
  // oracle-agnostic — they hold for any input regardless of the specific quaternion values, so
  // they catch regressions even when expected per-feature values change.
  //   (i)   Range: every misalignmentList entry is either NaN or in [0, 90] degrees.
  //   (ii)  Per-feature averaging formula: avg[F] == (sum of non-NaN entries in misoList[F])
  //         / (count of non-NaN entries in misoList[F]), or NaN if count==0.
  //   (iii) Non-hex focal feature: every entry in misalignmentList[F] is NaN, and avg[F] is NaN.
  constexpr float32 k_CAxisUpperBoundDeg = 90.0f;

  analytical_fixtures::FixtureData td = analytical_fixtures::BuildRealisticMicrostructure();
  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = analytical_fixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = analytical_fixtures::GetOutputAvgMisalignments(td.ds);

  SECTION("(i) Range: 0 <= entry <= 90 degrees, or NaN")
  {
    for(usize f = 1; f < td.total_features; ++f)
    {
      const auto& list = misoList.at(static_cast<int32>(f));
      for(const auto& entry : list)
      {
        if(!std::isnan(entry))
        {
          REQUIRE(entry >= 0.0f);
          REQUIRE(entry <= k_CAxisUpperBoundDeg);
        }
      }
    }
  }

  SECTION("(ii) Per-feature averaging formula")
  {
    for(usize f = 1; f < td.total_features; ++f)
    {
      const auto& list = misoList.at(static_cast<int32>(f));
      float32 sum = 0.0f;
      int32 count = 0;
      for(const auto& entry : list)
      {
        if(!std::isnan(entry))
        {
          sum += entry;
          ++count;
        }
      }
      if(count == 0)
      {
        REQUIRE(std::isnan(avg[f]));
      }
      else
      {
        REQUIRE(avg[f] == Approx(sum / static_cast<float32>(count)).margin(1e-3f));
      }
    }
  }

  SECTION("(iii) Non-hex focal => all NaN")
  {
    // F3 has Cubic (non-hex) phase. Every entry in its misalignmentList must be NaN, avg NaN.
    const auto& list = misoList.at(3);
    for(const auto& entry : list)
    {
      REQUIRE(std::isnan(entry));
    }
    REQUIRE(std::isnan(avg[3]));
  }
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Preflight Error - Feature array tuple count mismatch (-1560)",
          "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // The filter requires equal tuple counts for NeighborList, AvgQuats, and FeaturePhases.
  // FeaturePhases uses a separate AttributeMatrix with seven tuples instead of eight.
  // This mismatch makes preflight return error -1560.
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeomPtr->setDimensions({8, 1, 1});

  // NeighborList + AvgQuats are 8 tuples...
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "CellFeatureData", ShapeType{8}, imageGeomPtr->getId());
  NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{8}, featureAmPtr->getId());
  CreateTestDataArray<float32>(dataStructure, "AvgQuats", {8}, {4}, featureAmPtr->getId());

  // ...but FeaturePhases is 7 tuples in a separate AttributeMatrix (the mismatch).
  auto* mismatchAmPtr = AttributeMatrix::Create(dataStructure, "MismatchData", ShapeType{7}, imageGeomPtr->getId());
  CreateTestDataArray<int32>(dataStructure, "FeaturePhases", {7}, {1}, mismatchAmPtr->getId());

  auto* ensembleAmPtr = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, imageGeomPtr->getId());
  CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAmPtr->getId());

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FindAvgMisals_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellFeatureData", "NeighborList"})));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellFeatureData", "AvgQuats"})));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "MismatchData", "FeaturePhases"})));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CAxisMisalignmentListArrayName_Key, std::make_any<std::string>("CAxisMisalignmentList"));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgCAxisMisalignmentsArrayName_Key, std::make_any<std::string>("AvgCAxisMisalignments"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -1560);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: genuine HDF5 1MiB-chunk tail oracle",
          "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);

  // Each four-component float32 quaternion tuple is 16 bytes. The 1 MiB HDF5
  // chunk policy therefore places 65,536 tuples in one physical chunk.
  constexpr usize k_ChunkTuples = 65536;
  constexpr usize k_FeatureTuples = k_ChunkTuples + 1;
  constexpr usize k_LastFullChunkTuple = k_ChunkTuples - 1;
  constexpr usize k_TailTuple = k_ChunkTuples;

  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

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
  const DataPath misalignmentListPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_CAxisMisalignmentListOutName);
  const DataPath avgMisalignmentsPath = analytical_fixtures::k_FeatureDataPath.createChildPath(analytical_fixtures::k_AvgCAxisMisalignmentsOutName);

  auto featurePhasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, featurePhasesPath, {k_FeatureTuples}, {1});
  auto avgQuatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, avgQuatsPath, {k_FeatureTuples}, {4});
  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, crystalStructuresPath, {2}, {1});
  auto* featurePhasesPtr = Int32Array::Create(dataStructure, featurePhasesPath.getTargetName(), featurePhasesStore, featureDataPtr->getId());
  auto* avgQuatsPtr = Float32Array::Create(dataStructure, avgQuatsPath.getTargetName(), avgQuatsStore, featureDataPtr->getId());
  auto* neighborListPtr = NeighborList<int32>::Create(dataStructure, neighborListPath.getTargetName(), {k_FeatureTuples}, featureDataPtr->getId());
  auto* crystalStructuresPtr = UInt32Array::Create(dataStructure, crystalStructuresPath.getTargetName(), crystalStructuresStore, ensembleDataPtr->getId());
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
  const auto lastFullChunkQuat = analytical_fixtures::QuatFromPhiDeg(15.0F);
  const auto tailQuat = analytical_fixtures::QuatFromPhiDeg(30.0F);
  std::copy(lastFullChunkQuat.cbegin(), lastFullChunkQuat.cend(), avgQuatValues.begin() + k_LastFullChunkTuple * 4);
  std::copy(tailQuat.cbegin(), tailQuat.cend(), avgQuatValues.begin() + k_TailTuple * 4);
  const std::array<uint32, 2> crystalStructureValues = {ebsdlib::CrystalStructure::UnknownCrystalStructure, ebsdlib::CrystalStructure::Hexagonal_High};

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

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(executeResult.result.warnings().empty());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<float32>>(misalignmentListPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(avgMisalignmentsPath));
  const auto& misalignmentList = dataStructure.getDataRefAs<NeighborList<float32>>(misalignmentListPath);
  const auto& avgMisalignments = dataStructure.getDataRefAs<Float32Array>(avgMisalignmentsPath);
  REQUIRE(avgMisalignments.getDataStoreRef().getDataFormat() == "HDF5-OOC");

  const auto lastFullChunkValues = misalignmentList.at(k_LastFullChunkTuple);
  REQUIRE(lastFullChunkValues.size() == 1);
  REQUIRE(lastFullChunkValues[0] == Approx(15.0F).margin(1.0E-3F));
  REQUIRE(avgMisalignments[k_LastFullChunkTuple] == Approx(15.0F).margin(1.0E-3F));

  const auto tailValues = misalignmentList.at(k_TailTuple);
  REQUIRE(tailValues.size() == 1);
  REQUIRE(tailValues[0] == Approx(30.0F).margin(1.0E-3F));
  REQUIRE(avgMisalignments[k_TailTuple] == Approx(30.0F).margin(1.0E-3F));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Phase Index Bounds", "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();

  const int32 invalidPhaseIdx = GENERATE(-1, 2);
  CAPTURE(invalidPhaseIdx);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);

  analytical_fixtures::FixtureData fixture = analytical_fixtures::CreateScaffold(/*nX=*/1, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/3, /*numCrystalStructures=*/2);
  if(Application::Instance()->getIOManager("HDF5-OOC") != nullptr)
  {
    REQUIRE(fixture.feature_phases->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  (*fixture.crystal_structures)[1] = ebsdlib::CrystalStructure::Hexagonal_High;
  (*fixture.feature_phases)[1] = 1;
  (*fixture.feature_phases)[2] = 1;
  fixture.neighbor_list->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2}));

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = analytical_fixtures::BuildArgs(/*findAvgMisals=*/true);

  SECTION("Current Feature Phase returns an error")
  {
    (*fixture.feature_phases)[1] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -1564);
  }

  SECTION("Neighbor Feature Phase returns an error")
  {
    (*fixture.feature_phases)[2] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -1564);
  }

  SECTION("Feature zero is ignored")
  {
    (*fixture.feature_phases)[0] = invalidPhaseIdx;
    auto executeResult = filter.execute(fixture.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(fixture.ds);
}
