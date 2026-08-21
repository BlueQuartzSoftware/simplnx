#include "OrientationAnalysis/Filters/ComputeSchmidsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <catch2/catch.hpp>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_SchmidsArrayName("Schmids");
const std::string k_SlipSystemsArrayName("SlipSystems");
const std::string k_PolesArrayName("Poles");
const std::string k_PhisArrayName("Schmid_Phis");
const std::string k_LambdasArrayName("Schmid_Lambdas");
const std::string k_CalculatedArrayPrefix("Calculated_");
} // namespace

// =====================================================================================
// V&V hand-built fixtures (Class 1 Analytical + Class 4 Invariant)
// =====================================================================================
//
// ORACLE PROVENANCE
// -----------------
// Every expected number below was derived by hand from the ALGORITHM SOURCE, before
// the filter was ever run, and independently cross-checked at 80 significant digits by
// ww_work/ComputeSchmids/oracle.py (mpmath). No value here was read back from a filter
// run or from a legacy DREAM3D run.
//
// The chain being modelled is:
//
//   1. simplnx ComputeSchmids.cpp:
//        sampleLoading  = normalize(LoadingDirection)
//        om             = QuaternionD(qx, qy, qz, qw).toOrientationMatrix()
//        crystalLoading = om * sampleLoading
//        Poles[3i+k]    = static_cast<int32>(crystalLoading[k] * 100.0)   // TRUNCATES toward zero
//
//   2. EbsdLib Quaternion<T>::toOrientationMatrix() (row-major, epsijk = +1 so NO transpose;
//      OrientationFwd.hpp defines DREAM3D_PASSIVE_ROTATION => epsijkd == 1.0):
//        qq = w*w - (x*x + y*y + z*z)
//        om = [ qq+2x*x,      2(xy - wz),  2(xz + wy) ]
//             [ 2(yx + wz),  qq+2y*y,      2(yz - wx) ]
//             [ 2(zx - wy),   2(zy + wx),  qq+2z*z    ]
//
//   3. EbsdLib CubicOps::getSchmidFactorAndSS(load, m, angleComps, slipsys) -- the AUTO path.
//      With L = crystalLoading and mag = |L|:
//        theta1..4 = |Lx+Ly+Lz|, |Lx+Ly-Lz|, |Lx-Ly+Lz|, |-Lx+Ly+Lz|  all / (mag * sqrt(3))
//        lambda1..6 = |Lx+Ly|, |Lx+Lz|, |Lx-Ly|, |Lx-Lz|, |Ly+Lz|, |Ly-Lz| all / (mag * sqrt(2))
//      theta_i is cos(phi) against a {111} plane normal; lambda_j is cos(lambda) against a
//      <110> slip direction. The twelve FCC systems are enumerated, in source order, as
//      (theta, lambda) index pairs:
//        idx: 0        1        2        3        4        5
//             t1*l6    t1*l4    t1*l3    t2*l3    t2*l2    t2*l5
//        idx: 6        7        8        9        10       11
//             t3*l1    t3*l5    t3*l4    t4*l1    t4*l2    t4*l6
//      slipsys is seeded to 0 with schmidfactor = idx0 and each later index is taken only on a
//      STRICT '>' -- so ties resolve to the LOWEST index. angleComps carries the (theta, lambda)
//      of the winner, i.e. COSINES on this path (see the SC-3 note on the override path).
//
// SC-2 NOTE (EbsdLib <= 3.1.0): the sqrt(3)/sqrt(2) normalizers above were the float literals
// 1.732f / 1.414f, which inflate every Schmid factor by the uniform factor
//   sqrt(6) / (1.732f * 1.414f) = 1.00018035284   (+0.0180353 %)
// and let m exceed the physical cubic maximum of 0.5 (0.500090176 at the maximizing
// direction). Fixed on EbsdLib topic/3_1_1_staging; the values asserted below are the
// EXACT-arithmetic post-fix values and will fail by ~7.4e-5 against EbsdLib <= 3.1.0.
namespace
{
constexpr float64 k_Tol = 1.0e-6;

const std::string k_GeomName("DataContainer");
const std::string k_FeatureAMName("Cell Feature Data");
const std::string k_EnsembleAMName("Cell Ensemble Data");
const std::string k_PhasesName("Phases");
const std::string k_AvgQuatsName("AvgQuats");
const std::string k_CrystalStructuresName("CrystalStructures");

const DataPath k_FeatureAMPath({k_GeomName, k_FeatureAMName});
const DataPath k_EnsembleAMPath({k_GeomName, k_EnsembleAMName});

/**
 * @brief One hand-built fixture: N features (feature 0 is the conventional sentinel) and
 * numEnsembles ensemble slots. Ensemble 0 is the 999 "unknown" sentinel, ensemble 1 is
 * Cubic_High. Every feature defaults to phase 1 with the identity quaternion.
 */
struct SchmidFixture
{
  DataStructure ds;
  Int32Array* featurePhases = nullptr;
  Float32Array* avgQuats = nullptr;
  UInt32Array* crystalStructures = nullptr;
};

SchmidFixture MakeFixture(usize numFeatures, usize numEnsembles = 2)
{
  SchmidFixture td;
  auto* imageGeom = ImageGeom::Create(td.ds, k_GeomName);
  imageGeom->setDimensions({1, 1, 1});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  auto* featureAM = AttributeMatrix::Create(td.ds, k_FeatureAMName, ShapeType{numFeatures}, imageGeom->getId());
  auto* ensembleAM = AttributeMatrix::Create(td.ds, k_EnsembleAMName, ShapeType{numEnsembles}, imageGeom->getId());

  td.featurePhases = UnitTest::CreateTestDataArray<int32>(td.ds, k_PhasesName, {numFeatures}, {1}, featureAM->getId());
  td.avgQuats = UnitTest::CreateTestDataArray<float32>(td.ds, k_AvgQuatsName, {numFeatures}, {4}, featureAM->getId());
  td.crystalStructures = UnitTest::CreateTestDataArray<uint32>(td.ds, k_CrystalStructuresName, {numEnsembles}, {1}, ensembleAM->getId());

  for(usize i = 0; i < numFeatures; ++i)
  {
    (*td.featurePhases)[i] = 1;
    (*td.avgQuats)[i * 4 + 0] = 0.0F; // x
    (*td.avgQuats)[i * 4 + 1] = 0.0F; // y
    (*td.avgQuats)[i * 4 + 2] = 0.0F; // z
    (*td.avgQuats)[i * 4 + 3] = 1.0F; // w  -> identity
  }
  (*td.featurePhases)[0] = 0; // feature 0 is the sentinel; the algorithm starts its loop at 1
  for(usize e = 0; e < numEnsembles; ++e)
  {
    (*td.crystalStructures)[e] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  }
  if(numEnsembles > 1)
  {
    (*td.crystalStructures)[1] = ebsdlib::CrystalStructure::Cubic_High; // EbsdLib LaueOps index 1
  }
  return td;
}

void SetQuat(SchmidFixture& td, usize featureIdx, const std::array<float32, 4>& q)
{
  (*td.avgQuats)[featureIdx * 4 + 0] = q[0];
  (*td.avgQuats)[featureIdx * 4 + 1] = q[1];
  (*td.avgQuats)[featureIdx * 4 + 2] = q[2];
  (*td.avgQuats)[featureIdx * 4 + 3] = q[3];
}

Arguments MakeArgs(const std::vector<float32>& loading, bool storeAngleComps, bool overrideSystem, const std::vector<float32>& plane = {0.0F, 0.0F, 1.0F},
                   const std::vector<float32>& direction = {1.0F, 0.0F, 0.0F})
{
  Arguments args;
  args.insertOrAssign(ComputeSchmidsFilter::k_LoadingDirection_Key, std::make_any<VectorFloat32Parameter::ValueType>(loading));
  args.insertOrAssign(ComputeSchmidsFilter::k_StoreAngleComponents_Key, std::make_any<bool>(storeAngleComps));
  args.insertOrAssign(ComputeSchmidsFilter::k_OverrideSystem_Key, std::make_any<bool>(overrideSystem));
  args.insertOrAssign(ComputeSchmidsFilter::k_SlipPlane_Key, std::make_any<VectorFloat32Parameter::ValueType>(plane));
  args.insertOrAssign(ComputeSchmidsFilter::k_SlipDirection_Key, std::make_any<VectorFloat32Parameter::ValueType>(direction));
  args.insertOrAssign(ComputeSchmidsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeatureAMPath.createChildPath(k_PhasesName)));
  args.insertOrAssign(ComputeSchmidsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_FeatureAMPath.createChildPath(k_AvgQuatsName)));
  args.insertOrAssign(ComputeSchmidsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_EnsembleAMPath.createChildPath(k_CrystalStructuresName)));
  args.insertOrAssign(ComputeSchmidsFilter::k_SchmidsArrayName_Key, std::make_any<std::string>(k_SchmidsArrayName));
  args.insertOrAssign(ComputeSchmidsFilter::k_SlipSystemsArrayName_Key, std::make_any<std::string>(k_SlipSystemsArrayName));
  args.insertOrAssign(ComputeSchmidsFilter::k_PolesArrayName_Key, std::make_any<std::string>(k_PolesArrayName));
  args.insertOrAssign(ComputeSchmidsFilter::k_PhisArrayName_Key, std::make_any<std::string>(k_PhisArrayName));
  args.insertOrAssign(ComputeSchmidsFilter::k_LambdasArrayName_Key, std::make_any<std::string>(k_LambdasArrayName));
  return args;
}

