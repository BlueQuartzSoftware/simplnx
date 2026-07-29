#include "OrientationAnalysis/Filters/ComputeKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
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
const DataPath k_EnsembleDataPath = k_ImageGeomPath.createChildPath("EnsembleData");

const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_CellPhasesName = "Phases";
const std::string k_QuatsName = "Quats";
const std::string k_CrystalStructuresName = "CrystalStructures";

const std::string k_KAMOutName = "KernelAverageMisorientationsOut";

std::array<float32, 4> QuatFromPhi1Deg(float32 phi1Deg)
{
  const float32 halfAngleRad = (phi1Deg * 0.5f) * 3.14159265358979323846f / 180.0f;
  return {0.0f, 0.0f, std::sin(halfAngleRad), std::cos(halfAngleRad)};
}

struct FixtureData
{
  DataStructure ds;
  ImageGeom* geom = nullptr;
  AttributeMatrix* cellAM = nullptr;
  AttributeMatrix* ensembleAM = nullptr;
  Int32Array* featureIds = nullptr;
  Int32Array* cellPhases = nullptr;
  Float32Array* quats = nullptr;
  UInt32Array* crystalStructures = nullptr;
  usize totalCells = 0;
};

// Build a scaffold with an ImageGeom of the given (nX, nY, nZ) dimensions, a cell-level AM, and an
// ensemble AM. Cell arrays are sized as totalCells = nX*nY*nZ and initialized to: FeatureIds=1,
// CellPhases=1, Quats=identity. CrystalStructures index 0 = sentinel, index 1 = Cubic_High.
FixtureData CreateScaffold(usize nX, usize nY, usize nZ, usize numCrystalStructures = 2)
{
  FixtureData td;
  td.totalCells = nX * nY * nZ;

  td.geom = ImageGeom::Create(td.ds, k_GeomName);
  td.geom->setSpacing({1.0f, 1.0f, 1.0f});
  td.geom->setOrigin({0.0f, 0.0f, 0.0f});
  td.geom->setDimensions({nX, nY, nZ});

  td.cellAM = AttributeMatrix::Create(td.ds, "CellData", ShapeType{nZ, nY, nX}, td.geom->getId());
  td.ensembleAM = AttributeMatrix::Create(td.ds, "EnsembleData", ShapeType{numCrystalStructures}, td.geom->getId());

  td.featureIds = CreateTestDataArray<int32>(td.ds, k_FeatureIdsName, {nZ, nY, nX}, {1}, td.cellAM->getId());
  td.cellPhases = CreateTestDataArray<int32>(td.ds, k_CellPhasesName, {nZ, nY, nX}, {1}, td.cellAM->getId());
  td.quats = CreateTestDataArray<float32>(td.ds, k_QuatsName, {nZ, nY, nX}, {4}, td.cellAM->getId());
  td.crystalStructures = CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numCrystalStructures}, {1}, td.ensembleAM->getId());

  for(usize i = 0; i < td.totalCells; ++i)
  {
    (*td.featureIds)[i] = 1;
    (*td.cellPhases)[i] = 1;
    (*td.quats)[i * 4 + 0] = 0.0f;
    (*td.quats)[i * 4 + 1] = 0.0f;
    (*td.quats)[i * 4 + 2] = 0.0f;
    (*td.quats)[i * 4 + 3] = 1.0f; // identity quaternion
  }
  (*td.crystalStructures)[0] = 999u;
  if(numCrystalStructures > 1)
  {
    (*td.crystalStructures)[1] = 1u; // Cubic_High (EbsdLib LaueOps index)
  }
  return td;
}

void SetCellQuat(FixtureData& td, usize cellIdx, const std::array<float32, 4>& q)
{
  (*td.quats)[cellIdx * 4 + 0] = q[0];
  (*td.quats)[cellIdx * 4 + 1] = q[1];
  (*td.quats)[cellIdx * 4 + 2] = q[2];
  (*td.quats)[cellIdx * 4 + 3] = q[3];
}

Arguments BuildArgs(const std::vector<int32>& kernelRadius, bool useFeatureIds = true)
{
  Arguments args;
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_KernelSize_Key, std::make_any<VectorInt32Parameter::ValueType>(kernelRadius));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_UseFeatureIds_Key, std::make_any<bool>(useFeatureIds));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_FeatureIdsName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_CellPhasesName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_QuatsName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_EnsembleDataPath.createChildPath(k_CrystalStructuresName)));
  args.insertOrAssign(ComputeKernelAvgMisorientationsFilter::k_KernelAverageMisorientationsArrayName_Key, std::make_any<std::string>(k_KAMOutName));
  return args;
}

