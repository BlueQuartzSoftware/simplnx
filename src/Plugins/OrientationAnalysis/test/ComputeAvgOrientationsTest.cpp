/* ============================================================================
 * ComputeAvgOrientations V&V test suite.
 *
 * Verification is established INDEPENDENTLY of legacy DREAM3D, per the V&V
 * policy (src/Plugins/OrientationAnalysis/vv/ComputeAvgOrientationsFilter.md):
 *
 *  - Rodrigues average  : Class 1 (analytical) + Class 4 (invariant). Triclinic
 *                         symmetry makes getNearestQuat a no-op, so the running
 *                         quaternion average reduces to the closed form
 *                         normalize(sum(q_i)), positive-oriented.
 *  - vMF / Watson average: Class 2 (EbsdLib reference) + Class 4 (invariant).
 *                         The EM math lives in EbsdLib DirectionalStats and is
 *                         tested by EbsdLib's DirectionalStatsTest.cpp at THIS
 *                         filter's exact config (numEM=5, numIter=10, seed=43514)
 *                         on the 22 reference quaternions reproduced below. We do
 *                         NOT re-test the EM math; we only verify the filter's
 *                         value-add (per-feature voxel gathering, FZ reduction,
 *                         phase->crystal-structure lookup, single/zero-element
 *                         handling, correct output-tuple placement).
 *
 * The prior exemplar archive 7_ComputeAvgOrientation_v2.tar.gz was a CIRCULAR
 * ORACLE (regenerated from this filter's own output in PR #1577) and has been
 * retired in favor of the inline oracle below.
 * ========================================================================== */

#include "OrientationAnalysis/Filters/Algorithms/ComputeAvgOrientations.hpp"
#include "OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// ---- DataStructure layout -------------------------------------------------
const std::string k_ImageGeomName = "ImageGeom";
const std::string k_CellDataName = "Cell Data";
const std::string k_FeatureDataName = "Cell Feature Data";
const std::string k_EnsembleDataName = "Cell Ensemble Data";
const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_PhasesName = "Phases";
const std::string k_QuatsName = "Quats";
const std::string k_CrystalStructuresName = "CrystalStructures";

const DataPath k_ImageGeomPath({k_ImageGeomName});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath(k_CellDataName);
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath(k_FeatureIdsName);
const DataPath k_PhasesPath = k_CellDataPath.createChildPath(k_PhasesName);
const DataPath k_QuatsPath = k_CellDataPath.createChildPath(k_QuatsName);
const DataPath k_FeatureDataPath = k_ImageGeomPath.createChildPath(k_FeatureDataName);
const DataPath k_CrystalStructuresPath = k_ImageGeomPath.createChildPath(k_EnsembleDataName).createChildPath(k_CrystalStructuresName);

// Output array names
const std::string k_AvgQuatsName = "AvgQuats";
const std::string k_AvgEulerName = "AvgEulerAngles";
const std::string k_VMFQuatsName = "vMF Avg Quats";
const std::string k_VMFEulerName = "vMF Avg EulerAngles";
const std::string k_VMFKappaName = "vMF Kappas";
const std::string k_WatsonQuatsName = "Watson Avg Quats";
const std::string k_WatsonEulerName = "Watson Avg EulerAngles";
const std::string k_WatsonKappaName = "Watson Kappas";

// Crystal structure enumeration (EbsdLib::CrystalStructure)
constexpr uint32 k_Unknown = 999;
constexpr uint32 k_CubicHigh = 1;
constexpr uint32 k_Triclinic = 4;

// Rz(theta) = (0, 0, sin(theta/2), cos(theta/2)) in (x,y,z,w) storage order.
constexpr float32 k_Rz90_z = 0.7071067811865476f; // sin(45) = cos(45)
constexpr float32 k_Rz45_z = 0.3826834323650898f; // sin(22.5)
constexpr float32 k_Rz45_w = 0.9238795325112867f; // cos(22.5)

// Cubic-equivalent z-rotation representations of the physical orientation Rz(30):
// Rz(90) is a cubic symmetry operator, so Rz(30+90k) all describe the same orientation.
constexpr float32 k_Rz30_z = 0.2588190451025208f;   // sin(15)
constexpr float32 k_Rz30_w = 0.9659258262890683f;   // cos(15)
constexpr float32 k_Rz120_z = 0.8660254037844386f;  // sin(60)
constexpr float32 k_Rz120_w = 0.5f;                 // cos(60)
constexpr float32 k_Rz210_z = 0.9659258262890683f;  // sin(105)
constexpr float32 k_Rz210_w = -0.2588190451025208f; // cos(105)
constexpr float32 k_Rz300_z = 0.5f;                 // sin(150)
constexpr float32 k_Rz300_w = -0.8660254037844386f; // cos(150)

