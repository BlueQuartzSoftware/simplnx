#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ComputeAvgCAxesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace compute_avg_caxis_class1
{
// Paths for the Class 1 Oracle (hand-built) input dataset. See
// src/Plugins/OrientationAnalysis/vv/comparisons/ComputeAvgCAxesFilter/generate_inputs.py
// for the input definition and the closed-form expected outputs.
const DataPath k_DataContainerPath({"DataContainer"});
const DataPath k_CellDataPath = k_DataContainerPath.createChildPath("CellData");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_QuatsPath = k_CellDataPath.createChildPath("Quats");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_CellFeatureDataPath = k_DataContainerPath.createChildPath("CellFeatureData");
const DataPath k_CrystalStructuresPath = k_DataContainerPath.createChildPath("CellEnsembleData").createChildPath("CrystalStructures");
const std::string k_ComputedAvgCAxesName = "ComputedAvgCAxes";
} // namespace compute_avg_caxis_class1

TEST_CASE("OrientationAnalysis::ComputeAvgCAxesFilter: Class 1 Oracle (hand-built dataset)", "[OrientationAnalysis][ComputeAvgCAxesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_avg_c_axis.tar.gz", "compute_avg_c_axis");

  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "compute_avg_c_axis" / "data" / "compute_avg_caxes_inputs.dream3d";

  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFile);

  ComputeAvgCAxesFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeAvgCAxesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_QuatsPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_FeatureIdsPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_PhasesPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_CrystalStructuresPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_CellFeatureDataPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_AvgCAxesArrayName_Key, std::make_any<std::string>(compute_avg_caxis_class1::k_ComputedAvgCAxesName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath avgCAxesPath = compute_avg_caxis_class1::k_CellFeatureDataPath.createChildPath(compute_avg_caxis_class1::k_ComputedAvgCAxesName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(avgCAxesPath));
  const auto& avgCAxes = dataStructure.getDataRefAs<Float32Array>(avgCAxesPath);
  REQUIRE(avgCAxes.getNumberOfTuples() == 8);
  REQUIRE(avgCAxes.getNumberOfComponents() == 3);

  // ---------------------------------------------------------------------------
  // Class 1 (Analytical) — exact-value checks for F0..F6.
  // Closed-form expected values derived in
  // src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md and verified
  // bit-identical against the DREAM3D 6.5.172 custom backport
  // (vv/comparisons/ComputeAvgCAxesFilter/results/three_way_comparison.txt).
  // ---------------------------------------------------------------------------
  constexpr float32 k_Tol = 1.0e-5f;
  constexpr float32 k_SqrtThreeOverTwo = 0.8660254f;

  struct ExpectedFeature
  {
    int32 featureId;
    bool isNaN;
    float32 x;
    float32 y;
    float32 z;
    const char* description;
  };

  const std::vector<ExpectedFeature> expected = {
      {0, true, 0.0f, 0.0f, 0.0f, "F0: placeholder feature, never referenced -> counter==0 -> NaN"},
      {1, false, 0.0f, 0.0f, 1.0f, "F1: single cell, identity quaternion"},
      {2, false, 0.0f, k_SqrtThreeOverTwo, 0.5f, "F2: single cell, +60 deg about +X"},
      {3, false, 0.0f, 0.0f, 1.0f, "F3: three aligned identity cells -- trivial average"},
      {4, false, 0.0f, 0.0f, 1.0f, "F4: antipodal pair -- antipodal-flip resolves to (0,0,1)"},
      {5, true, 0.0f, 0.0f, 0.0f, "F5: declared feature, no cells reference it -> counter==0 -> NaN"},
      {6, true, 0.0f, 0.0f, 0.0f, "F6: sole cell is non-hex (Cubic_High) -> counter==0 -> NaN"},
  };

  for(const auto& exp : expected)
  {
    DYNAMIC_SECTION(exp.description)
    {
      const usize idx = 3 * static_cast<usize>(exp.featureId);
      const float32 x = avgCAxes[idx];
      const float32 y = avgCAxes[idx + 1];
      const float32 z = avgCAxes[idx + 2];
      if(exp.isNaN)
      {
        REQUIRE(std::isnan(x));
        REQUIRE(std::isnan(y));
        REQUIRE(std::isnan(z));
      }
      else
      {
        REQUIRE(x == Approx(exp.x).margin(k_Tol));
        REQUIRE(y == Approx(exp.y).margin(k_Tol));
        REQUIRE(z == Approx(exp.z).margin(k_Tol));
      }
    }
  }

  // ---------------------------------------------------------------------------
  // Class 4 (Invariant) — F7 precision-sensitive boundary case.
  // F7 was deliberately constructed so the antipodal-flip cancellation dot
  // product evaluates to zero in math but is precision-dependent in float32.
  // The SIMPLNX faithful-float32 Eigen path produces (0, sqrt(3)/2, 0.5);
  // a pure-double replay produces (0, 0, 1). These are genuinely different
  // c-axis directions, not hex c~-c equivalents. After the SIMPLNX
  // finalize-normalize step, both have magnitude 1.0 -- so the direction is
  // implementation-dependent but the unit-vector invariant holds.
  // See Deviation D2 in vv/deviations/ComputeAvgCAxesFilter.md.
  // ---------------------------------------------------------------------------
  DYNAMIC_SECTION("F7: magnitude == 1.0 invariant (precision-sensitive direction)")
  {
    const usize idx = 3 * 7;
    const float32 x = avgCAxes[idx];
    const float32 y = avgCAxes[idx + 1];
    const float32 z = avgCAxes[idx + 2];
    REQUIRE_FALSE(std::isnan(x));
    REQUIRE_FALSE(std::isnan(y));
    REQUIRE_FALSE(std::isnan(z));
    const float32 magnitude = std::sqrt(x * x + y * y + z * z);
    REQUIRE(magnitude == Approx(1.0f).margin(k_Tol));
  }

  // ---------------------------------------------------------------------------
  // Class 4 (Invariant) — general unit-vector invariant over all hex-valid features.
  // The Phase-7 finalize loop normalizes every counter>0 feature, so each
  // hex-valid output must be a unit vector within float32 epsilon.
  // ---------------------------------------------------------------------------
  DYNAMIC_SECTION("General invariant: hex-valid features have magnitude == 1.0")
  {
    for(int32 fid : {1, 2, 3, 4, 7})
    {
      const usize idx = 3 * static_cast<usize>(fid);
      const float32 x = avgCAxes[idx];
      const float32 y = avgCAxes[idx + 1];
      const float32 z = avgCAxes[idx + 2];
      const float32 magnitude = std::sqrt(x * x + y * y + z * z);
      REQUIRE(magnitude == Approx(1.0f).margin(k_Tol));
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeAvgCAxesFilter:No_Hex_Phase", "[OrientationAnalysis][ComputeAvgCAxesFilter]")
{
  UnitTest::LoadPlugins();

  // Reuse the Class 1 hand-built input. Mutate the ensemble CrystalStructures
  // array so phase 1 (originally Hexagonal_High = 0) becomes Cubic_High = 1.
  // Phase 2 is already Cubic_High, so after this mutation no ensemble phase
  // is hexagonal and the algorithm must emit -76402.
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_avg_c_axis.tar.gz", "compute_avg_c_axis");

  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "compute_avg_c_axis" / "data" / "compute_avg_caxes_inputs.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFile);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(compute_avg_caxis_class1::k_CrystalStructuresPath));
  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(compute_avg_caxis_class1::k_CrystalStructuresPath);
  crystalStructs[1] = 1; // Hexagonal_High -> Cubic_High

  ComputeAvgCAxesFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeAvgCAxesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_QuatsPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_FeatureIdsPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_PhasesPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_CrystalStructuresPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(compute_avg_caxis_class1::k_CellFeatureDataPath));
  args.insertOrAssign(ComputeAvgCAxesFilter::k_AvgCAxesArrayName_Key, std::make_any<std::string>(compute_avg_caxis_class1::k_ComputedAvgCAxesName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -76402);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeAvgCAxesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeAvgCAxesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeAvgCAxesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeAvgCAxesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeAvgCAxesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_CellFeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeAvgCAxesFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeAvgCAxesFilter::k_AvgCAxesArrayName_Key) == "TestArray");
    }
  }
}
