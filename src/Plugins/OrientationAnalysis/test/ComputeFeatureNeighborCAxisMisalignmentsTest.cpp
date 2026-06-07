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

#include <catch2/catch.hpp>

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
namespace AnalyticalFixtures
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

// Quaternion for a pure Bunge ZXZ Euler rotation (phi1=0, Phi=phiDeg, phi2=0). This is a pure
// rotation about the x-axis by phiDeg degrees, which tilts the crystal c-axis (originally along z)
// by phiDeg degrees from the global z-axis. For two cells with pure-Phi tilts of phiA and phiB
// degrees, the c-axis misalignment is exactly |phiA - phiB| degrees (folded into [0, 90]) — see
// the V&V provenance doc for the closed-form derivation.
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
  Int32Array* cellPhases = nullptr;
  Int32Array* featurePhases = nullptr;
  Float32Array* avgQuats = nullptr;
  NeighborList<int32>* neighborList = nullptr;
  UInt32Array* crystalStructures = nullptr;
  usize totalCells = 0;
  usize totalFeatures = 0;
};

// Build an ImageGeom-backed scaffold. Cell-level arrays (FeatureIds, Phases) are sized
// {nZ, nY, nX}; feature-level arrays (FeaturePhases, AvgQuats, NeighborList) are sized
// {numFeatures}; ensemble-level arrays (CrystalStructures) are sized {numCrystalStructures}.
// Defaults: every cell assigned to feature 1 / phase 1; every feature phase 0 (caller to set);
// identity quats; empty neighbor lists; CrystalStructures[0] = sentinel 999.
FixtureData CreateScaffold(usize nX, usize nY, usize nZ, usize numFeatures, usize numCrystalStructures)
{
  FixtureData td;
  td.totalCells = nX * nY * nZ;
  td.totalFeatures = numFeatures;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({nX, nY, nZ});

  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", ShapeType{nZ, nY, nX}, td.geom->getId());
  td.featureAM = AttributeMatrix::Create(td.ds, "CellFeatureData", ShapeType{numFeatures}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "CellEnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  td.featureIds = CreateTestDataArray<int32>(td.ds, k_FeatureIdsName, {nZ, nY, nX}, {1}, td.cellAM->getId());
  td.cellPhases = CreateTestDataArray<int32>(td.ds, k_CellPhasesName, {nZ, nY, nX}, {1}, td.cellAM->getId());
  td.featurePhases = CreateTestDataArray<int32>(td.ds, k_FeaturePhasesName, {numFeatures}, {1}, td.featureAM->getId());
  td.avgQuats = CreateTestDataArray<float32>(td.ds, k_AvgQuatsName, {numFeatures}, {4}, td.featureAM->getId());
  td.neighborList = NeighborList<int32>::Create(td.ds, k_NeighborListName, ShapeType{numFeatures}, td.featureAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numCrystalStructures}, {1}, td.ensembleAM->getId());

  for(usize i = 0; i < td.totalCells; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
  }
  for(usize i = 0; i < numFeatures; ++i)
  {
    (*td.featurePhases)[i] = 0;
    (*td.avgQuats)[i * 4 + 0] = 0.0f;
    (*td.avgQuats)[i * 4 + 1] = 0.0f;
    (*td.avgQuats)[i * 4 + 2] = 0.0f;
    (*td.avgQuats)[i * 4 + 3] = 1.0f;
    td.neighborList->setList(i, std::make_shared<std::vector<int32>>(std::vector<int32>{}));
  }
  (*td.crystalStructures)[0] = 999u;
  return td;
}

void SetAvgQuat(FixtureData& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avgQuats)[featureIdx * 4 + 0] = q[0];
  (*td.avgQuats)[featureIdx * 4 + 1] = q[1];
  (*td.avgQuats)[featureIdx * 4 + 2] = q[2];
  (*td.avgQuats)[featureIdx * 4 + 3] = q[3];
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
  return ds.getDataRefAs<NeighborList<float32>>(k_FeatureDataPath.createChildPath(k_CAxisMisalignmentListOutName));
}

const Float32Array& GetOutputAvgMisalignments(const DataStructure& ds)
{
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
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.crystalStructures)[2] = static_cast<uint32>(ebsdlib::CrystalStructure::Cubic_High);

  // Feature phases: F1, F2, F4, F5, F6 hex; F3 cubic (non-hex — exposes the per-mismatch decrement).
  (*td.featurePhases)[1] = 1;
  (*td.featurePhases)[2] = 1;
  (*td.featurePhases)[3] = 2;
  (*td.featurePhases)[4] = 1;
  (*td.featurePhases)[5] = 1;
  (*td.featurePhases)[6] = 1;

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
      int32 fid;
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
      (*td.featureIds)[idx] = fid;
      (*td.cellPhases)[idx] = (fid == 3) ? 2 : 1;
    }
  }

  // Per-feature neighbor lists (face-adjacencies derived from the layout above).
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 4}));
  td.neighborList->setList(2, std::make_shared<std::vector<int32>>(std::vector<int32>{1, 3, 4, 5}));
  td.neighborList->setList(3, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 5, 6}));
  td.neighborList->setList(4, std::make_shared<std::vector<int32>>(std::vector<int32>{1, 2, 5}));
  td.neighborList->setList(5, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3, 4, 6}));
  td.neighborList->setList(6, std::make_shared<std::vector<int32>>(std::vector<int32>{3, 5}));

  return td;
}
} // namespace AnalyticalFixtures
} // namespace

