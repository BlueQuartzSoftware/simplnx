#include "OrientationAnalysis/Filters/ComputeCAxisMisorientationsFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <limits>

using namespace nx::core;

namespace
{
namespace CAxisMisorientationFixtures
{
// A flat DataGroup > AttributeMatrix hierarchy is sufficient — this filter requires no geometry.
const auto k_FeatureDataPath = DataPath{{"TestData", "FeatureData"}};
const DataPath k_AvgCAxesPath = k_FeatureDataPath.createChildPath("AvgCAxes");
const std::string k_MisorientationsName = "CAxisMisorientations";
const DataPath k_MisorientationsPath = k_FeatureDataPath.createChildPath(k_MisorientationsName);

struct FixtureData
{
  DataStructure ds;
  Float32Array* avgCAxes = nullptr;
  usize numFeatures = 0;
};

// Build a minimal DataStructure with a feature-level AvgCAxes array.
// All features are initialized to NaN, matching the ComputeAvgCAxes sentinel convention for
// features that either have no cells or belong to a non-hexagonal phase.
FixtureData CreateScaffold(usize numFeatures)
{
  FixtureData td;
  td.numFeatures = numFeatures;

  const auto* dataGroup = DataGroup::Create(td.ds, "TestData");
  const auto* featureAM = AttributeMatrix::Create(td.ds, "FeatureData", ShapeType{numFeatures}, dataGroup->getId());
  const auto avgCAxesStore = DataStoreUtilities::CreateDataStore<float32>(td.ds, k_AvgCAxesPath, {numFeatures}, {3});
  td.avgCAxes = Float32Array::Create(td.ds, "AvgCAxes", avgCAxesStore, featureAM->getId());

  const float32 nan = std::numeric_limits<float32>::quiet_NaN();
  for(usize i = 0; i < numFeatures * 3; ++i)
  {
    (*td.avgCAxes)[i] = nan;
  }

  return td;
}

void SetCAxis(FixtureData& td, usize featureIdx, float32 cx, float32 cy, float32 cz)
{
  (*td.avgCAxes)[featureIdx * 3 + 0] = cx;
  (*td.avgCAxes)[featureIdx * 3 + 1] = cy;
  (*td.avgCAxes)[featureIdx * 3 + 2] = cz;
}

Arguments BuildArgs(std::vector<float32> refDir)
{
  Arguments args;
  args.insertOrAssign(ComputeCAxisMisorientationsFilter::k_ReferenceDir_Key, std::make_any<std::vector<float32>>(std::move(refDir)));
  args.insertOrAssign(ComputeCAxisMisorientationsFilter::k_AvgCAxesArrayPath_Key, std::make_any<DataPath>(k_AvgCAxesPath));
  args.insertOrAssign(ComputeCAxisMisorientationsFilter::k_MisorientationArrayName_Key, std::make_any<std::string>(k_MisorientationsName));
  return args;
}

const Float32Array& GetOutput(const DataStructure& ds)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_MisorientationsPath));
  return ds.getDataRefAs<Float32Array>(k_MisorientationsPath);
}

} // namespace CAxisMisorientationFixtures
} // namespace

// =====================================================================================
// Preflight error
// =====================================================================================

TEST_CASE("OrientationAnalysis::ComputeCAxisMisorientationsFilter: Preflight Error - Zero Reference Direction (-77000)", "[OrientationAnalysis][ComputeCAxisMisorientationsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // A zero-magnitude reference direction is a user error that cannot be caught at parameter
  // entry time, so the filter must reject it during preflight.
  auto td = CAxisMisorientationFixtures::CreateScaffold(2);
  CAxisMisorientationFixtures::SetCAxis(td, 1, 0.0f, 0.0f, 1.0f);

  ComputeCAxisMisorientationsFilter filter;
  const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 0.0f});

  const auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -77000);
}

// =====================================================================================
// Class 1 (Analytical) — exact expected values derived from pure vector geometry.
//
// Reference direction is [0, 0, 1] (global Z) for every section. The expected angle is
// arccos(|cAxis · refDir|) converted to degrees. The antipodal cases (antiparallel and
// folded antiparallel) verify that the |dot| in the algorithm keeps results in [0°, 90°].
//
// Feature 0 is the sentinel (NaN in all sections). Feature 1 is the test feature.
// =====================================================================================