// ---------------------------------------------------------------------------
// Build a DataStructure with an ImageGeom, a Cell Data AM (FeatureIds, Phases,
// Quats), a Cell Feature Data AM (numFeatures tuples, output target), and a
// Cell Ensemble Data AM (CrystalStructures). All values supplied by the caller.
// ---------------------------------------------------------------------------
DataStructure BuildDataStructure(int32 numCells, int32 numFeatures, const std::vector<int32>& featureIds, const std::vector<int32>& phases, const std::vector<float32>& quats,
                                 const std::vector<uint32>& crystalStructures)
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setDimensions({static_cast<usize>(numCells), 1, 1});

  AttributeMatrix* cellDataAM = AttributeMatrix::Create(dataStructure, k_CellDataName, ShapeType{static_cast<usize>(numCells)}, imageGeom->getId());

  {
    auto buffer = std::make_unique<int32[]>(featureIds.size());
    std::copy(featureIds.begin(), featureIds.end(), buffer.get());
    DataArray<int32>::Create(dataStructure, k_FeatureIdsName, std::make_shared<Int32DataStore>(std::move(buffer), cellDataAM->getShape(), ShapeType{1}), cellDataAM->getId());
  }
  {
    auto buffer = std::make_unique<int32[]>(phases.size());
    std::copy(phases.begin(), phases.end(), buffer.get());
    DataArray<int32>::Create(dataStructure, k_PhasesName, std::make_shared<Int32DataStore>(std::move(buffer), cellDataAM->getShape(), ShapeType{1}), cellDataAM->getId());
  }
  {
    auto buffer = std::make_unique<float32[]>(quats.size());
    std::copy(quats.begin(), quats.end(), buffer.get());
    DataArray<float32>::Create(dataStructure, k_QuatsName, std::make_shared<Float32DataStore>(std::move(buffer), cellDataAM->getShape(), ShapeType{4}), cellDataAM->getId());
  }

  // Cell Feature Data AM (output target). Output arrays are created here by preflight.
  AttributeMatrix::Create(dataStructure, k_FeatureDataName, ShapeType{static_cast<usize>(numFeatures)}, imageGeom->getId());

  // Cell Ensemble Data AM + CrystalStructures
  AttributeMatrix* ensembleAM = AttributeMatrix::Create(dataStructure, k_EnsembleDataName, ShapeType{crystalStructures.size()}, imageGeom->getId());
  {
    auto buffer = std::make_unique<uint32[]>(crystalStructures.size());
    std::copy(crystalStructures.begin(), crystalStructures.end(), buffer.get());
    DataArray<uint32>::Create(dataStructure, k_CrystalStructuresName, std::make_shared<UInt32DataStore>(std::move(buffer), ensembleAM->getShape(), ShapeType{1}), ensembleAM->getId());
  }

  return dataStructure;
}

// Configure common input array arguments.
void SetInputArgs(Arguments& args)
{
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureDataPath));
}

// Assert a 4-component quaternion tuple equals (x,y,z,w) within margin.
void CheckQuat(const Float32Array& arr, usize feature, float32 x, float32 y, float32 z, float32 w, float32 margin)
{
  const auto& store = arr.getDataStoreRef();
  REQUIRE(store.getValue(feature * 4 + 0) == Approx(x).margin(margin));
  REQUIRE(store.getValue(feature * 4 + 1) == Approx(y).margin(margin));
  REQUIRE(store.getValue(feature * 4 + 2) == Approx(z).margin(margin));
  REQUIRE(store.getValue(feature * 4 + 3) == Approx(w).margin(margin));
}

// Assert all components of a quaternion tuple are NaN.
void CheckQuatNaN(const Float32Array& arr, usize feature)
{
  const auto& store = arr.getDataStoreRef();
  for(usize c = 0; c < 4; c++)
  {
    REQUIRE(std::isnan(store.getValue(feature * 4 + c)));
  }
}

// Assert a quaternion tuple is unit-norm and northern-hemisphere (w >= 0).
void CheckUnitNorthern(const Float32Array& arr, usize feature)
{
  const auto& store = arr.getDataStoreRef();
  const double xq = store.getValue(feature * 4 + 0);
  const double yq = store.getValue(feature * 4 + 1);
  const double zq = store.getValue(feature * 4 + 2);
  const double wq = store.getValue(feature * 4 + 3);
  REQUIRE(std::sqrt(xq * xq + yq * yq + zq * zq + wq * wq) == Approx(1.0).margin(1.0e-5));
  REQUIRE(wq >= 0.0);
}
} // namespace

