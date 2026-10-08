#include "SimplnxCore/Filters/ComputeNumFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_FeatureCounts("Feature Count");
const std::string k_FeatureCountsNX("Feature Count NX");

const DataPath k_FeaturePhasesPath({Constants::k_DataContainer, Constants::k_FeatureData, Constants::k_Phases});
const DataPath k_IncorrectFeaturePhasesPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_Phases});

const DataPath k_FeatureCountsPath({Constants::k_DataContainer, Constants::k_CellEnsembleData, k_FeatureCounts});
const DataPath k_FeatureCountsPathNX({Constants::k_DataContainer, Constants::k_CellEnsembleData, k_FeatureCountsNX});

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_volume_fraction_feature_count.dream3d", unit_test::k_TestFilesDir));

template <typename T>
Arguments CreatePhaseFixture(DataStructure& dataStructure, const std::vector<T>& values, const ShapeType& componentShape = {1})
{
  const usize numComponents = componentShape.front();
  auto* featureData = AttributeMatrix::Create(dataStructure, "Feature Data", {values.size() / numComponents});
  REQUIRE(featureData != nullptr);
  auto* phases = UnitTest::CreateTestDataArray<T>(dataStructure, "Phases", {values.size() / numComponents}, componentShape, featureData->getId());
  REQUIRE(phases != nullptr);
  for(usize index = 0; index < values.size(); index++)
  {
    (*phases)[index] = values[index];
  }
  REQUIRE(AttributeMatrix::Create(dataStructure, "Ensemble Data", {3}) != nullptr);

  auto args = ComputeNumFeaturesFilter().getDefaultArguments();
  args.insertOrAssign(ComputeNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Feature Data", "Phases"})));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Ensemble Data"})));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_NumFeaturesArrayName_Key, std::make_any<std::string>("Counts"));
  return args;
}

void RequireInvalidPhase(const std::vector<int32>& values, const std::string& expectedValue, const std::string& expectedTuple)
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const auto args = CreatePhaseFixture<int32>(dataStructure, values);
  ComputeNumFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  const auto& error = executeResult.result.errors().front();
  REQUIRE(error.code == -9740);
  REQUIRE_THAT(error.message, Catch::Matchers::Contains(expectedValue));
  REQUIRE_THAT(error.message, Catch::Matchers::Contains(expectedTuple));
  REQUIRE_THAT(error.message, Catch::Matchers::Contains("Ensemble tuple count 3"));

  // Validation must finish before counting any features.
  const DataPath countsPath({"Ensemble Data", "Counts"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto& counts = dataStructure.getDataRefAs<Int32Array>(countsPath);
  REQUIRE(counts[0] == 0);
  REQUIRE(counts[1] == 0);
  REQUIRE(counts[2] == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: Feature Phases requires 1 component", "[SimplnxCore][ComputeNumFeaturesFilter][ComponentShape]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const auto args = CreatePhaseFixture<int32>(dataStructure, {0, 0, 1, 1}, {2});
  ComputeNumFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == -208);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: Feature Phases requires int32", "[SimplnxCore][ComputeNumFeaturesFilter][ComponentShape]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const auto args = CreatePhaseFixture<float32>(dataStructure, {0.0F, 1.0F});
  ComputeNumFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().front().code == -207);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: Rejects phase above Ensemble range", "[SimplnxCore][ComputeNumFeaturesFilter][ComponentShape]")
{
  RequireInvalidPhase({0, 1, 5}, "value 5", "tuple index 2");
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: Rejects negative phase", "[SimplnxCore][ComputeNumFeaturesFilter][ComponentShape]")
{
  RequireInvalidPhase({0, -1}, "value -1", "tuple index 1");
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: Counts valid phases and skips feature zero", "[SimplnxCore][ComputeNumFeaturesFilter][ComponentShape]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const auto args = CreatePhaseFixture<int32>(dataStructure, {0, 1, 2, 1, 0, 2, 1});
  ComputeNumFeaturesFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath countsPath({"Ensemble Data", "Counts"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(countsPath));
  const auto& counts = dataStructure.getDataRefAs<Int32Array>(countsPath);
  REQUIRE(counts.getNumberOfTuples() == 3);
  REQUIRE(counts[0] == 1);
  REQUIRE(counts[1] == 3);
  REQUIRE(counts[2] == 2);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: Valid filter execution", "[SimplnxCore][ComputeNumFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeNumFeaturesFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz", "6_6_volume_fraction_feature_count.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX.getParent()));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_NumFeaturesArrayName_Key, std::make_any<std::string>(k_FeatureCountsNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& d3dFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPath);
  auto& nxFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPathNX);

  for(usize index = 0; index < d3dFeatureCountsArrayRef.getSize(); index++)
  {
    REQUIRE(d3dFeatureCountsArrayRef[index] == nxFeatureCountsArrayRef[index]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: InValid filter execution", "[SimplnxCore][ComputeNumFeaturesFilter]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeNumFeaturesFilter filter;
  Arguments args;

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_volume_fraction_feature_count.dream3d.tar.gz", "6_6_volume_fraction_feature_count.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeNumFeaturesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_IncorrectFeaturePhasesPath));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureCountsPathNX.getParent()));
  args.insertOrAssign(ComputeNumFeaturesFilter::k_NumFeaturesArrayName_Key, std::make_any<std::string>(k_FeatureCountsNX));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& d3dFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPath);
  auto& nxFeatureCountsArrayRef = dataStructure.getDataRefAs<Int32Array>(k_FeatureCountsPathNX);

  for(usize index = 1; index < d3dFeatureCountsArrayRef.getSize(); index++)
  {
    REQUIRE(d3dFeatureCountsArrayRef[index] != nxFeatureCountsArrayRef[index]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeNumFeaturesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeNumFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeNumFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeNumFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeNumFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeNumFeaturesFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeNumFeaturesFilter::k_EnsembleAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeNumFeaturesFilter::k_NumFeaturesArrayName_Key) == "TestArray");
    }
  }
}
