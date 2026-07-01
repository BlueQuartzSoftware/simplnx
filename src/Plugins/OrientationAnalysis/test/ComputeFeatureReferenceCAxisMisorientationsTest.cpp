#include "OrientationAnalysis/Filters/ComputeFeatureReferenceCAxisMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
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

// Constants used by the legacy "InValid Filter Execution" + "SIMPL Backwards Compatibility" tests
// at the bottom of this file. The 4 V&V tests above use the AnalyticalFixtures namespace and need none of
// these — they construct everything in-memory.
namespace
{
const std::string k_FeatRefCAxisMisComputed = "NX_FeatureReferenceCAxisMisorientations";
const std::string k_FeatAvgCAxisMisComputed = "NX_FeatureAvgCAxisMisorientations";
const std::string k_FeatStDevCAxisMisComputed = "NX_FeatureStdevCAxisMisorientations";
const DataPath k_AvgCAxesPath = k_CellFeatureDataPath.createChildPath("AvgCAxes");
} // namespace

// =============================================================================
// V&V Class 1 (Analytical) + Class 4 (Invariant) oracle support — added 2026-06-10.
//
// Replaces the retired "Valid Filter Execution" TEST_CASE (which consumed
// `caxis_data.tar.gz` exemplar arrays — a circular oracle whose expected values were
// produced by a SIMPLNX run, not by independent analytical derivation). The 4 fixtures
// below cover all 7 algorithmic paths via closed-form hand-derivation and a Class 4
// invariants sweep, with the realistic-microstructure fixture exercising the
// load-bearing all-non-hex-feature → NaN path (paths 5 + 7 of the code-path table in
// vv/ComputeFeatureReferenceCAxisMisorientationsFilter.md).
//
// The `caxis_data.tar.gz` archive download is RETAINED in test/CMakeLists.txt because
// ComputeCAxisLocationsTest.cpp still consumes it; only this filter's exemplar consumer
// is retired. The remaining 2 TEST_CASEs in this file (`InValid Filter Execution` +
// `SIMPL Backwards Compatibility`) still load the archive as a starting DataStructure.
//
// See:
//   - vv/ComputeFeatureReferenceCAxisMisorientationsFilter.md   (V&V report)
//   - vv/deviations/ComputeFeatureReferenceCAxisMisorientationsFilter.md (deviations)
// =============================================================================
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
const std::string k_QuatsName = "Quats";
const std::string k_AvgCAxesName = "AvgCAxes";
const std::string k_CrystalStructuresName = "CrystalStructures";

const std::string k_FRCAxisMisOutName = "FeatureReferenceCAxisMisorientations";
const std::string k_FeatAvgCAxisMisOutName = "FeatureAvgCAxisMisorientations";
const std::string k_FeatStdevCAxisMisOutName = "FeatureStdevCAxisMisorientations";

// Quaternion for a pure Bunge ZXZ Euler rotation (phi1=0, Phi=phiDeg, phi2=0). This is a pure
// rotation about the x-axis by phiDeg degrees; the crystal c-axis (originally along z) tilts to
// `[0, sin(phiDeg), cos(phiDeg)]` in sample frame. For a cell at phi_cell and a feature avg at
// phi_avg, the per-cell c-axis misorientation reduces to `|phi_cell - phi_avg|` folded to [0, 90].
std::array<float32, 4> QuatFromPhiDeg(float32 phiDeg)
{
  const float32 halfAngleRad = (phiDeg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {std::sin(halfAngleRad), 0.0f, 0.0f, std::cos(halfAngleRad)};
}

// The pre-computed c-axis vector for a feature whose average orientation is a pure-Phi rotation
// by phiDeg about x. This is exactly what the filter would expect as input from upstream
// ComputeAvgCAxes / FindAvgCAxes.
std::array<float32, 3> CAxisFromPhiDeg(float32 phiDeg)
{
  const float32 phiRad = phiDeg * 3.14159265358979323846f / 180.0f;
  return {0.0f, std::sin(phiRad), std::cos(phiRad)};
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
  Float32Array* quats = nullptr;
  Float32Array* avgCAxes = nullptr;
  UInt32Array* crystalStructures = nullptr;
  usize totalCells = 0;
  usize totalFeatures = 0;
};

// Build an ImageGeom-backed scaffold. Cell-level arrays (FeatureIds, Phases, Quats) are sized
// {nZ, nY, nX}; feature-level array (AvgCAxes — 3 components) is sized {numFeatures}; ensemble-
// level array (CrystalStructures) is sized {numCrystalStructures}. Defaults: every cell assigned
// to feature 1 / phase 1 / identity quat; every feature avgCAxes = (0,0,1) (sample-z); ensemble
// sentinel at index 0 = 999, all others left for the caller to set.
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
  td.quats = CreateTestDataArray<float32>(td.ds, k_QuatsName, {nZ, nY, nX}, {4}, td.cellAM->getId());
  td.avgCAxes = CreateTestDataArray<float32>(td.ds, k_AvgCAxesName, {numFeatures}, {3}, td.featureAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numCrystalStructures}, {1}, td.ensembleAM->getId());

  for(usize i = 0; i < td.totalCells; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
    (*td.quats)[i * 4 + 0] = 0.0f;
    (*td.quats)[i * 4 + 1] = 0.0f;
    (*td.quats)[i * 4 + 2] = 0.0f;
    (*td.quats)[i * 4 + 3] = 1.0f;
  }
  for(usize i = 0; i < numFeatures; ++i)
  {
    (*td.avgCAxes)[i * 3 + 0] = 0.0f;
    (*td.avgCAxes)[i * 3 + 1] = 0.0f;
    (*td.avgCAxes)[i * 3 + 2] = 1.0f;
  }
  (*td.crystalStructures)[0] = 999u;
  return td;
}