const Float32Array& GetOutputKAM(const DataStructure& ds)
{
  return ds.getDataRefAs<Float32Array>(k_CellDataPath.createChildPath(k_KAMOutName));
}
} // namespace AnalyticalFixtures
} // namespace

// Retired 2026-06-03 (V&V cycle): the main exemplar-comparison TEST_CASE that consumed
// `6_6_stats_test_v2.tar.gz` was removed. The exemplar `KernelAverageMisorientations` array was a
// circular oracle (regenerated from pre-EbsdLib-2.4.1 SIMPLNX output). The precision shift surfaced
// on the failing ctest for this filter. The Class 1 + Class 4 data fixtures below replace the
// retired test; they cover all 5 algorithmic paths through `FindKernelAvgMisorientationsImpl::convert()`.
// The shared archive remains downloaded for `AlignSectionsMutualInformation`, `ComputeShapesFilter`,
// and `ComputeSchmidsFilter` tests, which still consume it.
// See `vv/provenance/ComputeKernelAvgMisorientationsFilter.md` for retirement details.

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeKernelAvgMisorientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeKernelAvgMisorientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeKernelAvgMisorientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (IntVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeKernelAvgMisorientationsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeKernelAvgMisorientationsFilter::k_KernelAverageMisorientationsArrayName_Key) == "TestName");
    }
  }
}