// =============================================================================
// Class 1 (Analytical) + Class 4 (Invariant) — Rodrigues average, Triclinic
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: Rodrigues Analytical Oracle", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  // 8 cells across 5 active features (+ feature 5 deliberately empty), Triclinic.
  //  cell : feature, phase, quat(x,y,z,w)
  //   0   :   0,      0,     identity      (background; phase 0 -> ignored)
  //   1   :   1,      1,     identity      single-voxel identity
  //   2   :   2,      1,     Rz(90)        single-voxel non-identity
  //   3,4 :   3,      1,     identity,Rz90 mean of identity & Rz(90) = Rz(45)
  //   5,6,7:  4,      1,     3x Rz(90)     N identical -> Rz(90)
  //   (feature 5 has no cells -> zero-voxel feature)
  const int32 numCells = 8;
  const int32 numFeatures = 6;
  const std::vector<int32> featureIds = {0, 1, 2, 3, 3, 4, 4, 4};
  const std::vector<int32> phases = {0, 1, 1, 1, 1, 1, 1, 1};
  const std::vector<float32> quats = {
      0.0f, 0.0f, 0.0f,     1.0f,     // c0 identity
      0.0f, 0.0f, 0.0f,     1.0f,     // c1 identity
      0.0f, 0.0f, k_Rz90_z, k_Rz90_z, // c2 Rz90
      0.0f, 0.0f, 0.0f,     1.0f,     // c3 identity
      0.0f, 0.0f, k_Rz90_z, k_Rz90_z, // c4 Rz90
      0.0f, 0.0f, k_Rz90_z, k_Rz90_z, // c5 Rz90
      0.0f, 0.0f, k_Rz90_z, k_Rz90_z, // c6 Rz90
      0.0f, 0.0f, k_Rz90_z, k_Rz90_z, // c7 Rz90
  };
  const std::vector<uint32> crystalStructures = {k_Unknown, k_Triclinic};

  DataStructure dataStructure = BuildDataStructure(numCells, numFeatures, featureIds, phases, quats, crystalStructures);

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  SetInputArgs(args);
  // Enable all three methods so we also exercise the vMF/Watson value-add invariants on this data.
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // ---- Class 1: Rodrigues exact averages (x,y,z,w) ----
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName)));
  const auto& avgQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName));
  constexpr float32 k_QuatTol = 1.0e-6f;
  CheckQuat(avgQuats, 0, 0.0f, 0.0f, 0.0f, 1.0f, k_QuatTol);         // F0 background -> identity
  CheckQuat(avgQuats, 1, 0.0f, 0.0f, 0.0f, 1.0f, k_QuatTol);         // F1 single identity
  CheckQuat(avgQuats, 2, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, k_QuatTol); // F2 single Rz90
  CheckQuat(avgQuats, 3, 0.0f, 0.0f, k_Rz45_z, k_Rz45_w, k_QuatTol); // F3 mean(identity,Rz90) = Rz45
  CheckQuat(avgQuats, 4, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, k_QuatTol); // F4 3x Rz90 = Rz90
  CheckQuat(avgQuats, 5, 0.0f, 0.0f, 0.0f, 1.0f, k_QuatTol);         // F5 empty -> identity

  // ---- Class 4: Rodrigues invariants ----
  for(usize f = 0; f < static_cast<usize>(numFeatures); f++)
  {
    CheckUnitNorthern(avgQuats, f);
  }
  // Identity / empty features -> zero Euler.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgEulerName)));
  const auto& avgEuler = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgEulerName));
  for(usize c = 0; c < 3; c++)
  {
    REQUIRE(avgEuler.getDataStoreRef().getValue(1 * 3 + c) == Approx(0.0f).margin(1.0e-6f)); // F1 identity
    REQUIRE(avgEuler.getDataStoreRef().getValue(5 * 3 + c) == Approx(0.0f).margin(1.0e-6f)); // F5 empty
  }
  // z-rotation fixtures (F2, F3, F4): Euler is gimbal-degenerate (only phi1+phi2 is
  // determined), so assert the invariant — Phi ~= 0 and all components finite — rather
  // than individual phi1/phi2 values.
  for(usize f : {static_cast<usize>(2), static_cast<usize>(3), static_cast<usize>(4)})
  {
    REQUIRE(std::isfinite(avgEuler.getDataStoreRef().getValue(f * 3 + 0)));
    REQUIRE(avgEuler.getDataStoreRef().getValue(f * 3 + 1) == Approx(0.0f).margin(1.0e-6f));
    REQUIRE(std::isfinite(avgEuler.getDataStoreRef().getValue(f * 3 + 2)));
  }

  // ---- Class 4: vMF / Watson value-add invariants on this small data ----
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName)));
  const auto& vmfQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName));
  const auto& vmfKappa = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFKappaName));
  const auto& watsonQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_WatsonQuatsName));
  const auto& watsonKappa = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_WatsonKappaName));

  for(const Float32Array* arr : {&vmfQuats, &watsonQuats})
  {
    // Zero-voxel features (F0, F5) -> NaN.
    CheckQuatNaN(*arr, 0);
    CheckQuatNaN(*arr, 5);
    // Single-voxel features (F1 identity, F2 Rz90) -> muhat == FZ(voxel quat).
    CheckQuat(*arr, 1, 0.0f, 0.0f, 0.0f, 1.0f, 1.0e-6f);
    CheckQuat(*arr, 2, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, 1.0e-6f);
    // Multi-voxel features (F3, F4) -> unit + northern hemisphere.
    CheckUnitNorthern(*arr, 3);
    CheckUnitNorthern(*arr, 4);
  }
  // Single-voxel features -> kappa == 0 (EM skipped).
  REQUIRE(vmfKappa.getDataStoreRef().getValue(1) == Approx(0.0f).margin(1.0e-6f));
  REQUIRE(vmfKappa.getDataStoreRef().getValue(2) == Approx(0.0f).margin(1.0e-6f));
  REQUIRE(watsonKappa.getDataStoreRef().getValue(1) == Approx(0.0f).margin(1.0e-6f));
  REQUIRE(watsonKappa.getDataStoreRef().getValue(2) == Approx(0.0f).margin(1.0e-6f));
  // Zero-voxel features -> kappa NaN.
  REQUIRE(std::isnan(vmfKappa.getDataStoreRef().getValue(0)));
  REQUIRE(std::isnan(vmfKappa.getDataStoreRef().getValue(5)));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =============================================================================