/**
 * @brief Asserts the five outputs of one feature against the oracle. Angle components are
 * checked only when phis/lambdas are >= 0 (a negative sentinel means "do not check").
 */
void CheckFeature(const DataStructure& ds, usize featureIdx, float64 expectedM, int32 expectedSlipSystem, const std::array<int32, 3>& expectedPoles, float64 expectedPhi, float64 expectedLambda)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_SchmidsArrayName)));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_SlipSystemsArrayName)));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_PolesArrayName)));
  const auto& schmids = ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_SchmidsArrayName));
  const auto& slipSystems = ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_SlipSystemsArrayName));
  const auto& poles = ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_PolesArrayName));

  CAPTURE(featureIdx);
  CHECK(static_cast<float64>(schmids[featureIdx]) == Approx(expectedM).margin(k_Tol));
  CHECK(slipSystems[featureIdx] == expectedSlipSystem);
  for(usize k = 0; k < 3; ++k)
  {
    CAPTURE(k);
    CHECK(poles[featureIdx * 3 + k] == expectedPoles[k]);
  }

  if(expectedPhi >= 0.0)
  {
    REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_PhisArrayName)));
    REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_LambdasArrayName)));
    const auto& phis = ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_PhisArrayName));
    const auto& lambdas = ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_LambdasArrayName));
    CHECK(static_cast<float64>(phis[featureIdx]) == Approx(expectedPhi).margin(k_Tol));
    CHECK(static_cast<float64>(lambdas[featureIdx]) == Approx(expectedLambda).margin(k_Tol));
  }
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeSchmidsFilter", "[OrientationAnalysis][ComputeSchmidsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);
  DataPath cellFeatureDataPath({k_DataContainer, k_CellFeatureData});
  DataPath avgQuatsDataPath = cellFeatureDataPath.createChildPath(k_AvgQuats);
  DataPath featurePhasesDataPath = cellFeatureDataPath.createChildPath(k_Phases);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeSchmidsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeSchmidsFilter::k_LoadingDirection_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 1.0F, 1.0F}));
    args.insertOrAssign(ComputeSchmidsFilter::k_StoreAngleComponents_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeSchmidsFilter::k_OverrideSystem_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeSchmidsFilter::k_SlipPlane_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.0F, 0.0F, 1.0F}));
    args.insertOrAssign(ComputeSchmidsFilter::k_SlipDirection_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 0.0F, 0.0F}));

    args.insertOrAssign(ComputeSchmidsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesDataPath));
    args.insertOrAssign(ComputeSchmidsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsDataPath));
    args.insertOrAssign(ComputeSchmidsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(ComputeSchmidsFilter::k_SchmidsArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_SchmidsArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_SlipSystemsArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_SlipSystemsArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_PolesArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_PolesArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_PhisArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_PhisArrayName));
    args.insertOrAssign(ComputeSchmidsFilter::k_LambdasArrayName_Key, std::make_any<std::string>(k_CalculatedArrayPrefix + k_LambdasArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // The two INTEGER outputs must still match the archive EXACTLY. Both are provably invariant
  // under the EbsdLib 3.1.1 normalizer fix:
  //   * SlipSystems is an argmax over twelve candidates that the fix rescales by a single positive
  //     factor, so the ranking -- and therefore the winner -- cannot move.
  //   * Poles is derived from crystalLoading, which the fix does not touch at all.
  // Keeping these exact is the check that the fix changed only what it was supposed to.
  {
    const std::vector<std::string> comparisonNames = {k_SlipSystemsArrayName, k_PolesArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_CalculatedArrayPrefix + comparisonName});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<int32>(exemplarData, calculatedData, 1);
    }
  }

  // The three FLOAT outputs no longer match the archive, and they are not supposed to. The archive
  // was generated with EbsdLib <= 3.1.0, whose CubicOps::getSchmidFactorAndSS normalized with the
  // float literals 1.732f and 1.414f instead of sqrt(3) and sqrt(2). Both literals differ from the
  // constants they stand in for, so every archived value carries a KNOWN, UNIFORM, per-component
  // inflation factor:
  //
  //   Phis    (cos phi,    normalized by sqrt(3)):  sqrt(3) / 1.732f          = 1.0000293385
  //   Lambdas (cos lambda, normalized by sqrt(2)):  sqrt(2) / 1.414f          = 1.0001510099
  //   Schmids (their product)                    :  sqrt(6) / (1.732f*1.414f) = 1.0001803528
  //
  // Rather than retire the comparison -- which would throw away 214 real regression checks -- or
  // loosen it into meaninglessness, assert the RELATIONSHIP. Every archived value must equal the
  // freshly computed value times its predicted factor. That is a strictly STRONGER statement than
  // the equality it replaces: it pins both the new values and the exact size and shape of the
  // change, and it fails if the fix perturbed anything the bias analysis did not account for.
  //
  // Recompute these expectations (and regenerate the archive) if the exemplar is ever rebuilt
  // against EbsdLib >= 3.1.1, at which point the factors all become 1.0.
  {
    const std::vector<std::pair<std::string, float64>> biasedComparisons = {
        {k_SchmidsArrayName, 1.0001803528351113},
        {k_PhisArrayName, 1.0000293384785181},
        {k_LambdasArrayName, 1.0001510099261918},
    };
    for(const auto& [comparisonName, biasFactor] : biasedComparisons)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_CalculatedArrayPrefix + comparisonName});
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(exemplarPath));
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(calculatedPath));
      const auto& exemplarData = dataStructure.getDataRefAs<Float32Array>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<Float32Array>(calculatedPath);
      REQUIRE(exemplarData.getSize() == calculatedData.getSize());

      usize mismatches = 0;
      for(usize i = 1; i < calculatedData.getSize(); i++)
      {
        const float64 expected = static_cast<float64>(calculatedData[i]) * biasFactor;
        const float64 archived = static_cast<float64>(exemplarData[i]);
        // Absolute margin covers the float32 quantization of both operands near zero; the relative
        // epsilon covers it away from zero.
        if(archived != Approx(expected).margin(2.0e-7).epsilon(1.0e-6))
        {
          CAPTURE(comparisonName, i, archived, expected, biasFactor);
          CHECK(archived == Approx(expected).margin(2.0e-7).epsilon(1.0e-6));
          if(++mismatches >= 5)
          {
            break;
          }
        }
      }
      CAPTURE(comparisonName);
      CHECK(mismatches == 0);
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_schmids.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Class 1 (Analytical): auto slip-system path, identity quaternion, Cubic_High.
//
// om == identity, so crystalLoading == normalize(LoadingDirection) EXACTLY (the quaternion
// (0,0,0,1) gives qq == 1 and every off-diagonal term is an exact zero).
//
// [0,0,1]  L = (0,0,1)
//   theta1..4 = 1/sqrt(3) each        (|0+0+1| = |0+0-1| = |0-0+1| = |-0+0+1| = 1)
//   lambda1 = 0, lambda2 = 1/sqrt(2), lambda3 = 0, lambda4..6 = 1/sqrt(2)
//   schmid = [1/sqrt6, 1/sqrt6, 0, 0, 1/sqrt6, 1/sqrt6, 0, 1/sqrt6, 1/sqrt6, 0, 1/sqrt6, 1/sqrt6]
//   EIGHT-way tie at 1/sqrt(6) -- indices 0, 1, 4, 5, 7, 8, 10, 11; STRICT '>' keeps index 0.
//   m = 1/sqrt(6) = 0.4082482904638630, cos(phi) = theta1 = 1/sqrt(3), cos(lambda) = lambda6 = 1/sqrt(2)
//   Poles = trunc(100 * (0,0,1)) = (0, 0, 100)
//
// [1,1,1]  L = (1,1,1)/sqrt(3), so Lx == Ly == Lz == s
//   theta1 = 3s/sqrt3 = 1 ; theta2 = theta3 = theta4 = s/sqrt3 = 1/3 (bit-identical: s+s-s == s)
//   lambda1 = lambda2 = lambda5 = 2s/sqrt2 = 2/sqrt6 ; lambda3 = lambda4 = lambda6 = 0
//   schmid = [0,0,0,0, 2/(3 sqrt6) x6 at idx 4,5,6,7 and 9,10, 0 elsewhere]  -- a SIX-way tie.
//   first non-zero maximum is index 4 (theta2*lambda2).
//   m = 2/(3*sqrt(6)) = 0.2721655269759087, cos(phi) = 1/3, cos(lambda) = 2/sqrt(6) = 0.8164965809277260
//   Poles = trunc(100 * 0.5773502691896258) = (57, 57, 57)
//
// [0,1,1]  L = (0,1,1)/sqrt(2), Lx = 0, Ly == Lz == u
//   theta1 = theta4 = 2u/sqrt3 = 2/sqrt6 ; theta2 = theta3 = 0
//   lambda1..4 = 0.5 ; lambda5 = 1 ; lambda6 = 0
//   schmid[0] = 0 (theta1*lambda6); schmid[1] = (2/sqrt6)(1/2) = 1/sqrt6 -> first strict win at index 1.
//   indices 2, 9, 10 tie at 1/sqrt6 and are rejected by the strict '>'.
//   m = 1/sqrt(6) = 0.4082482904638630, cos(phi) = 2/sqrt(6) = 0.8164965809277260, cos(lambda) = 0.5
//   This fixture is the one that SEPARATES Phis from Lambdas (0.8165 vs 0.5).
//   Poles = trunc(100 * 0.7071067811865476) = (0, 70, 70)
//
// [1,2,3]  L = (1,2,3)/sqrt(14) -- the enumeration-order pin: a UNIQUE maximum, no ties.
//   theta1..4 = 6, 0, 2, 4 (all / sqrt(42)); lambda1..6 = 3, 4, 1, 2, 5, 1 (all / sqrt(28))
//   schmid * sqrt(1176) = [6, 12, 6, 0, 0, 0, 6, 10, 4, 12, 16, 4]  -> unique max 16 at index 10.
//   m = 16/sqrt(1176) = 8/sqrt(294) = 0.4665694748158435
//   cos(phi) = 4/sqrt(42) = 0.6172133998483701, cos(lambda) = 4/sqrt(28) = 0.7559289460184544
//   Poles = trunc(100 * (0.2672612419, 0.5345224838, 0.8017837257)) = (26, 53, 80)
//   Every component is far from an integer boundary, so the truncation is exact, not marginal.
//
// [3,6,9] is [1,2,3] scaled by 3: the filter normalizes the loading direction, so every
// output must be identical (scale invariance, Class 4).
// -----------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ComputeSchmidsFilter: Class 1 analytical oracle, auto slip system", "[OrientationAnalysis][ComputeSchmidsFilter][VV]")
{
  UnitTest::LoadPlugins();

  struct OracleRow
  {
    const char* label;
    std::vector<float32> loading;
    float64 m;
    int32 slipSystem;
    std::array<int32, 3> poles;
    float64 cosPhi;
    float64 cosLambda;
  };

  // clang-format off
  const std::vector<OracleRow> rows = {
      {"[0,0,1]", {0.0F, 0.0F, 1.0F}, 0.4082482904638630, 0,  {0, 0, 100},  0.5773502691896258, 0.7071067811865476},
      {"[1,1,1]", {1.0F, 1.0F, 1.0F}, 0.2721655269759087, 4,  {57, 57, 57}, 0.3333333333333333, 0.8164965809277260},
      {"[0,1,1]", {0.0F, 1.0F, 1.0F}, 0.4082482904638630, 1,  {0, 70, 70},  0.8164965809277260, 0.5000000000000000},
      {"[1,2,3]", {1.0F, 2.0F, 3.0F}, 0.4665694748158435, 10, {26, 53, 80}, 0.6172133998483701, 0.7559289460184544},
      {"[3,6,9]", {3.0F, 6.0F, 9.0F}, 0.4665694748158435, 10, {26, 53, 80}, 0.6172133998483701, 0.7559289460184544},
  };
  // clang-format on

  for(const auto& row : rows)
  {
    DYNAMIC_SECTION("loading " << row.label)
    {
      SchmidFixture td = MakeFixture(2);

      ComputeSchmidsFilter filter;
      Arguments args = MakeArgs(row.loading, true, false);

      auto preflightResult = filter.preflight(td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      CheckFeature(td.ds, 1, row.m, row.slipSystem, row.poles, row.cosPhi, row.cosLambda);

      // Class 4 invariants.
      const auto& schmids = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_SchmidsArrayName));
      const auto& phis = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_PhisArrayName));
      const auto& lambdas = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_LambdasArrayName));
      // (a) The Schmid factor of a cubic crystal cannot exceed 0.5 and cannot be negative.
      //     EbsdLib <= 3.1.0 violated the upper bound (max 0.500090176); this assertion is only
      //     satisfiable against the fixed normalizers.
      CHECK(schmids[1] >= 0.0F);
      CHECK(schmids[1] <= 0.5F);
      // (b) On the auto path angleComps are COSINES, so m must be their product exactly.
      CHECK(static_cast<float64>(schmids[1]) == Approx(static_cast<float64>(phis[1]) * static_cast<float64>(lambdas[1])).margin(k_Tol));
      // (c) Both cosines are direction cosines: in [0, 1].
      CHECK(phis[1] >= 0.0F);
      CHECK(phis[1] <= 1.0F);
      CHECK(lambdas[1] >= 0.0F);
      CHECK(lambdas[1] <= 1.0F);

      // Feature 0 is the sentinel slot; the algorithm writes explicit zeros there.
      CheckFeature(td.ds, 0, 0.0, 0, {0, 0, 0}, 0.0, 0.0);

      UnitTest::CheckArraysInheritTupleDims(td.ds);
    }
  }
}