void SetCellQuat(FixtureData& td, usize cellIdx, const std::array<float32, 4>& q)
{
  (*td.quats)[cellIdx * 4 + 0] = q[0];
  (*td.quats)[cellIdx * 4 + 1] = q[1];
  (*td.quats)[cellIdx * 4 + 2] = q[2];
  (*td.quats)[cellIdx * 4 + 3] = q[3];
}

void SetAvgCAxis(FixtureData& td, usize featureIdx, const std::array<float32, 3>& cAxis)
{
  (*td.avgCAxes)[featureIdx * 3 + 0] = cAxis[0];
  (*td.avgCAxes)[featureIdx * 3 + 1] = cAxis[1];
  (*td.avgCAxes)[featureIdx * 3 + 2] = cAxis[2];
}

Arguments BuildArgs()
{
  Arguments args;
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_FeatureIdsName)));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_CellPhasesName)));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_QuatsName)));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_AvgCAxesArrayPath_Key, std::make_any<DataPath>(k_FeatureDataPath.createChildPath(k_AvgCAxesName)));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_EnsembleDataPath.createChildPath(k_CrystalStructuresName)));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureReferenceCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FRCAxisMisOutName));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureAvgCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatAvgCAxisMisOutName));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureStdevCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatStdevCAxisMisOutName));
  return args;
}

const Float32Array& GetOutputCellMisos(const DataStructure& ds)
{
  return ds.getDataRefAs<Float32Array>(k_CellDataPath.createChildPath(k_FRCAxisMisOutName));
}

const Float32Array& GetOutputFeatureAvg(const DataStructure& ds)
{
  return ds.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_FeatAvgCAxisMisOutName));
}

const Float32Array& GetOutputFeatureStdev(const DataStructure& ds)
{
  return ds.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_FeatStdevCAxisMisOutName));
}