// Class 2 (EbsdLib reference) — vMF / Watson average reproduces EbsdLib's
// already-asserted DirectionalStatsTest values, proving the filter routes data
// correctly without re-deriving the EM math.
//
// Reference: EbsdLib/Source/Test/DirectionalStatsTest.cpp ("DirectionalStatsTest:VMF"
// and ":Watson"), which run numEM=5, numIter=10, seed=43514 over these same 22
// quaternions FZ-reduced with Cubic_High ops. The filter performs the identical
// pipeline. Tolerances are loosened from EbsdLib's 1e-6 to absorb the float32
// round-trip of the input quaternions through the DataArray.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: vMF/Watson EbsdLib Reference Oracle", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  // The 22 reference quaternions from EbsdLib DirectionalStatsTest detail::k_TestQuats,
  // in (x,y,z,w) order. All assigned to feature 1 (feature 0 is a background filler).
  const std::vector<std::array<float32, 4>> refQuats = {
      {0.6719963424253053f, 0.6129423860730265f, 0.1364166710960659f, 0.3925723359703083f},    {0.295631110116223f, 0.8806176765212745f, -0.1670640703896814f, 0.330460816004792f},
      {0.293789811168501f, 0.8831169225237752f, -0.1660320799434538f, 0.3259223779297207f},    {0.2947005642746592f, 0.8832293192391356f, -0.1668647237149576f, 0.3243666305773475f},
      {0.1769340339862296f, 0.9269220251506355f, 0.1767900887688091f, 0.2797412579887368f},    {-0.4895854071041807f, 0.7937642457108052f, 0.0101229019146293f, 0.3607519622103487f},
      {-0.4885846329994489f, 0.7943361326081295f, 0.007316124555987299f, 0.3609177733936573f}, {-0.4891812284025829f, 0.7943711408640211f, 0.005622187254144007f, 0.3600619493245755f},
      {-0.4904884701659861f, 0.7944995174028157f, 0.01142771077728101f, 0.3578560952496329f},  {0.6682755620419383f, 0.6167302380229949f, 0.1345230991224496f, 0.3936433950774778f},
      {0.669694655986085f, 0.6188030782527685f, 0.1372821020479966f, 0.3869695628158121f},     {0.6725537285776072f, 0.6116846895234272f, 0.1362780204804377f, 0.3936262490141125f},
      {0.6712100069793456f, 0.6122801449826168f, 0.1349260392167796f, 0.3954555784561961f},    {0.1724037140729195f, 0.9289624416784542f, 0.1788682254529524f, 0.27443013545888f},
      {0.1753504855620956f, 0.9267758042757149f, 0.1810262782021287f, 0.2785108658966909f},    {0.1781196317732192f, 0.9243210069735922f, 0.1813607833836466f, 0.284626666169506f},
      {-0.4860300998793424f, 0.7962186305444112f, 0.01011143900233245f, 0.3601505146276495f},  {-0.4888952933345148f, 0.7960889874984892f, 0.007050530465747282f, 0.3566146465852751f},
      {0.2949445932779656f, 0.8840049176186158f, -0.16492632529573f, 0.3230205871870195f},     {0.1773081200034897f, 0.9267173624983174f, 0.1786384658179278f, 0.2790072743768655f},
      {-0.4859900547906299f, 0.7949907303647343f, 0.009415733770101149f, 0.3629252667372687f}, {-0.4927679779186163f, 0.7939850383340995f, 0.005047892871246353f, 0.3560084235199459f},
  };

  const int32 numFeatures = 2;
  const int32 numCells = 1 + static_cast<int32>(refQuats.size()); // 1 background + 22
  std::vector<int32> featureIds(numCells, 1);
  featureIds[0] = 0; // background filler -> feature 0
  std::vector<int32> phases(numCells, 1);
  phases[0] = 0; // background phase 0 -> ignored
  std::vector<float32> quats(static_cast<usize>(numCells) * 4, 0.0f);
  quats[3] = 1.0f; // background identity quat (w)
  for(usize i = 0; i < refQuats.size(); i++)
  {
    quats[(i + 1) * 4 + 0] = refQuats[i][0];
    quats[(i + 1) * 4 + 1] = refQuats[i][1];
    quats[(i + 1) * 4 + 2] = refQuats[i][2];
    quats[(i + 1) * 4 + 3] = refQuats[i][3];
  }
  const std::vector<uint32> crystalStructures = {k_Unknown, k_CubicHigh};

  DataStructure dataStructure = BuildDataStructure(numCells, numFeatures, featureIds, phases, quats, crystalStructures);

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  SetInputArgs(args);
  // Rodrigues off; vMF + Watson on.
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName)));
  const auto& vmfQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName));
  const auto& vmfKappa = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFKappaName));
  const auto& watsonQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_WatsonQuatsName));
  const auto& watsonKappa = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_WatsonKappaName));

  // Tolerances loosened from EbsdLib's 1e-6 to absorb the float32 input round-trip.
  constexpr float32 k_QuatMargin = 5.0e-3f;
  constexpr double k_KappaRel = 0.02; // 2%

  // vMF reference (DirectionalStatsTest.cpp:202-206), stored (x,y,z,w).
  CheckQuat(vmfQuats, 1, 0.3322000547718371f, -0.1964639452260062f, 0.2450656693404858f, 0.8893749825279105f, k_QuatMargin);
  REQUIRE(vmfKappa.getDataStoreRef().getValue(1) == Approx(88.9943042750539774).epsilon(k_KappaRel));

  // Watson reference (DirectionalStatsTest.cpp:249-253), stored (x,y,z,w).
  CheckQuat(watsonQuats, 1, 0.2948298270586034f, -0.2106011604618418f, 0.2378717152588106f, 0.9011878668560466f, k_QuatMargin);
  REQUIRE(watsonKappa.getDataStoreRef().getValue(1) == Approx(30.5730272919979669).epsilon(k_KappaRel));

  // Value-add: background feature 0 (zero voxels) -> NaN.
  CheckQuatNaN(vmfQuats, 0);
  CheckQuatNaN(watsonQuats, 0);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =============================================================================