// -----------------------------------------------------------------------------
// Class 1 + Class 4: the quaternion -> orientation-matrix convention, and equivariance.
//
// All three quaternions below are EXACTLY representable in float32 and produce an EXACT
// orientation matrix (every entry is 0 or +/-1), so there is no floating-point slack in the
// expected Poles. Loading is [1,2,3] for all three.
//
//   q = (0,0,0,1)          identity          om = I
//                          crystalLoading = ( 1, 2, 3)/sqrt(14) -> SS 10, Poles ( 26,  53, 80)
//
//   q = (0,0,1,0)          180 deg about Z   qq = -1  ->  om = diag(-1,-1,1)
//                          crystalLoading = (-1,-2, 3)/sqrt(14)
//                          theta1..4 = 0, 6, 4, 2 ; lambda1..6 = 3, 2, 1, 4, 1, 5 (over sqrt(28))
//                          schmid*sqrt(1176) = [0,0,0, 6,12,6, 12,4,16, 6,4,10] -> unique max 16 at idx 8
//                          -> SS 8, Poles (-26, -53, 80).  NEGATIVE poles: this fixture is what
//                          pins truncation-toward-zero (a round() would give -27 and -53).
//
//   q = (.5,.5,.5,.5)      120 deg about [111]; qq = -0.5
//                          om = [[0,0,1],[1,0,0],[0,1,0]]  (an ASYMMETRIC permutation matrix)
//                          crystalLoading = ( 3, 1, 2)/sqrt(14)
//                          theta1..4 = 6, 2, 4, 0 ; lambda1..6 = 4, 5, 2, 1, 3, 1 (over sqrt(28))
//                          schmid*sqrt(1176) = [6,6,12, 4,10,6, 16,12,4, 0,0,0] -> unique max 16 at idx 6
//                          -> SS 6, Poles (80, 26, 53).
//                          Transposing om would give crystalLoading = (2,3,1)/sqrt(14), hence
//                          SS 5 and Poles (53, 80, 26) -- so this row DISCRIMINATES the transpose.
//
//   q = (1,0,0,0)          180 deg about X   qq = -1  ->  om = diag(1,-1,-1)
//                          crystalLoading = ( 1,-2,-3)/sqrt(14)
//                          theta1..4 = 4, 2, 0, 6 ; lambda1..6 = 1, 2, 3, 4, 5, 1 (over sqrt(28))
//                          schmid*sqrt(1176) = [4,16,12, 6,4,10, 0,0,0, 6,12,6] -> unique max 16 at idx 1
//                          -> SS 1, Poles (26, -53, -80).
//                          The three rows above all have quaternion x == y, so none of them notices a
//                          swapped x/y read -- a gap the mutation run found. This row closes it: swap
//                          x and y and the quaternion becomes (0,1,0,0), a 180 deg rotation about Y,
//                          giving om = diag(-1,1,-1), SS 4 and Poles (-26, 53, -80).
//
// All three rotations are cubic symmetry operations, so the Schmid factor and both angle
// components must be IDENTICAL across the three (Class 4 symmetry invariance) even though the
// winning slip-system index and the Poles differ.
// -----------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ComputeSchmidsFilter: orientation-matrix convention and equivariance", "[OrientationAnalysis][ComputeSchmidsFilter][VV]")
{
  UnitTest::LoadPlugins();

  struct QuatRow
  {
    const char* label;
    std::array<float32, 4> quat; // (x, y, z, w)
    int32 slipSystem;
    std::array<int32, 3> poles;
  };

  const float64 k_ExpectedM = 0.4665694748158435;      // 8/sqrt(294)
  const float64 k_ExpectedPhi = 0.6172133998483701;    // 4/sqrt(42)
  const float64 k_ExpectedLambda = 0.7559289460184544; // 4/sqrt(28)

  const std::vector<QuatRow> rows = {
      {"identity (0,0,0,1)", {0.0F, 0.0F, 0.0F, 1.0F}, 10, {26, 53, 80}},
      {"180 deg about Z (0,0,1,0)", {0.0F, 0.0F, 1.0F, 0.0F}, 8, {-26, -53, 80}},
      {"120 deg about [111] (.5,.5,.5,.5)", {0.5F, 0.5F, 0.5F, 0.5F}, 6, {80, 26, 53}},
      {"180 deg about X (1,0,0,0)", {1.0F, 0.0F, 0.0F, 0.0F}, 1, {26, -53, -80}},
  };

  for(const auto& row : rows)
  {
    DYNAMIC_SECTION("quat " << row.label)
    {
      SchmidFixture td = MakeFixture(2);
      SetQuat(td, 1, row.quat);

      ComputeSchmidsFilter filter;
      Arguments args = MakeArgs({1.0F, 2.0F, 3.0F}, true, false);

      auto preflightResult = filter.preflight(td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      CheckFeature(td.ds, 1, k_ExpectedM, row.slipSystem, row.poles, k_ExpectedPhi, k_ExpectedLambda);

      const auto& schmids = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_SchmidsArrayName));
      CHECK(schmids[1] <= 0.5F);

      UnitTest::CheckArraysInheritTupleDims(td.ds);
    }
  }
}