// 5x5x1 realistic-microstructure scaffold used by Fixture 2 and the Class 4 invariants test.
// Layout (rows = y, cols = x):
//   y=0: F1 (5 cells), all Phi=0, hex                  -> miso = 0 for all
//   y=1: F2 (5 cells), Phi = 8,9,10,11,12, hex         -> miso = 2,1,0,1,2
//   y=2: F3 (5 cells), all cubic                       -> 0 hex cells, exposes lat. div-by-zero
//   y=3: F4 (5 cells), all Phi=20, hex                 -> miso = 0 for all
//   y=4: F5 (5 cells), Phi = 25,28,30,32,35, hex       -> miso = 5,2,0,2,5
// Feature avg c-axes are set to the closed-form value at each feature's "center" Phi:
//   F1 = CAxis(0deg), F2 = CAxis(10deg), F3 = ignored, F4 = CAxis(20deg), F5 = CAxis(30deg).
FixtureData BuildRealisticMicrostructure()
{
  FixtureData td = CreateScaffold(/*nX=*/5, /*nY=*/5, /*nZ=*/1, /*numFeatures=*/6, /*numCrystalStructures=*/3);

  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);
  (*td.crystalStructures)[2] = static_cast<uint32>(ebsdlib::CrystalStructure::Cubic_High);

  SetAvgCAxis(td, 1, CAxisFromPhiDeg(0.0f));
  SetAvgCAxis(td, 2, CAxisFromPhiDeg(10.0f));
  SetAvgCAxis(td, 3, CAxisFromPhiDeg(10.0f)); // ignored — F3 is cubic
  SetAvgCAxis(td, 4, CAxisFromPhiDeg(20.0f));
  SetAvgCAxis(td, 5, CAxisFromPhiDeg(30.0f));

  // Cell-level layout.
  for(usize y = 0; y < 5; ++y)
  {
    for(usize x = 0; x < 5; ++x)
    {
      const usize cellIdx = y * 5 + x;
      int32 fid = static_cast<int32>(y + 1); // y=0 -> F1, y=1 -> F2, ..., y=4 -> F5
      (*td.featureIds)[cellIdx] = fid;
      (*td.cellPhases)[cellIdx] = (fid == 3) ? 2 : 1; // F3 cubic, all others hex
    }
  }

  // F1: all Phi=0
  for(usize x = 0; x < 5; ++x)
  {
    SetCellQuat(td, /*y=0*/ 0 * 5 + x, QuatFromPhiDeg(0.0f));
  }
  // F2: Phi = 8, 9, 10, 11, 12
  const std::array<float32, 5> f2Phis = {8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
  for(usize x = 0; x < 5; ++x)
  {
    SetCellQuat(td, /*y=1*/ 1 * 5 + x, QuatFromPhiDeg(f2Phis[x]));
  }
  // F3: cubic — quats irrelevant (algorithm takes the non-hex skip branch)
  // F4: all Phi=20
  for(usize x = 0; x < 5; ++x)
  {
    SetCellQuat(td, /*y=3*/ 3 * 5 + x, QuatFromPhiDeg(20.0f));
  }
  // F5: Phi = 25, 28, 30, 32, 35
  const std::array<float32, 5> f5Phis = {25.0f, 28.0f, 30.0f, 32.0f, 35.0f};
  for(usize x = 0; x < 5; ++x)
  {
    SetCellQuat(td, /*y=4*/ 4 * 5 + x, QuatFromPhiDeg(f5Phis[x]));
  }

  return td;
}

} // namespace AnalyticalFixtures
} // namespace

// =============================================================================
// Fixture 1 — Simple Hex Triple (closed-form sanity).
//
// 3x1x1 ImageGeom, 1 hex feature, 3 cells at Phi = 0, 5, 10.
// AvgCAxes[F1] = CAxisFromPhiDeg(5) — the geometric mean for these 3 cells.
// Expected:
//   cellRefCAxisMis = [5, 0, 5]
//   featAvg[F1]     = (5+0+5)/3 = 10/3 ≈ 3.3333
//   featStdev[F1]   = sqrt(50/9) = 5*sqrt(2)/3 ≈ 2.3570
// Exercises paths 3 (per-cell normal), 5 (per-feature avg), 6+7 (stddev).
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 1 — Simple Hex Triple", "[OrientationAnalysis][ComputeFeatureReferenceCAxisMisorientationsFilter]")
{
  using namespace AnalyticalFixtures;

  FixtureData td = CreateScaffold(/*nX=*/3, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/2, /*numCrystalStructures=*/2);
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);

  SetCellQuat(td, 0, QuatFromPhiDeg(0.0f));
  SetCellQuat(td, 1, QuatFromPhiDeg(5.0f));
  SetCellQuat(td, 2, QuatFromPhiDeg(10.0f));

  SetAvgCAxis(td, 1, CAxisFromPhiDeg(5.0f));

  ComputeFeatureReferenceCAxisMisorientationsFilter filter;
  Arguments args = BuildArgs();

  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& cellMisos = GetOutputCellMisos(td.ds);
  const auto& featAvg = GetOutputFeatureAvg(td.ds);
  const auto& featStdev = GetOutputFeatureStdev(td.ds);

  REQUIRE(cellMisos[0] == Approx(5.0f).margin(1e-3f));
  REQUIRE(cellMisos[1] == Approx(0.0f).margin(1e-3f));
  REQUIRE(cellMisos[2] == Approx(5.0f).margin(1e-3f));

  REQUIRE(featAvg[1] == Approx(10.0f / 3.0f).margin(1e-3f));
  REQUIRE(featStdev[1] == Approx(5.0f * std::sqrt(2.0f) / 3.0f).margin(1e-3f));
}