// =====================================================================================
// Class 1 (Analytical) data fixtures + Class 4 (Invariant) companion.
//
// All Class 1 fixtures use pure phi1 Bunge ZXZ Euler rotations (phi1, 0, 0) with Phi = phi2 = 0,
// stored as quaternions via QuatFromPhi1Deg(). For cubic Laue class, the symmetry group's c-axis
// 4-fold rotation reduces phi1 differences modulo 90 degrees. By keeping all phi1 differences <= 45
// degrees, the symmetry-reduced minimum rotation magnitude is exactly |delta_phi1|, so the expected
// misorientation between two cubic-phase cells is just |phi1_neighbor - phi1_focal| in degrees.
// This is what makes the oracle closed-form.
// =====================================================================================

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Uniform 2D Single Feature", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // 3x3x1 image, single feature, single phase, all cells share the identity quaternion.
  // Kernel radius {1,1,0} => 3x3x1 kernel (9 cells max, fewer at boundaries).
  // Expected: every cell's KAM = 0 (all in-kernel neighbors have the same orientation).
  // Exercises focal-valid path, feature-id-match accumulator, 2D boundary clamping.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 3, 1);

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 1, 0});

  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  for(usize i = 0; i < td.totalCells; ++i)
  {
    REQUIRE(kam[i] == Approx(0.0f).margin(1e-4f));
  }

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - 1D x-axis Gradient", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // 5x1x1 image, single feature, single phase. Per-cell phi1: [0, 5, 10, 15, 20] degrees.
  // Kernel radius {1,0,0} => 1D kernel along x with up to 3 cells per kernel.
  // Expected KAM:
  //   cell 0 (focal phi1=0):  neighbors {self=0, x+1=5}                  -> misos {0, 5}        -> avg 5/2 = 2.500
  //   cell 1 (focal phi1=5):  neighbors {x-1=0, self=5, x+1=10}          -> misos {5, 0, 5}     -> avg 10/3 ~ 3.3333
  //   cell 2 (focal phi1=10): neighbors {x-1=5, self=10, x+1=15}         -> misos {5, 0, 5}     -> avg 10/3 ~ 3.3333
  //   cell 3 (focal phi1=15): neighbors {x-1=10, self=15, x+1=20}        -> misos {5, 0, 5}     -> avg 10/3 ~ 3.3333
  //   cell 4 (focal phi1=20): neighbors {x-1=15, self=20}                -> misos {5, 0}        -> avg 5/2 = 2.500
  // Exercises averaging arithmetic + 1D x-stride boundary clamp.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(5, 1, 1);
  const std::array<float32, 5> phi1Deg = {0.0f, 5.0f, 10.0f, 15.0f, 20.0f};
  for(usize i = 0; i < 5; ++i)
  {
    AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
  }

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 5> expected = {2.5f, 10.0f / 3.0f, 10.0f / 3.0f, 10.0f / 3.0f, 2.5f};
  for(usize i = 0; i < 5; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - 1D z-axis Gradient (3D path)", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // 1x1x3 image (single column of cells in z), single feature, single phase.
  // Per-plane phi1: [0, 10, 20] degrees. Kernel radius {0,0,1} => 1D kernel along z.
  // Expected KAM:
  //   plane 0 (focal=0):  neighbors {self=0, z+1=10}              -> misos {0, 10}      -> avg 10/2 = 5.0
  //   plane 1 (focal=10): neighbors {z-1=0, self=10, z+1=20}      -> misos {10, 0, 10}  -> avg 20/3 ~ 6.6667
  //   plane 2 (focal=20): neighbors {z-1=10, self=20}             -> misos {10, 0}      -> avg 10/2 = 5.0
  // Exercises the 3D outer-loop z-path + z-stride boundary clamp.
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(1, 1, 3);
  const std::array<float32, 3> phi1Deg = {0.0f, 10.0f, 20.0f};
  for(usize i = 0; i < 3; ++i)
  {
    AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
  }

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({0, 0, 1});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 3> expected = {5.0f, 20.0f / 3.0f, 5.0f};
  for(usize i = 0; i < 3; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Multi-Feature Multi-Voxel + Background", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // 6x1x1 image with mixed features, multi-voxel features, and a background cell. All cells with
  // phase != 0 are cubic. Layout:
  //   cell x=0: featureId=1, phase=1, phi1=0   degrees
  //   cell x=1: featureId=1, phase=1, phi1=10  degrees
  //   cell x=2: featureId=2, phase=1, phi1=0   degrees
  //   cell x=3: featureId=2, phase=1, phi1=20  degrees
  //   cell x=4: featureId=0, phase=0, phi1=N/A (background)
  //   cell x=5: featureId=1, phase=1, phi1=30  degrees
  //
  // Kernel radius {1,0,0}. Algorithm only accumulates misorientations between cells in the same
  // featureId; background cells (featureId=0 OR phase=0) get KAM=0 directly.
  //
  // Expected KAM:
  //   cell 0 (F1): kernel cells x=0(self,F1), x=1(F1).
  //                same-feat: |0-0|=0, |10-0|=10. sum=10, divisor=2 -> KAM = 5.0
  //   cell 1 (F1): kernel cells x=0(F1), x=1(self,F1), x=2(F2 - SKIP).
  //                same-feat: |0-10|=10, |10-10|=0. sum=10, divisor=2 -> KAM = 5.0
  //   cell 2 (F2): kernel cells x=1(F1 - SKIP), x=2(self,F2), x=3(F2).
  //                same-feat: |0-0|=0, |20-0|=20. sum=20, divisor=2 -> KAM = 10.0
  //   cell 3 (F2): kernel cells x=2(F2), x=3(self,F2), x=4(F0 - SKIP, also background-phase mismatch).
  //                same-feat: |0-20|=20, |20-20|=0. sum=20, divisor=2 -> KAM = 10.0
  //   cell 4 (F0,P0): focal-invalid path - KAM = 0 exactly.
  //   cell 5 (F1): kernel cells x=4(F0 - SKIP), x=5(self,F1).
  //                same-feat: |30-30|=0. sum=0, divisor=1 -> KAM = 0.0
  //
  // Exercises: multi-voxel within-feature averaging (cells 0-3), multi-feature mismatch skip
  // (cells 1, 2, 5), background skip path (cell 4), isolated single-cell feature (cell 5).
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(6, 1, 1);
  // FeatureIds: [1, 1, 2, 2, 0, 1]
  (*td.featureIds)[0] = 1;
  (*td.featureIds)[1] = 1;
  (*td.featureIds)[2] = 2;
  (*td.featureIds)[3] = 2;
  (*td.featureIds)[4] = 0;
  (*td.featureIds)[5] = 1;
  // CellPhases: [1, 1, 1, 1, 0, 1]
  (*td.cellPhases)[0] = 1;
  (*td.cellPhases)[1] = 1;
  (*td.cellPhases)[2] = 1;
  (*td.cellPhases)[3] = 1;
  (*td.cellPhases)[4] = 0;
  (*td.cellPhases)[5] = 1;
  // Quats per phi1 (cell 4's quat is set to identity by CreateScaffold; algorithm short-circuits anyway):
  AnalyticalFixtures::SetCellQuat(td, 0, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
  AnalyticalFixtures::SetCellQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(20.0f));
  AnalyticalFixtures::SetCellQuat(td, 5, AnalyticalFixtures::QuatFromPhi1Deg(30.0f));

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 6> expected = {5.0f, 5.0f, 10.0f, 10.0f, 0.0f, 0.0f};
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }
  // Background cell must be exactly 0 (not just within tolerance) - it is set via the explicit
  // KAM=0 short-circuit at the bottom of the inner loop.
  REQUIRE(kam[4] == 0.0f);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Per-Voxel Mode (use_feature_ids = false)", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Same 6x1x1 layout as the Multi-Feature Multi-Voxel fixture, but run with use_feature_ids =
  // false (per-voxel KAM, issue #1613). In this mode a kernel neighbor contributes iff it is
  // in-bounds AND featureIds[neighbor] > 0 AND cellPhases[neighbor] == cellPhases[focal]. The
  // focal-validity gate (featureIds[focal] > 0 && cellPhases[focal] > 0) is unchanged.
  //   cell x=0: featureId=1, phase=1, phi1=0   degrees
  //   cell x=1: featureId=1, phase=1, phi1=10  degrees
  //   cell x=2: featureId=2, phase=1, phi1=0   degrees
  //   cell x=3: featureId=2, phase=1, phi1=20  degrees
  //   cell x=4: featureId=0, phase=0, phi1=N/A (background)
  //   cell x=5: featureId=1, phase=1, phi1=30  degrees
  //
  // Kernel radius {1,0,0}. Expected per-voxel KAM:
  //   cell 0: neighbors {self=0, x=1(F1,P1)=10}                       -> avg 10/2  = 5.0
  //   cell 1: neighbors {x=0(F1,P1)=10, self=0, x=2(F2,P1)=10}        -> avg 20/3 ~= 6.6667
  //           (default per-grain mode gives 5.0 here - x=2 was skipped; this cell proves the mode differs)
  //   cell 2: neighbors {x=1(F1,P1)=10, self=0, x=3(F2,P1)=20}        -> avg 30/3  = 10.0
  //   cell 3: neighbors {x=2(F2,P1)=20, self=0}; x=4 SKIPPED (featureId=0) -> avg 20/2 = 10.0
  //   cell 4: focal-invalid (featureId=0, phase=0)                    -> KAM = 0 exactly
  //   cell 5: x=4 SKIPPED (featureId=0), {self=0}                     -> avg 0/1   = 0.0
  //
  // Exercises: cross-feature accumulation (cells 1, 2), featureId=0 neighbor exclusion in
  // per-voxel mode (cells 3, 5), unchanged focal-invalid path (cell 4).
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(6, 1, 1);
  // FeatureIds: [1, 1, 2, 2, 0, 1]
  (*td.featureIds)[0] = 1;
  (*td.featureIds)[1] = 1;
  (*td.featureIds)[2] = 2;
  (*td.featureIds)[3] = 2;
  (*td.featureIds)[4] = 0;
  (*td.featureIds)[5] = 1;
  // CellPhases: [1, 1, 1, 1, 0, 1]
  (*td.cellPhases)[0] = 1;
  (*td.cellPhases)[1] = 1;
  (*td.cellPhases)[2] = 1;
  (*td.cellPhases)[3] = 1;
  (*td.cellPhases)[4] = 0;
  (*td.cellPhases)[5] = 1;
  AnalyticalFixtures::SetCellQuat(td, 0, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 1, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
  AnalyticalFixtures::SetCellQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
  AnalyticalFixtures::SetCellQuat(td, 3, AnalyticalFixtures::QuatFromPhi1Deg(20.0f));
  AnalyticalFixtures::SetCellQuat(td, 5, AnalyticalFixtures::QuatFromPhi1Deg(30.0f));

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0}, false);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 6> expected = {5.0f, 20.0f / 3.0f, 10.0f, 10.0f, 0.0f, 0.0f};
  for(usize i = 0; i < 6; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }
  REQUIRE(kam[4] == 0.0f);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 1 - Per-Voxel Mode Two-Phase Gates", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // 5x1x1 image, every cell its own feature, two cubic phases. Verifies the per-voxel mode's
  // neighbor gates: (a) different-phase neighbors are excluded, (b) featureId=0 neighbors are
  // excluded, (c) a focal cell with featureId=0 is still forced to KAM=0 even when its phase > 0.
  // CrystalStructures: index 0 = sentinel, index 1 = Cubic_High, index 2 = Cubic_High.
  //   cell x=0: featureId=1, phase=1, phi1=0   degrees
  //   cell x=1: featureId=2, phase=1, phi1=10  degrees
  //   cell x=2: featureId=3, phase=2, phi1=20  degrees
  //   cell x=3: featureId=4, phase=1, phi1=30  degrees
  //   cell x=4: featureId=0, phase=1, phi1=40  degrees (invalid focal: featureId == 0)
  //
  // Kernel radius {1,0,0}, use_feature_ids = false. Expected KAM:
  //   cell 0: {self=0, x=1(P1)=10}                                   -> 10/2 = 5.0
  //           (per-grain mode would give 0.0 - every feature is a single cell)
  //   cell 1: {x=0(P1)=10, self=0}; x=2 SKIPPED (phase 2 != 1)       -> 10/2 = 5.0
  //   cell 2: {self=0}; x=1 and x=3 SKIPPED (phase 1 != 2)           -> 0/1  = 0.0
  //   cell 3: {self=0}; x=2 SKIPPED (phase), x=4 SKIPPED (featureId=0) -> 0/1 = 0.0
  //   cell 4: focal featureId == 0                                    -> KAM = 0 exactly
  AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(5, 1, 1, 3);
  (*td.crystalStructures)[2] = 1u; // Cubic_High for phase 2
  const std::array<int32, 5> featureIds = {1, 2, 3, 4, 0};
  const std::array<int32, 5> phases = {1, 1, 2, 1, 1};
  const std::array<float32, 5> phi1Deg = {0.0f, 10.0f, 20.0f, 30.0f, 40.0f};
  for(usize i = 0; i < 5; ++i)
  {
    (*td.featureIds)[i] = featureIds[i];
    (*td.cellPhases)[i] = phases[i];
    AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
  }

  ComputeKernelAvgMisorientationsFilter filter;
  Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0}, false);
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
  const std::array<float32, 5> expected = {5.0f, 5.0f, 0.0f, 0.0f, 0.0f};
  for(usize i = 0; i < 5; ++i)
  {
    REQUIRE(kam[i] == Approx(expected[i]).margin(1e-3f));
  }
  // The invalid focal cell takes the explicit KAM=0 short-circuit.
  REQUIRE(kam[4] == 0.0f);

  UnitTest::CheckArraysInheritTupleDims(td.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 4 - Mode Equivalence on Single Feature", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Invariant: on single-feature single-phase data, per-grain and per-voxel modes admit exactly
  // the same neighbor set (every neighbor passes both gates), so the outputs must be identical
  // bit-for-bit. 3x3x3 gradient fixture, phi1 = 2x + 3y + 4z degrees (max 18, well under the
  // 45-degree cubic FZ bound), full 3D kernel {1,1,1}.
  auto buildFixture = []() {
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 3, 3);
    for(usize z = 0; z < 3; ++z)
    {
      for(usize y = 0; y < 3; ++y)
      {
        for(usize x = 0; x < 3; ++x)
        {
          const usize idx = (z * 9) + (y * 3) + x;
          const auto phi1 = static_cast<float32>(2 * x + 3 * y + 4 * z);
          AnalyticalFixtures::SetCellQuat(td, idx, AnalyticalFixtures::QuatFromPhi1Deg(phi1));
        }
      }
    }
    return td;
  };

  AnalyticalFixtures::FixtureData tdPerGrain = buildFixture();
  AnalyticalFixtures::FixtureData tdPerVoxel = buildFixture();

  ComputeKernelAvgMisorientationsFilter filter;
  auto runFilter = [&filter](AnalyticalFixtures::FixtureData& td, bool useFeatureIds) {
    Arguments args = AnalyticalFixtures::BuildArgs({1, 1, 1}, useFeatureIds);
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };
  runFilter(tdPerGrain, true);
  runFilter(tdPerVoxel, false);

  const auto& kamPerGrain = AnalyticalFixtures::GetOutputKAM(tdPerGrain.ds);
  const auto& kamPerVoxel = AnalyticalFixtures::GetOutputKAM(tdPerVoxel.ds);
  bool anyNonzero = false;
  for(usize i = 0; i < tdPerGrain.totalCells; ++i)
  {
    REQUIRE(kamPerGrain[i] == kamPerVoxel[i]);
    if(kamPerGrain[i] > 1e-4f)
    {
      anyNonzero = true;
    }
  }
  REQUIRE(anyNonzero); // sanity: the fixture is non-trivial

  UnitTest::CheckArraysInheritTupleDims(tdPerGrain.ds);
  UnitTest::CheckArraysInheritTupleDims(tdPerVoxel.ds);
}

