#include "OrientationAnalysis/Filters/ComputeTwinBoundariesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <Eigen/Core>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace TwinBoundariesConstants
{
// uint8 mask type
const ChoicesParameter::ValueType k_OutputType = 1ULL; // represents uint8

// Tolerances used for generation
const float32 k_AxisToleranceValue = 1.0f;
const float32 k_AngleToleranceValue = 1.0f;

// Feature Level Data
const DataPath k_AvgQuatsPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_AvgQuats});
const DataPath k_PhasesPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});

// Ensemble Level Data
const DataPath k_CrystalStructuresPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures});

// Face Level Data
const StringLiteral k_TriangleContainerName = "TriangleDataContainer";
const DataPath k_FaceDataPath({k_TriangleContainerName, Constants::k_FaceData});
const DataPath k_FaceLabelsPath = k_FaceDataPath.createChildPath(Constants::k_FaceLabels);
const DataPath k_FaceNormalsPath = k_FaceDataPath.createChildPath(Constants::k_FaceNormals);

// Created Data Names/Paths
const StringLiteral k_ExemplarBoundariesName = "NX Twin Boundaries";
const StringLiteral k_ExemplarIncoherenceName = "NX Twin Boundaries Incoherence";

const StringLiteral k_GeneratedBoundariesName = "Generated Twin Boundaries";
const StringLiteral k_GeneratedIncoherenceName = "Generated Twin Boundaries Incoherence";

const DataPath k_ExemplarBoundariesPath = k_FaceDataPath.createChildPath(k_ExemplarBoundariesName);
const DataPath k_ExemplarIncoherencePath = k_FaceDataPath.createChildPath(k_ExemplarIncoherenceName);

const DataPath k_GeneratedBoundariesPath = k_FaceDataPath.createChildPath(k_GeneratedBoundariesName);
const DataPath k_GeneratedIncoherencePath = k_FaceDataPath.createChildPath(k_GeneratedIncoherenceName);
} // namespace TwinBoundariesConstants