// -----------------------------------------------------------------------------
// Class 1: the OVERRIDE slip-system path -- a DIFFERENT EbsdLib overload with different
// semantics for BOTH the slip-system index and the angle components.
//
// CubicOps::getSchmidFactorAndSS(load, plane, direction, m, angleComps, slipsys) loops over the
// 24 CubicHigh symmetry operators. For symmetry operator i:
//    slipPlane     = MatSym[i] * plane        (== column 3 of MatSym[i] when plane == (0,0,1))
//    slipDirection = MatSym[i] * direction    (== column 1 of MatSym[i] when direction == (1,0,0))
// operators whose slipPlane[2] < 0 are skipped (duplicate-avoidance), then
//    m_i = |L . slipPlane| * |L . slipDirection|
// and slipsys becomes the SYMMETRY-OPERATOR INDEX (0..23) of the strict maximum -- NOT the
// 0..11 FCC slip-system index of the auto path.
//
// SC-3 (documented deviation): angleComps on THIS path are acos() results, i.e. RADIANS,
// whereas the auto path writes raw COSINES into the same two output arrays. The Phis/Lambdas
// arrays therefore change UNITS with the OverrideSystem toggle.
//
// Every cubic symmetry operator is a signed permutation matrix, so with plane == (0,0,1) and
// direction == (1,0,0) the pair (slipPlane, slipDirection) is always a pair of DISTINCT signed
// basis vectors, and m_i = |L_a| * |L_b| for some a != b.
//
//   L = (1,2,3)/sqrt(14): the maximum of |L_a||L_b| over a != b is (3/sqrt14)(2/sqrt14) = 6/14 = 3/7.
//     SIX operators of the twenty-four attain that maximum -- {3, 6, 8, 9, 11, 14} -- so the
//     asserted index is ENUMERATION-ORDER DEPENDENT: the strict '>' keeps the first, operator 3 =
//     [[0,-1,0],[1,0,0],[0,0,1]], whose column 3 = (0,0,1) (z >= 0, kept) and column 1 = (0,1,0),
//     giving cos(phi) = 3/sqrt(14) and cos(lambda) = 2/sqrt(14). The A/B run confirmed that
//     DREAM3D 6.5.171, whose symmetry table holds the same group in a different order, reports 8
//     for the identical physical answer. This assertion therefore pins EbsdLib's table order, not
//     a portable physical label.
//     m = 3/7 = 0.4285714285714286 ; Phis = acos(3/sqrt14) = 0.6405223126794245 rad (36.699 deg)
//                                    Lambdas = acos(2/sqrt14) = 1.0068536854342678 rad (57.688 deg)
//     The two acos values are NOT closed-form rationals or surds; they were recomputed to 60
//     significant digits as atan(sqrt(5)/3) = 0.640522312679424574143... and
//     atan(sqrt(10)/2) = 1.006853685434267776537... and rounded to the nearest double. They are
//     asserted at k_Tol = 1e-6, which is far looser than that rounding.
//   L = (1,1,1)/sqrt(3): all |L_a| equal, so every kept operator gives 1/3; the strict '>' keeps
//     the FIRST, operator 0 (the identity). m = 1/3, both angles = acos(1/sqrt3) = 0.9553166181245093 rad.
//   L = (0,0,1): slipPlane and slipDirection are distinct basis vectors, so at most one of them
//     can be +/-Z; the product is 0 for ALL 24 operators. The strict '>' is never taken, so the
//     function returns its initialized state: m = 0, slipsys = 0, angleComps = (0,0). This is the
//     "no slip system found" degenerate case.
// -----------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ComputeSchmidsFilter: override slip system path", "[OrientationAnalysis][ComputeSchmidsFilter][VV]")
{
  UnitTest::LoadPlugins();

  struct OverrideRow
  {
    const char* label;
    std::vector<float32> loading;
    float64 m;
    int32 symOpIndex;
    std::array<int32, 3> poles;
    float64 phiRadians;
    float64 lambdaRadians;
  };

  // clang-format off
  const std::vector<OverrideRow> rows = {
      {"[1,2,3]", {1.0F, 2.0F, 3.0F}, 0.4285714285714286, 3, {26, 53, 80},  0.6405223126794245, 1.0068536854342678},
      {"[1,1,1]", {1.0F, 1.0F, 1.0F}, 0.3333333333333333, 0, {57, 57, 57},  0.9553166181245093, 0.9553166181245093},
      {"[0,0,1]", {0.0F, 0.0F, 1.0F}, 0.0000000000000000, 0, {0, 0, 100},   0.0000000000000000, 0.0000000000000000},
  };
  // clang-format on

  for(const auto& row : rows)
  {
    DYNAMIC_SECTION("loading " << row.label)
    {
      SchmidFixture td = MakeFixture(2);

      ComputeSchmidsFilter filter;
      Arguments args = MakeArgs(row.loading, true, true, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F});

      auto preflightResult = filter.preflight(td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(td.ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      CheckFeature(td.ds, 1, row.m, row.symOpIndex, row.poles, row.phiRadians, row.lambdaRadians);

      // Class 4: on the override path m must equal cos(Phis) * cos(Lambdas) because the stored
      // components are the ANGLES, not their cosines. This is the assertion that proves the SC-3
      // unit flip is real rather than a documentation misreading.
      const auto& schmids = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_SchmidsArrayName));
      const auto& phis = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_PhisArrayName));
      const auto& lambdas = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(k_LambdasArrayName));
      if(row.m > 0.0)
      {
        CHECK(static_cast<float64>(schmids[1]) == Approx(std::cos(static_cast<float64>(phis[1])) * std::cos(static_cast<float64>(lambdas[1]))).margin(k_Tol));
      }
      CHECK(schmids[1] <= 0.5F);

      UnitTest::CheckArraysInheritTupleDims(td.ds);
    }
  }

  SECTION("preflight rejects a slip direction that is not in the slip plane")
  {
    SchmidFixture td = MakeFixture(2);
    ComputeSchmidsFilter filter;
    // (0,0,1) . (1,0,1) == 1 != 0
    Arguments args = MakeArgs({1.0F, 2.0F, 3.0F}, true, true, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 1.0F});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    CHECK(preflightResult.outputActions.errors()[0].code == -13500);
  }
}