// Class 4 (Invariant) — Rodrigues under non-trivial (cubic) symmetry (#1660).
// One Cubic_High feature is fed the SAME physical orientation, Rz(30 deg), as
// five different representations: cubic-equivalent z-rotations Rz(30+90k) plus
// the negated double-cover representative -Rz(30). Because Rz(90) is a cubic
// symmetry operator (and getNearestQuat canonicalizes the sign), every voxel's
// nearest-equivalent pick is exactly Rz(30), so the running average MUST
// finalize to Rz(30) — an implementation-independent expectation that exercises
// the 24-operator symmetry-reduction branch, the reset-to-identity first-voxel
// branch (on a non-trivial representation), and the count weighting.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: Rodrigues Cubic Symmetry Invariant", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  const int32 numCells = 6;
  const int32 numFeatures = 2;
  const std::vector<int32> featureIds = {0, 1, 1, 1, 1, 1};
  const std::vector<int32> phases = {0, 1, 1, 1, 1, 1};
  // Deliberately scrambled order so the first-voxel reset operates on Rz(120),
  // not the base representation.
  const std::vector<float32> quats = {
      0.0f, 0.0f, 0.0f,      1.0f,      // c0 background identity
      0.0f, 0.0f, k_Rz120_z, k_Rz120_w, // c1 Rz(120) == Rz(30) * Rz(90)
      0.0f, 0.0f, k_Rz30_z,  k_Rz30_w,  // c2 Rz(30)  base representation
      0.0f, 0.0f, -k_Rz30_z, -k_Rz30_w, // c3 -Rz(30) double-cover representative
      0.0f, 0.0f, k_Rz210_z, k_Rz210_w, // c4 Rz(210) == Rz(30) * Rz(180)
      0.0f, 0.0f, k_Rz300_z, k_Rz300_w, // c5 Rz(300) == Rz(30) * Rz(270)
  };
  const std::vector<uint32> crystalStructures = {k_Unknown, k_CubicHigh};

  DataStructure dataStructure = BuildDataStructure(numCells, numFeatures, featureIds, phases, quats, crystalStructures);

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  SetInputArgs(args);
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName)));
  const auto& avgQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName));
  CheckQuat(avgQuats, 1, 0.0f, 0.0f, k_Rz30_z, k_Rz30_w, 1.0e-5f);
  CheckUnitNorthern(avgQuats, 1);

  // Euler invariant for a pure z-rotation: Phi ~= 0, all components finite.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgEulerName)));
  const auto& avgEuler = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgEulerName));
  REQUIRE(std::isfinite(avgEuler.getDataStoreRef().getValue(1 * 3 + 0)));
  REQUIRE(avgEuler.getDataStoreRef().getValue(1 * 3 + 1) == Approx(0.0f).margin(1.0e-5f));
  REQUIRE(std::isfinite(avgEuler.getDataStoreRef().getValue(1 * 3 + 2)));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =============================================================================