// =============================================================================
// Fixture 2 — Realistic Microstructure (exposes divide-by-zero on F3).
//
// 5x5x1 ImageGeom, 6 features (sentinel + 5 real). F1, F2, F4, F5 are Hex; F3 is Cubic.
// Per-feature expected (Phi values laid out in BuildRealisticMicrostructure):
//   F1 (Phi all 0, avg=0):       misos=[0,0,0,0,0]    -> avg=0,     stddev=0
//   F2 (Phi 8-12, avg=10):       misos=[2,1,0,1,2]    -> avg=1.2,   stddev=sqrt(0.56) ≈ 0.7483
//   F3 (all cubic):              algorithm skip path  -> avg=NaN,   stddev=NaN  (loadbearing)
//   F4 (Phi all 20, avg=20):     misos=[0,0,0,0,0]    -> avg=0,     stddev=0
//   F5 (Phi 25,28,30,32,35; avg=30): misos=[5,2,0,2,5] -> avg=2.8,   stddev=sqrt(3.76) ≈ 1.9391
//
// Exercises paths 2 (mixed-phase warning), 3, 4 (per-cell skip), 5, 6, 7.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 1 — Realistic Microstructure (exposes divide-by-zero)",
          "[OrientationAnalysis][ComputeFeatureReferenceCAxisMisorientationsFilter]")
{
  using namespace AnalyticalFixtures;

  FixtureData td = BuildRealisticMicrostructure();

  ComputeFeatureReferenceCAxisMisorientationsFilter filter;
  Arguments args = BuildArgs();

  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Algorithm emits warning -9803 because we have mixed phases (cubic + hex).
  REQUIRE_FALSE(executeResult.result.warnings().empty());

  const auto& cellMisos = GetOutputCellMisos(td.ds);
  const auto& featAvg = GetOutputFeatureAvg(td.ds);
  const auto& featStdev = GetOutputFeatureStdev(td.ds);

  // Per-cell misos:
  //   y=0 (F1):  all 0.0
  //   y=1 (F2):  2, 1, 0, 1, 2
  //   y=2 (F3):  all 0.0 (skip branch writes 0 explicitly)
  //   y=3 (F4):  all 0.0
  //   y=4 (F5):  5, 2, 0, 2, 5
  const std::array<float32, 25> expectedCells = {
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, //
      2.0f, 1.0f, 0.0f, 1.0f, 2.0f, //
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, //
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, //
      5.0f, 2.0f, 0.0f, 2.0f, 5.0f  //
  };
  for(usize i = 0; i < 25; ++i)
  {
    INFO("Cell index " << i);
    REQUIRE(cellMisos[i] == Approx(expectedCells[i]).margin(1e-3f));
  }

  REQUIRE(featAvg[1] == Approx(0.0f).margin(1e-3f));
  REQUIRE(featAvg[2] == Approx(1.2f).margin(1e-3f));
  REQUIRE(std::isnan(featAvg[3]));
  REQUIRE(featAvg[4] == Approx(0.0f).margin(1e-3f));
  REQUIRE(featAvg[5] == Approx(2.8f).margin(1e-3f));

  REQUIRE(featStdev[1] == Approx(0.0f).margin(1e-3f));
  REQUIRE(featStdev[2] == Approx(std::sqrt(0.56f)).margin(1e-3f));
  REQUIRE(std::isnan(featStdev[3]));
  REQUIRE(featStdev[4] == Approx(0.0f).margin(1e-3f));
  REQUIRE(featStdev[5] == Approx(std::sqrt(3.76f)).margin(1e-3f));
}

// =============================================================================
// Fixture 3 — All-Identical Orientation Feature (Class 4 invariant I5).
//
// 5x1x1 hex feature with all cells at the same orientation Phi=10 and avgCAxes pointed
// at the same Phi=10. Confirms that "feature whose cells all share the avg orientation"
// produces zero per-cell miso, zero featAvg, zero featStdev — a sanity check that the
// algorithm doesn't accidentally introduce drift.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 1 — All-Identical Orientation Feature",
          "[OrientationAnalysis][ComputeFeatureReferenceCAxisMisorientationsFilter]")
{
  using namespace AnalyticalFixtures;

  FixtureData td = CreateScaffold(/*nX=*/5, /*nY=*/1, /*nZ=*/1, /*numFeatures=*/2, /*numCrystalStructures=*/2);
  (*td.crystalStructures)[1] = static_cast<uint32>(ebsdlib::CrystalStructure::Hexagonal_High);

  for(usize i = 0; i < 5; ++i)
  {
    SetCellQuat(td, i, QuatFromPhiDeg(10.0f));
  }
  SetAvgCAxis(td, 1, CAxisFromPhiDeg(10.0f));

  ComputeFeatureReferenceCAxisMisorientationsFilter filter;
  Arguments args = BuildArgs();

  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& cellMisos = GetOutputCellMisos(td.ds);
  const auto& featAvg = GetOutputFeatureAvg(td.ds);
  const auto& featStdev = GetOutputFeatureStdev(td.ds);

  for(usize i = 0; i < 5; ++i)
  {
    INFO("Cell index " << i);
    REQUIRE(cellMisos[i] == Approx(0.0f).margin(1e-3f));
  }
  REQUIRE(featAvg[1] == Approx(0.0f).margin(1e-3f));
  REQUIRE(featStdev[1] == Approx(0.0f).margin(1e-3f));
}