TEST_CASE("OrientationAnalysis::ComputeTwinBoundariesFilter: Baseline Incoherence", "[SimplnxCore][ComputeTwinBoundariesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_twin_boundaries_test_v2.tar.gz", "compute_twin_boundaries_test");

  // Read the modified Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_twin_boundaries_test/validation/7_0_Compute_Twin_Boundaries_Test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate ComputeTwinBoundariesFilter
  {
    ComputeTwinBoundariesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AxisTolerance_Key, std::make_any<float32>(TwinBoundariesConstants::k_AxisToleranceValue));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AngleTolerance_Key, std::make_any<float32>(TwinBoundariesConstants::k_AngleToleranceValue));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_BoundariesArrayType_Key, std::make_any<ChoicesParameter::ValueType>(TwinBoundariesConstants::k_OutputType));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FindCoherence_Key, std::make_any<bool>(true));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_FaceLabelsPath));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FaceNormalsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_FaceNormalsPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_PhasesPath));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_AvgQuatsPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_CrystalStructuresPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_TwinBoundariesName_Key, std::make_any<std::string>(TwinBoundariesConstants::k_GeneratedBoundariesName));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_TwinBoundariesIncoherenceName_Key, std::make_any<std::string>(TwinBoundariesConstants::k_GeneratedIncoherenceName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    UnitTest::CompareArrays<float32>(dataStructure, TwinBoundariesConstants::k_ExemplarIncoherencePath, TwinBoundariesConstants::k_GeneratedIncoherencePath);
    UnitTest::CompareArrays<uint8>(dataStructure, TwinBoundariesConstants::k_ExemplarBoundariesPath, TwinBoundariesConstants::k_GeneratedBoundariesPath);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/compute_twin_boundaries/base_incoherence.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeTwinBoundariesFilter: No Incoherence", "[SimplnxCore][ComputeTwinBoundariesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_twin_boundaries_test_v2.tar.gz", "compute_twin_boundaries_test");

  // Read the modified Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_twin_boundaries_test/validation/7_0_Compute_Twin_Boundaries_Test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate ComputeTwinBoundariesFilter
  {
    ComputeTwinBoundariesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AxisTolerance_Key, std::make_any<float32>(TwinBoundariesConstants::k_AxisToleranceValue));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AngleTolerance_Key, std::make_any<float32>(TwinBoundariesConstants::k_AngleToleranceValue));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_BoundariesArrayType_Key, std::make_any<ChoicesParameter::ValueType>(TwinBoundariesConstants::k_OutputType));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FindCoherence_Key, std::make_any<bool>(false));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_FaceLabelsPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_PhasesPath));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_AvgQuatsPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_CrystalStructuresPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_TwinBoundariesName_Key, std::make_any<std::string>(TwinBoundariesConstants::k_GeneratedBoundariesName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    // The exemplar algorithm skips invalid values while this algorithm doesn't due to lack of a priori knowledge (face normals array) so we simulate it here
    auto& faceNormals = dataStructure.getDataRefAs<Float64Array>(TwinBoundariesConstants::k_FaceNormalsPath);
    auto& exemplarBoundaries = dataStructure.getDataRefAs<UInt8Array>(TwinBoundariesConstants::k_ExemplarBoundariesPath);
    auto& generatedBoundaries = dataStructure.getDataRefAs<UInt8Array>(TwinBoundariesConstants::k_GeneratedBoundariesPath);
    for(usize i = 0; i < faceNormals.getNumberOfTuples(); i++)
    {
      const Eigen::Vector3d normals{faceNormals[3 * i], faceNormals[(3 * i) + 1], faceNormals[(3 * i) + 2]};

      if(normals.hasNaN())
      {
        continue;
      }

      REQUIRE(exemplarBoundaries[i] == generatedBoundaries[i]);
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/compute_twin_boundaries/no_incoherence.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeTwinBoundariesFilter: NaN Warning Check", "[SimplnxCore][ComputeTwinBoundariesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_twin_boundaries_test_v2.tar.gz", "compute_twin_boundaries_test");

  // Read the modified Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_twin_boundaries_test/validation/7_0_Compute_Twin_Boundaries_Test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate ComputeTwinBoundariesFilter
  {
    ComputeTwinBoundariesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AxisTolerance_Key, std::make_any<float32>(TwinBoundariesConstants::k_AxisToleranceValue));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AngleTolerance_Key, std::make_any<float32>(TwinBoundariesConstants::k_AngleToleranceValue));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FindCoherence_Key, std::make_any<bool>(true));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_FaceLabelsPath));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FaceNormalsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_FaceNormalsPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_PhasesPath));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_AvgQuatsPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(TwinBoundariesConstants::k_CrystalStructuresPath));

    args.insertOrAssign(ComputeTwinBoundariesFilter::k_TwinBoundariesName_Key, std::make_any<std::string>(TwinBoundariesConstants::k_GeneratedBoundariesName));
    args.insertOrAssign(ComputeTwinBoundariesFilter::k_TwinBoundariesIncoherenceName_Key, std::make_any<std::string>(TwinBoundariesConstants::k_GeneratedIncoherenceName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    REQUIRE(!executeResult.result.warnings().empty());

    bool found = false;
    for(const auto& warning : executeResult.result.warnings())
    {
      if(warning.code == -93213)
      {
        found = true;
      }
    }

    REQUIRE(found);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeTwinBoundariesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeTwinBoundariesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeTwinBoundariesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeTwinBoundariesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeTwinBoundariesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeTwinBoundariesFilter::k_FindCoherence_Key) == true);
      CHECK(args.value<float32>(ComputeTwinBoundariesFilter::k_AxisTolerance_Key) == 2.5f);
      CHECK(args.value<float32>(ComputeTwinBoundariesFilter::k_AngleTolerance_Key) == 2.5f);
      CHECK(args.value<DataPath>(ComputeTwinBoundariesFilter::k_FaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeTwinBoundariesFilter::k_FaceNormalsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeTwinBoundariesFilter::k_AvgQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeTwinBoundariesFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeTwinBoundariesFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeTwinBoundariesFilter::k_TwinBoundariesName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeTwinBoundariesFilter::k_TwinBoundariesIncoherenceName_Key) == "TestName");
    }
  }
}