// Class 4 (Invariant) — Rodrigues voxel-ordering independence. The F3 fixture
// of the analytical oracle (mean of identity and Rz(90) = Rz(45)) is run with
// both voxel orderings; the result must be identical. Both orders accumulate
// to (0, 0, 0.7071, 1.7071) before normalization (see provenance sidecar).
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: Rodrigues Voxel Ordering Independence", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<float32> quatIdentity = {0.0f, 0.0f, 0.0f, 1.0f};
  const std::vector<float32> quatRz90 = {0.0f, 0.0f, k_Rz90_z, k_Rz90_z};

  const std::string ordering = GENERATE("identity-first", "Rz90-first");
  DYNAMIC_SECTION("Ordering: " << ordering)
  {
    std::vector<float32> quats;
    const std::vector<float32>& first = (ordering == "identity-first") ? quatIdentity : quatRz90;
    const std::vector<float32>& second = (ordering == "identity-first") ? quatRz90 : quatIdentity;
    quats.insert(quats.end(), first.begin(), first.end());
    quats.insert(quats.end(), second.begin(), second.end());

    const std::vector<int32> featureIds = {1, 1};
    const std::vector<int32> phases = {1, 1};
    const std::vector<uint32> crystalStructures = {k_Unknown, k_Triclinic};
    DataStructure dataStructure = BuildDataStructure(2, 2, featureIds, phases, quats, crystalStructures);

    ComputeAvgOrientationsFilter filter;
    Arguments args;
    SetInputArgs(args);
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName)));
    const auto& avgQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName));
    CheckQuat(avgQuats, 1, 0.0f, 0.0f, k_Rz45_z, k_Rz45_w, 1.0e-6f);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

// =============================================================================
// Regression (#1659) — the vMF/Watson voxel gather must ignore phase-0
// (unindexed) voxels, matching the counting pass and the Rodrigues path.
// Pre-fix, the gather loop collected every voxel of the feature regardless of
// phase, so a phase-0 voxel contributed a garbage quaternion to the EM average
// (and defeated the single-voxel shortcut when featureNumVoxels == 1).
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: vMF/Watson Ignores Phase-0 Voxels", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Two active features, each polluted with a phase-0 voxel carrying a garbage
  // (non-unit, southern-hemisphere) quaternion. The counting pass gates on
  // phase > 0, so:
  //   F1: 1 counted voxel (Rz90)  -> single-voxel shortcut: muhat == Rz90, kappa == 0
  //   F2: 2 counted voxels (Rz90) -> EM over identical quats: muhat == Rz90
  const int32 numCells = 5;
  const int32 numFeatures = 3; // F0 has no voxels -> NaN
  const std::vector<int32> featureIds = {1, 1, 2, 2, 2};
  const std::vector<int32> phases = {1, 0, 1, 1, 0};
  const std::vector<float32> quats = {
      0.0f,  0.0f,   k_Rz90_z, k_Rz90_z, // c0 F1 phase 1: Rz90
      0.62f, -0.41f, 0.55f,    -0.37f,   // c1 F1 phase 0: garbage
      0.0f,  0.0f,   k_Rz90_z, k_Rz90_z, // c2 F2 phase 1: Rz90
      0.0f,  0.0f,   k_Rz90_z, k_Rz90_z, // c3 F2 phase 1: Rz90
      0.62f, -0.41f, 0.55f,    -0.37f,   // c4 F2 phase 0: garbage
  };
  const std::vector<uint32> crystalStructures = {k_Unknown, k_Triclinic};

  DataStructure dataStructure = BuildDataStructure(numCells, numFeatures, featureIds, phases, quats, crystalStructures);

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  SetInputArgs(args);
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName)));
  const auto& vmfQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName));
  const auto& vmfKappa = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFKappaName));
  const auto& watsonQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_WatsonQuatsName));
  const auto& watsonKappa = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_WatsonKappaName));

  for(const Float32Array* arr : {&vmfQuats, &watsonQuats})
  {
    // F1: the phase-0 garbage voxel must not defeat the single-voxel shortcut.
    CheckQuat(*arr, 1, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, 1.0e-6f);
    // F2: EM over the two identical counted voxels must land on Rz90 —
    // orientation equality via |dot(muhat, Rz90)| == 1, robust to double cover.
    const auto& store = arr->getDataStoreRef();
    const double dot = store.getValue(2 * 4 + 2) * k_Rz90_z + store.getValue(2 * 4 + 3) * k_Rz90_z;
    REQUIRE(std::abs(dot) == Approx(1.0).margin(1.0e-4));
    REQUIRE(store.getValue(2 * 4 + 0) == Approx(0.0).margin(1.0e-4));
    REQUIRE(store.getValue(2 * 4 + 1) == Approx(0.0).margin(1.0e-4));
  }
  // F1 single counted voxel -> EM skipped -> kappa == 0.
  REQUIRE(vmfKappa.getDataStoreRef().getValue(1) == Approx(0.0f).margin(1.0e-6f));
  REQUIRE(watsonKappa.getDataStoreRef().getValue(1) == Approx(0.0f).margin(1.0e-6f));

  // Cross-path agreement: the Rodrigues path already excludes phase-0 voxels.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName)));
  const auto& avgQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName));
  CheckQuat(avgQuats, 1, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, 1.0e-6f);
  CheckQuat(avgQuats, 2, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, 1.0e-6f);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =============================================================================
