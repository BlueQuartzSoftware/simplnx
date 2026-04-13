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
} // namespace

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