// Retired 2026-06-04 (V&V cycle): the main exemplar-comparison TEST_CASE that consumed
// `compute_feature_neighbor_caxis_misalignments.tar.gz` was removed. The exemplar arrays (suffixed
// `(7_5)`) were generated from a pre-fix SIMPL 6.5.171 pipeline run on a HEX-ONLY dataset — the
// divisor bug at algorithm.cpp:111 (`hexNeighborListSize` reassigned inside the inner j-loop) is
// therefore not exercised by the exemplar (no mismatch decrements ever fire), and the exemplar
// would have happily passed even on the buggy code. The 4 hand-derived data fixtures below cover
// the 6 algorithmic paths and include 3 bug-exposing per-feature configurations.
// See `vv/provenance/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` for retirement details.

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: SIMPL Backwards Compatibility",
          "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureNeighborCAxisMisalignmentsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureNeighborCAxisMisalignmentsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureNeighborCAxisMisalignmentsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
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
// All Class 1 fixtures use pure Bunge ZXZ Euler rotations (0, Phi, 0) about the x-axis, which tilt
// the crystal c-axis (originally along z) by Phi degrees from the global z-axis. For two cells with
// pure-Phi tilts of phiA and phiB degrees, the c-axis misalignment is exactly |phiA - phiB|
// degrees (folded into [0, 90]). This makes the oracle closed-form — see the V&V provenance doc
// for the closed-form derivation.
// =====================================================================================

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 1 - Simple Hex Pair", "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();

  // 3 features total: 0=sentinel, 1=Hex Phi=0deg, 2=Hex Phi=10deg.
  // Neighbor lists: F1 <-> F2 (single hex-hex pair, no mismatches).
  // Expected misalignmentList: F1=[10deg], F2=[10deg]. Expected avg: F1=10, F2=10.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(/*nX=*/1, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/3, /*numCrystalStructures=*/2);
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.featurePhases)[1] = 1;
  (*td.featurePhases)[2] = 1;
  AnalyticalFixtures::SetAvgQuat(td, 1, AnalyticalFixtures::QuatFromPhiDeg(0.0f));
  AnalyticalFixtures::SetAvgQuat(td, 2, AnalyticalFixtures::QuatFromPhiDeg(10.0f));
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2}));
  td.neighborList->setList(2, std::make_shared<std::vector<int32>>(std::vector<int32>{1}));

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisalignments(td.ds);
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
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::BuildRealisticMicrostructure();

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisalignments(td.ds);

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
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(/*nX=*/1, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/5, /*numCrystalStructures=*/3);
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.crystalStructures)[2] = static_cast<uint32>(ebsdlib::CrystalStructure::Cubic_High);
  (*td.featurePhases)[1] = 1;
  (*td.featurePhases)[2] = 1;
  (*td.featurePhases)[3] = 1;
  (*td.featurePhases)[4] = 2; // Cubic
  AnalyticalFixtures::SetAvgQuat(td, 1, AnalyticalFixtures::QuatFromPhiDeg(0.0f));
  AnalyticalFixtures::SetAvgQuat(td, 2, AnalyticalFixtures::QuatFromPhiDeg(5.0f));
  AnalyticalFixtures::SetAvgQuat(td, 3, AnalyticalFixtures::QuatFromPhiDeg(10.0f));
  AnalyticalFixtures::SetAvgQuat(td, 4, AnalyticalFixtures::QuatFromPhiDeg(20.0f)); // ignored — F4 is non-hex
  td.neighborList->setList(1, std::make_shared<std::vector<int32>>(std::vector<int32>{2, 3, 4}));

  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisalignments(td.ds);
  REQUIRE(misoList.at(1).size() == 3);
  REQUIRE(misoList.at(1)[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(misoList.at(1)[1] == Approx(10.0f).margin(1e-3f));
  REQUIRE(std::isnan(misoList.at(1)[2]));
  REQUIRE(avg[1] == Approx(7.5f).margin(1e-3f));
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

  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::BuildRealisticMicrostructure();
  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs(/*findAvgMisals=*/true);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& misoList = AnalyticalFixtures::GetOutputMisalignmentList(td.ds);
  const auto& avg = AnalyticalFixtures::GetOutputAvgMisalignments(td.ds);

  SECTION("(i) Range: 0 <= entry <= 90 degrees, or NaN")
  {
    for(usize f = 1; f < td.totalFeatures; ++f)
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
    for(usize f = 1; f < td.totalFeatures; ++f)
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