TEST_CASE("OrientationAnalysis::ComputeKernelAvgMisorientationsFilter: Class 4 - Invariants", "[OrientationAnalysis][ComputeKernelAvgMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Class 4 invariants asserted across several derived fixture variants. These invariants are
  // oracle-agnostic - they hold for any input, so they catch regressions even when specific
  // expected values change. Two precondition invariants tested:
  //   (i)  Uniform-orientation single-feature => KAM == 0 everywhere (any cell, any kernel size).
  //   (ii) Background cell (featureId==0 OR cellPhases==0) => KAM == 0 exactly.
  // Plus three universal invariants on the gradient fixture:
  //   (iii)  All KAM values are non-negative.
  //   (iv)   All KAM values are <= 62.8 degrees (Mackenzie cubic upper bound).
  //   (v)    KAM value at non-trivial focal cells must be > 0 (sanity check that the algorithm
  //          actually computed something rather than zeroing everything).
  constexpr float32 k_CubicMaxAngleDeg = 62.8f;

  SECTION("(i) Uniform-orientation single-feature => KAM == 0")
  {
    // 3x3x3 uniform-identity-quaternion fixture, full 3D kernel.
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 3, 3);
    ComputeKernelAvgMisorientationsFilter filter;
    Arguments args = AnalyticalFixtures::BuildArgs({1, 1, 1});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
    for(usize i = 0; i < td.totalCells; ++i)
    {
      REQUIRE(kam[i] == Approx(0.0f).margin(1e-4f));
    }
  }

  SECTION("(ii) Background cell => KAM == 0 exactly")
  {
    // 3x1x1 with cells [F1P1, F0P0, F1P1]. Background cell at index 1.
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(3, 1, 1);
    (*td.featureIds)[1] = 0;
    (*td.cellPhases)[1] = 0;
    AnalyticalFixtures::SetCellQuat(td, 0, AnalyticalFixtures::QuatFromPhi1Deg(0.0f));
    AnalyticalFixtures::SetCellQuat(td, 2, AnalyticalFixtures::QuatFromPhi1Deg(10.0f));
    ComputeKernelAvgMisorientationsFilter filter;
    Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
    REQUIRE(kam[1] == 0.0f);
  }

  SECTION("(iii, iv, v) Range and non-triviality invariants on x-axis gradient")
  {
    AnalyticalFixtures::FixtureData td = AnalyticalFixtures::CreateScaffold(5, 1, 1);
    const std::array<float32, 5> phi1Deg = {0.0f, 5.0f, 10.0f, 15.0f, 20.0f};
    for(usize i = 0; i < 5; ++i)
    {
      AnalyticalFixtures::SetCellQuat(td, i, AnalyticalFixtures::QuatFromPhi1Deg(phi1Deg[i]));
    }
    ComputeKernelAvgMisorientationsFilter filter;
    Arguments args = AnalyticalFixtures::BuildArgs({1, 0, 0});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& kam = AnalyticalFixtures::GetOutputKAM(td.ds);
    for(usize i = 0; i < td.totalCells; ++i)
    {
      REQUIRE(kam[i] >= 0.0f);
      REQUIRE(kam[i] <= k_CubicMaxAngleDeg);
    }
    // Non-triviality: at least one cell must have KAM > 0 (algorithm did something).
    bool anyNonzero = false;
    for(usize i = 0; i < td.totalCells; ++i)
    {
      if(kam[i] > 1e-4f)
      {
        anyNonzero = true;
      }
    }
    REQUIRE(anyNonzero);
  }
}