// -----------------------------------------------------------------------------
// Option and guard coverage.
//
// SC-1: the five output arrays were created by CreateArrayAction with an EMPTY fill value, where
// legacy SIMPL FindSchmids passed initValue 0. Any feature the loop skips (`laueClass >=
// CrystalStructure::LaueGroupEnd`, ComputeSchmids.cpp:83) is never written, so those rows depend
// entirely on how the store was initialized. The fixture below is large and dirties the heap
// first, which is what settled how much of that exposure is real -- see the note inside it.
//
// SC-4: `crystalStructures[featurePhases[i]]` (ComputeSchmids.cpp:82) is unbounded. A phase id
// beyond the ensemble count, or a negative one, reads out of bounds.
// -----------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ComputeSchmidsFilter: options, skip path and phase guards", "[OrientationAnalysis][ComputeSchmidsFilter][VV]")
{
  UnitTest::LoadPlugins();

  SECTION("StoreAngleComponents == false does not create Phis/Lambdas")
  {
    SchmidFixture td = MakeFixture(2);
    ComputeSchmidsFilter filter;
    Arguments args = MakeArgs({1.0F, 2.0F, 3.0F}, false, false);

    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // The three unconditional outputs are unchanged by the toggle.
    CheckFeature(td.ds, 1, 0.4665694748158435, 10, {26, 53, 80}, -1.0, -1.0);
    CHECK(td.ds.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_PhisArrayName)) == nullptr);
    CHECK(td.ds.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_LambdasArrayName)) == nullptr);

    UnitTest::CheckArraysInheritTupleDims(td.ds);
  }

  SECTION("features whose Laue class is beyond LaueGroupEnd get defined zeros, not stale memory")
  {
    // Ensemble slots: 0 = 999 sentinel, 1 = Cubic_High, 2 = 999 sentinel (>= LaueGroupEnd == 11).
    // Feature 1 uses phase 1 (computed); every other feature uses phase 2 (skipped by the
    // continue), so all of their output rows are ones the algorithm never writes.
    //
    // WHY THE FIXTURE IS LARGE, AND WHAT THAT SETTLED. The claim SC-1 originally rested on was
    // that without an explicit fill the skipped rows hold indeterminate memory. Removing the
    // explicit "0" fill from all five CreateArrayAction calls and running THIS fixture -- 20000
    // features, so every output array is 78 KB or more, with same-size blocks poisoned to 0xAB
    // and freed immediately before the run -- still yields all zeros. That is not the allocator
    // being kind: a standalone probe confirms that on this platform `new float[20000]` after a
    // same-size dirty free does return 0xABABABAB, whereas the 8-to-36-byte allocations a
    // three-feature fixture uses come from libmalloc's nano zone as freshly zeroed pages. So the
    // zeros here are written by SIMPLNX, not inherited from the heap, and the mechanism is in the
    // store factory rather than in the action: CoreDataIOManager::addDataStoreFnc() constructs
    // every in-core store with `static_cast<T>(0)` as its initValue, and DataStore's constructor
    // unconditionally `std::fill_n`s the buffer with it (DataStore.hpp:66-69). The fillValue
    // string on CreateArrayAction only adds a second, redundant fill.
    //
    // The explicit fill is therefore DEFENSIVE, not a repair of a live in-core defect: it
    // restores what legacy SIMPL FindSchmids stated explicitly, and it is the only guarantee for
    // any store implementation whose factory does not hard-code a zero initValue. This section
    // is the regression lock on the guarantee, and the large dirty-heap fixture is what makes it
    // a non-vacuous one -- at this size the assertion would catch a store that stopped filling.
    constexpr usize k_FeatureCount = 20000;
    SchmidFixture td = MakeFixture(k_FeatureCount, 3);
    for(usize i = 2; i < k_FeatureCount; ++i)
    {
      (*td.featurePhases)[i] = 2;
    }

    ComputeSchmidsFilter filter;
    Arguments args = MakeArgs({1.0F, 2.0F, 3.0F}, true, false);
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Dirty the heap with blocks the same size as the five output arrays and free them
    // immediately before the run, so that a default-initialized `new value_type[n]` of that size
    // is served from a recycled block that still holds the poison.
    {
      std::vector<char*> dirtyBlocks;
      for(int32 repetition = 0; repetition < 8; ++repetition)
      {
        for(usize byteCount : {k_FeatureCount * sizeof(float32), k_FeatureCount * 3 * sizeof(int32)})
        {
          char* block = new char[byteCount];
          std::memset(block, 0xAB, byteCount);
          dirtyBlocks.push_back(block);
        }
      }
      for(char* block : dirtyBlocks)
      {
        delete[] block;
      }
    }

    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Feature 1 still gets the full oracle answer.
    CheckFeature(td.ds, 1, 0.4665694748158435, 10, {26, 53, 80}, 0.6172133998483701, 0.7559289460184544);
    // Feature 2 was skipped: every output must be a defined zero.
    CheckFeature(td.ds, 2, 0.0, 0, {0, 0, 0}, 0.0, 0.0);

    // ...and so must every other row the algorithm never writes: feature 0 (the sentinel) and
    // features 2 through 19999. Aggregated into one CHECK per data type so the section stays a
    // handful of assertions instead of a hundred thousand.
    usize nonZeroFloatValues = 0;
    for(const auto& arrayName : {k_SchmidsArrayName, k_PhisArrayName, k_LambdasArrayName})
    {
      REQUIRE_NOTHROW(td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(arrayName)));
      const auto& arrayRef = td.ds.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath(arrayName));
      REQUIRE(arrayRef.getSize() == k_FeatureCount);
      for(usize i = 0; i < k_FeatureCount; ++i)
      {
        if(i != 1 && arrayRef[i] != 0.0F)
        {
          ++nonZeroFloatValues;
        }
      }
    }
    CHECK(nonZeroFloatValues == 0);

    usize nonZeroSlipSystemValues = 0;
    {
      REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_SlipSystemsArrayName)));
      const auto& slipSystemsRef = td.ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_SlipSystemsArrayName));
      REQUIRE(slipSystemsRef.getSize() == k_FeatureCount);
      for(usize i = 0; i < k_FeatureCount; ++i)
      {
        if(i != 1 && slipSystemsRef[i] != 0)
        {
          ++nonZeroSlipSystemValues;
        }
      }
    }
    CHECK(nonZeroSlipSystemValues == 0);

    usize nonZeroPoleValues = 0;
    {
      REQUIRE_NOTHROW(td.ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_PolesArrayName)));
      const auto& polesRef = td.ds.getDataRefAs<Int32Array>(k_FeatureAMPath.createChildPath(k_PolesArrayName));
      REQUIRE(polesRef.getSize() == k_FeatureCount * 3);
      for(usize i = 0; i < k_FeatureCount * 3; ++i)
      {
        if((i / 3) != 1 && polesRef[i] != 0)
        {
          ++nonZeroPoleValues;
        }
      }
    }
    CHECK(nonZeroPoleValues == 0);

    UnitTest::CheckArraysInheritTupleDims(td.ds);
  }

  SECTION("a phase id beyond the ensemble count is an error, not an out-of-bounds read")
  {
    SchmidFixture td = MakeFixture(2); // 2 ensemble slots -> valid phase ids are 0 and 1
    (*td.featurePhases)[1] = 7;

    ComputeSchmidsFilter filter;
    Arguments args = MakeArgs({1.0F, 2.0F, 3.0F}, true, false);
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().size() == 1);
    CHECK(executeResult.result.errors()[0].code == -13501);
    // The message must name the offending value and the ensemble count.
    CHECK(executeResult.result.errors()[0].message.find('7') != std::string::npos);
    CHECK(executeResult.result.errors()[0].message.find('2') != std::string::npos);
  }

  SECTION("a negative phase id is an error, not an out-of-bounds read")
  {
    SchmidFixture td = MakeFixture(2);
    (*td.featurePhases)[1] = -3;

    ComputeSchmidsFilter filter;
    Arguments args = MakeArgs({1.0F, 2.0F, 3.0F}, true, false);
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().size() == 1);
    CHECK(executeResult.result.errors()[0].code == -13502);
    CHECK(executeResult.result.errors()[0].message.find("-3") != std::string::npos);
  }
}

TEST_CASE("OrientationAnalysis::ComputeSchmidsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeSchmidsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeSchmidsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeSchmidsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeSchmidsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(ComputeSchmidsFilter::k_StoreAngleComponents_Key) == true);
      CHECK(args.value<bool>(ComputeSchmidsFilter::k_OverrideSystem_Key) == true);
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ComputeSchmidsFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeSchmidsFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeSchmidsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeSchmidsFilter::k_SchmidsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeSchmidsFilter::k_SlipSystemsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeSchmidsFilter::k_PolesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeSchmidsFilter::k_PhisArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeSchmidsFilter::k_LambdasArrayName_Key) == "TestName");
    }
  }
}