// Guards (#1661) — unknown/unsupported crystal structures and out-of-range
// Phases values must be excluded from BOTH averaging paths, must not read out
// of range on the CrystalStructures array, and the drop must be reported as
// warnings (-54671 unknown crystal structure, -54672 out-of-range phase) —
// never a silent drop.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: Unknown Crystal Structure and Out-Of-Range Phase Guards", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Ensemble: phase 1 valid Triclinic, phase 2 Unknown (999).
  //  cell : feature, phase, quat            expectation
  //   0   :   1,      1,    Rz90            valid, averaged
  //   1   :   2,      2,    Rz90            unknown crystal structure -> dropped
  //   2   :   3,      7,    Rz90            phase index out of range   -> dropped
  //   3   :   1,      7,    garbage         out-of-range phase mixed into valid feature -> dropped
  const int32 numCells = 4;
  const int32 numFeatures = 4; // F0 empty
  const std::vector<int32> featureIds = {1, 2, 3, 1};
  const std::vector<int32> phases = {1, 2, 7, 7};
  const std::vector<float32> quats = {
      0.0f,  0.0f,   k_Rz90_z, k_Rz90_z, // c0 valid
      0.0f,  0.0f,   k_Rz90_z, k_Rz90_z, // c1 unknown xtal
      0.0f,  0.0f,   k_Rz90_z, k_Rz90_z, // c2 OOB phase
      0.62f, -0.41f, 0.55f,    -0.37f,   // c3 OOB phase, garbage quat
  };
  const std::vector<uint32> crystalStructures = {k_Unknown, k_Triclinic, k_Unknown};

  DataStructure dataStructure = BuildDataStructure(numCells, numFeatures, featureIds, phases, quats, crystalStructures);

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  SetInputArgs(args);
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The drops must be surfaced as warnings, not silent.
  std::vector<int32> warningCodes;
  for(const auto& warning : executeResult.result.warnings())
  {
    warningCodes.push_back(warning.code);
  }
  REQUIRE(std::count(warningCodes.begin(), warningCodes.end(), -54671) > 0); // unknown crystal structure
  REQUIRE(std::count(warningCodes.begin(), warningCodes.end(), -54672) > 0); // out-of-range phase

  // Rodrigues: F1 keeps only its valid voxel; F2/F3 fully dropped -> identity.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName)));
  const auto& avgQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_AvgQuatsName));
  CheckQuat(avgQuats, 1, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, 1.0e-6f);
  CheckQuat(avgQuats, 2, 0.0f, 0.0f, 0.0f, 1.0f, 1.0e-6f);
  CheckQuat(avgQuats, 3, 0.0f, 0.0f, 0.0f, 1.0f, 1.0e-6f);

  // vMF/Watson: F1 single valid voxel -> shortcut; F2 (unknown xtal) and F3
  // (all voxels out-of-range phase) -> NaN.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName)));
  const auto& vmfQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFQuatsName));
  const auto& vmfKappa = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_VMFKappaName));
  const auto& watsonQuats = dataStructure.getDataRefAs<Float32Array>(k_FeatureDataPath.createChildPath(k_WatsonQuatsName));
  for(const Float32Array* arr : {&vmfQuats, &watsonQuats})
  {
    CheckQuat(*arr, 1, 0.0f, 0.0f, k_Rz90_z, k_Rz90_z, 1.0e-6f);
    CheckQuatNaN(*arr, 2);
    CheckQuatNaN(*arr, 3);
  }
  REQUIRE(vmfKappa.getDataStoreRef().getValue(1) == Approx(0.0f).margin(1.0e-6f));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// =============================================================================
