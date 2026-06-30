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

#include "OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
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
// Error path — no averaging method enabled => no feature-level result array is
// created, so the algorithm returns error -54670.
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
}

// =============================================================================
// Preflight error — mismatched cell-array tuple counts => error -651.
// (Mirrors the coverage target from GCOV/PR #1644; kept here so the V&V suite is
// self-complete. Reconcile the duplicate at merge time if both land.)
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
}

// =============================================================================
// SIMPL Backwards Compatibility (kept, unchanged) — validates UUID + parameter
// conversion of the legacy six-parameter (Rodrigues) filter.
// =============================================================================
TEST_CASE("OrientationAnalysis::ComputeAvgOrientationsFilter: Preflight Error - Cell array tuple count mismatch (-651)", "[OrientationAnalysis][ComputeAvgOrientationsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Build a minimal synthetic DataStructure where the three cell-level arrays that are
  // validated together (Quats, Phases, FeatureIds) do NOT all share the same tuple count.
  // This drives the validateNumberOfTuples() guard in preflightImpl that emits error -651.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", {10}, {4}, cellAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {10}, {1}, cellAM->getId());

  // CellPhases lives in a separate AttributeMatrix with a deliberately different tuple
  // count (9 != 10) so the cross-array tuple-count check fails.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  AttributeMatrix::Create(dataStructure, "CellFeatureData", {5}, imageGeom->getId());

  ComputeAvgOrientationsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "Quats"})));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellFeatureData"})));

  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseRodriguesAverage_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesQuatsArrayName_Key, std::make_any<std::string>("AvgQuats"));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_RodriguesAvgEulerArrayName_Key, std::make_any<std::string>("AvgEulerAngles"));

  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseVonMisesFisher_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgQuatsArrayName_Key, std::make_any<std::string>("vMF Avg Quats"));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherAvgEulerArrayName_Key, std::make_any<std::string>("vMF Avg EulerAngles"));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_VonMisesFisherKappaArrayName_Key, std::make_any<std::string>("vMF Kappas"));

  args.insertOrAssign(ComputeAvgOrientationsFilter::k_UseWatson_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgQuatsArrayName_Key, std::make_any<std::string>("Watson Avg Quats"));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonAvgEulerArrayName_Key, std::make_any<std::string>("Watson Avg EulerAngles"));
  args.insertOrAssign(ComputeAvgOrientationsFilter::k_WatsonKappaArrayName_Key, std::make_any<std::string>("Watson Kappas"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -651);
}

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