TEST_CASE("OrientationAnalysis::ComputeCAxisMisorientationsFilter: Class 1 - Analytical Angles", "[OrientationAnalysis][ComputeCAxisMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Shared constants for intermediate angles.
  static const float32 k_Sqrt2Inv = 1.0f / std::sqrt(2.0f);
  static const float32 k_Sqrt3Over2 = std::sqrt(3.0f) / 2.0f;
  static constexpr float32 k_Half = 0.5f;

  // CreateScaffold and the filter are re-constructed fresh for each SECTION by Catch2.
  auto td = CAxisMisorientationFixtures::CreateScaffold(2);
  ComputeCAxisMisorientationsFilter filter;

  SECTION("Aligned: c=[0,0,1] vs ref[0,0,1] => 0°")
  {
    CAxisMisorientationFixtures::SetCAxis(td, 1, 0.0f, 0.0f, 1.0f);
    const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);
    REQUIRE(output[1] == Approx(0.0f).margin(1e-3f));
  }

  SECTION("Perpendicular: c=[1,0,0] vs ref[0,0,1] => 90°")
  {
    CAxisMisorientationFixtures::SetCAxis(td, 1, 1.0f, 0.0f, 0.0f);
    const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);
    REQUIRE(output[1] == Approx(90.0f).margin(1e-3f));
  }

  SECTION("45°: c=[1/√2, 0, 1/√2] vs ref[0,0,1]")
  {
    CAxisMisorientationFixtures::SetCAxis(td, 1, k_Sqrt2Inv, 0.0f, k_Sqrt2Inv);
    const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);
    REQUIRE(output[1] == Approx(45.0f).margin(1e-3f));
  }

  SECTION("30°: c=[0.5, 0, √3/2] vs ref[0,0,1]")
  {
    CAxisMisorientationFixtures::SetCAxis(td, 1, k_Half, 0.0f, k_Sqrt3Over2);
    const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);
    REQUIRE(output[1] == Approx(30.0f).margin(1e-3f));
  }

  SECTION("Antiparallel: c=[0,0,-1] vs ref[0,0,1] => 0° (antipodal symmetry)")
  {
    // The c-axis [001] and [00-1] are crystallographically equivalent in hexagonal.
    // The |dot| in the algorithm must fold this back to 0° rather than 180°.
    CAxisMisorientationFixtures::SetCAxis(td, 1, 0.0f, 0.0f, -1.0f);
    const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);
    REQUIRE(output[1] == Approx(0.0f).margin(1e-3f));
  }

  SECTION("Folded antiparallel 30°: c=[-0.5, 0, -√3/2] vs ref[0,0,1] => 30°")
  {
    // Same as the 30° case but with the c-axis flipped. Both hemispheres must give the same angle.
    CAxisMisorientationFixtures::SetCAxis(td, 1, -k_Half, 0.0f, -k_Sqrt3Over2);
    const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
    auto preflightResult = filter.preflight(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(td.ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);
    REQUIRE(output[1] == Approx(30.0f).margin(1e-3f));
  }
}

// =====================================================================================
// NaN propagation
//
// ComputeAvgCAxes writes NaN for non-hexagonal features. This test verifies that NaN
// propagates to the output for those features and does not corrupt adjacent valid features.
// =====================================================================================

TEST_CASE("OrientationAnalysis::ComputeCAxisMisorientationsFilter: NaN Propagation - Non-Hexagonal Features", "[OrientationAnalysis][ComputeCAxisMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Feature layout:
  //   F0: NaN (sentinel — no cells belong to feature 0)
  //   F1: [0,0,1] => 0°   (valid hex)
  //   F2: NaN              (non-hexagonal phase — marked by ComputeAvgCAxes)
  //   F3: [1,0,0] => 90°  (valid hex, adjacent to the NaN feature)
  auto td = CAxisMisorientationFixtures::CreateScaffold(4);
  CAxisMisorientationFixtures::SetCAxis(td, 1, 0.0f, 0.0f, 1.0f);
  // F2 is left as NaN (scaffold initializes all features to NaN).
  CAxisMisorientationFixtures::SetCAxis(td, 3, 1.0f, 0.0f, 0.0f);

  ComputeCAxisMisorientationsFilter filter;
  const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);

  REQUIRE(std::isnan(output[0]));
  REQUIRE(output[1] == Approx(0.0f).margin(1e-3f));
  REQUIRE(std::isnan(output[2]));
  REQUIRE(output[3] == Approx(90.0f).margin(1e-3f));
}

// =====================================================================================
// Class 4 (Invariants) — oracle-agnostic properties that hold for any valid input.
// =====================================================================================

TEST_CASE("OrientationAnalysis::ComputeCAxisMisorientationsFilter: Class 4 - Invariants", "[OrientationAnalysis][ComputeCAxisMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  // Mixed fixture: F0=sentinel(NaN), F1=0°, F2=90°, F3=30°, F4=NaN (non-hex).
  static const float32 k_Sqrt3Over2 = std::sqrt(3.0f) / 2.0f;
  static constexpr float32 k_Half = 0.5f;

  auto td = CAxisMisorientationFixtures::CreateScaffold(5);
  CAxisMisorientationFixtures::SetCAxis(td, 1, 0.0f, 0.0f, 1.0f);
  CAxisMisorientationFixtures::SetCAxis(td, 2, 1.0f, 0.0f, 0.0f);
  CAxisMisorientationFixtures::SetCAxis(td, 3, k_Half, 0.0f, k_Sqrt3Over2);
  // F0 and F4 are left as NaN by the scaffold.

  ComputeCAxisMisorientationsFilter filter;
  const Arguments args = CAxisMisorientationFixtures::BuildArgs({0.0f, 0.0f, 1.0f});
  auto preflightResult = filter.preflight(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(td.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& output = CAxisMisorientationFixtures::GetOutput(td.ds);

  SECTION("(i) Valid outputs are in [0°, 90°]")
  {
    for(usize f = 0; f < td.numFeatures; ++f)
    {
      if(!std::isnan(output[f]))
      {
        constexpr float32 k_CAxisUpperBoundDeg = 90.0f;
        REQUIRE(output[f] >= 0.0f);
        REQUIRE(output[f] <= k_CAxisUpperBoundDeg);
      }
    }
  }

  SECTION("(ii) NaN in => NaN out; non-NaN in => non-NaN out")
  {
    const auto& avgCAxes = *td.avgCAxes;
    for(usize f = 0; f < td.numFeatures; ++f)
    {
      const bool inputIsNaN = std::isnan(avgCAxes[f * 3]);
      const bool outputIsNaN = std::isnan(output[f]);
      REQUIRE(inputIsNaN == outputIsNaN);
    }
  }
}