// Error path — no averaging method enabled is now rejected in preflight with
// -54673 so the GUI surfaces it before execution (issue #1661). The runtime
// -54670 check in the algorithm remains as a backstop for direct invocation.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: No Method Enabled Error", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  const int32 numCells = 2;
  const int32 numFeatures = 2;
  const std::vector<int32> featureIds = {0, 1};
  const std::vector<int32> phases = {1, 1};
  const std::vector<float32> quats = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  const std::vector<uint32> crystalStructures = {k_Unknown, k_Triclinic};

  DataStructure dataStructure = BuildDataStructure(numCells, numFeatures, featureIds, phases, quats, crystalStructures);

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  SetInputArgs(args);
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -54673);

  // Runtime backstop: invoking the algorithm directly (bypassing preflight) with no
  // method enabled must still fail with -54670 since no result array exists.
  ComputeAvgOrientationsInputValues inputValues;
  inputValues.cellFeatureIdsArrayPath = k_FeatureIdsPath;
  inputValues.cellPhasesArrayPath = k_PhasesPath;
  inputValues.cellQuatsArrayPath = k_QuatsPath;
  inputValues.crystalStructuresArrayPath = k_CrystalStructuresPath;
  inputValues.useRodriguesAverage = false;
  inputValues.useVonMisesAverage = false;
  inputValues.useWatsonAverage = false;
  inputValues.avgQuatsArrayPath = k_FeatureDataPath.createChildPath(k_AvgQuatsName);
  inputValues.avgEulerAnglesArrayPath = k_FeatureDataPath.createChildPath(k_AvgEulerName);
  inputValues.VMFQuatsArrayPath = k_FeatureDataPath.createChildPath(k_VMFQuatsName);
  inputValues.VMFEulerAnglesArrayPath = k_FeatureDataPath.createChildPath(k_VMFEulerName);
  inputValues.VMFKappaArrayPath = k_FeatureDataPath.createChildPath(k_VMFKappaName);
  inputValues.WatsonQuatsArrayPath = k_FeatureDataPath.createChildPath(k_WatsonQuatsName);
  inputValues.WatsonEulerAnglesArrayPath = k_FeatureDataPath.createChildPath(k_WatsonEulerName);
  inputValues.WatsonKappaArrayPath = k_FeatureDataPath.createChildPath(k_WatsonKappaName);

  const std::atomic_bool shouldCancel{false};
  Result<> algoResult = ComputeAvgOrientations(dataStructure, IFilter::MessageHandler{}, shouldCancel, &inputValues)();
  REQUIRE(algoResult.invalid());
  REQUIRE(algoResult.errors()[0].code == -54670);
}

// =============================================================================
// Preflight error — mismatched cell-array tuple counts => error -651.
// (Covers the GCOV target from PR #1644; the duplicate TEST_CASE that #1644
// added was removed in favor of this one — issue #1661.)
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientations: Cell Array Tuple Mismatch Error (-651)", "[OrientationAnalysis][ComputeAvgOrientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Valid 4-cell structure, then add a Phases array with a mismatched tuple count (5).
  const std::vector<int32> featureIds = {0, 1, 1, 1};
  const std::vector<int32> phases = {1, 1, 1, 1};
  const std::vector<float32> quats = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  const std::vector<uint32> crystalStructures = {k_Unknown, k_Triclinic};
  DataStructure dataStructure = BuildDataStructure(4, 2, featureIds, phases, quats, crystalStructures);

  // Add a mismatched (5-tuple) Phases array under a separate AttributeMatrix.
  auto* imageGeom = dataStructure.getDataAs<ImageGeom>(k_ImageGeomPath);
  AttributeMatrix* badAM = AttributeMatrix::Create(dataStructure, "BadData", ShapeType{5}, imageGeom->getId());
  {
    auto buffer = std::make_unique<int32[]>(5);
    std::fill_n(buffer.get(), 5, 1);
    DataArray<int32>::Create(dataStructure, "PhasesBad", std::make_shared<Int32DataStore>(std::move(buffer), badAM->getShape(), ShapeType{1}), badAM->getId());
  }
  const DataPath badPhasesPath = k_ImageGeomPath.createChildPath("BadData").createChildPath("PhasesBad");

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  SetInputArgs(args);
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(badPhasesPath)); // mismatched tuple count
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>(k_AvgEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>(k_VMFQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>(k_VMFEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>(k_VMFKappaName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>(k_WatsonQuatsName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>(k_WatsonEulerName));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>(k_WatsonKappaName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -651);
}

// =============================================================================
// SIMPL Backwards Compatibility (kept, unchanged) — validates UUID + parameter
// conversion of the legacy six-parameter (Rodrigues) filter.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientationsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeAvgOrientationsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeAvgOrientationsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeAvgOrientationsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeAvgOrientationsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CellQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key) == "TestArray");
      CHECK(args.value<std::string>(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key) == "TestArray");
    }
  }
}