// =============================================================================
// Fixture 4 — Class 4 Invariants (3 SECTIONs, reuses Realistic Microstructure).
//
// Invariants asserted:
//   (i)   Range:  cellRefCAxisMis[i] ∈ [0, 90] for hex cells, == 0 for non-hex.
//   (ii)  Formula: featAvg[f] equals the arithmetic mean of cellRefCAxisMis[hex+valid cells in f].
//   (iii) NaN propagation (load-bearing): all-non-hex feature → featAvg, featStdev both NaN.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 4 — Invariants", "[OrientationAnalysis][ComputeFeatureReferenceCAxisMisorientationsFilter]")
{
  using namespace AnalyticalFixtures;

  FixtureData td = BuildRealisticMicrostructure();

  ComputeFeatureReferenceCAxisMisorientationsFilter filter;
  Arguments args = BuildArgs();

  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& cellMisos = GetOutputCellMisos(td.ds);
  const auto& featAvg = GetOutputFeatureAvg(td.ds);
  const auto& featStdev = GetOutputFeatureStdev(td.ds);

  SECTION("(i) Range bounds")
  {
    for(usize i = 0; i < 25; ++i)
    {
      INFO("Cell index " << i);
      const int32 fid = (*td.featureIds)[i];
      const bool isHexCell = (fid != 3); // F3 is cubic in this fixture
      if(isHexCell)
      {
        REQUIRE(cellMisos[i] >= 0.0f);
        REQUIRE(cellMisos[i] <= 90.0f);
      }
      else
      {
        REQUIRE(cellMisos[i] == Approx(0.0f).margin(1e-6f));
      }
    }
  }

  SECTION("(ii) Per-feature averaging formula")
  {
    // For each feature, recompute the mean from the per-cell array and compare to featAvg.
    // F3 is non-hex (counts==0) — handled in section (iii); skip it here.
    for(int32 fid = 1; fid <= 5; ++fid)
    {
      if(fid == 3)
      {
        continue;
      }
      double sumMisos = 0.0;
      usize count = 0;
      for(usize i = 0; i < 25; ++i)
      {
        if((*td.featureIds)[i] == fid && (*td.cellPhases)[i] != 2) // skip cubic cells
        {
          sumMisos += cellMisos[i];
          count++;
        }
      }
      const float32 expectedAvg = static_cast<float32>(sumMisos / static_cast<double>(count));
      INFO("Feature " << fid);
      REQUIRE(featAvg[fid] == Approx(expectedAvg).margin(1e-3f));
    }
  }

  SECTION("(iii) All-non-hex feature → NaN")
  {
    // F3 has 0 hex cells; algorithm reaches line 176 with counts[3]==0 and produces NaN/NaN.
    // This documents the latent divide-by-zero as desired-behavior.
    REQUIRE(std::isnan(featAvg[3]));
    REQUIRE(std::isnan(featStdev[3]));
  }
}

// =============================================================================
// Retained from pre-V&V: exercises path 1 (all-non-hex preflight error -9802) by
// mutating CrystalStructures[1] from Hex_High to Cubic_High after loading the
// caxis_data exemplar input.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: InValid Filter Execution", "[OrientationAnalysis][ComputeFeatureReferenceCAxisMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "caxis_data.tar.gz", "caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/caxis_data/7_0_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
  crystalStructs[1] = 1;

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFeatureReferenceCAxisMisorientationsFilter filter;
  Arguments args;

  // Invalid crystal structure type : should fail in execute
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_AvgCAxesArrayPath_Key, std::make_any<DataPath>(k_AvgCAxesPath));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureAvgCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatAvgCAxisMisComputed));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureStdevCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatStDevCAxisMisComputed));
  args.insertOrAssign(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureReferenceCAxisMisorientationsArrayName_Key, std::make_any<std::string>(k_FeatRefCAxisMisComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: SIMPL Backwards Compatibility",
          "[OrientationAnalysis][ComputeFeatureReferenceCAxisMisorientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureReferenceCAxisMisorientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureReferenceCAxisMisorientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureReferenceCAxisMisorientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_AvgCAxesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureAvgCAxisMisorientationsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureStdevCAxisMisorientationsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeFeatureReferenceCAxisMisorientationsFilter::k_FeatureReferenceCAxisMisorientationsArrayName_Key) == "TestName");
    }
  }
}
