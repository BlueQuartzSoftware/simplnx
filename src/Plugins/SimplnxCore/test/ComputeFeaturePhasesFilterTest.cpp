#include "SimplnxCore/Filters/ComputeFeaturePhasesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter(Valid Parameters)", "[SimplnxCore][ComputeFeaturePhasesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);
  DataPath cellFeatureDataPath = smallIn100Group.createChildPath(Constants::k_CellFeatureData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(nx::core::Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(nx::core::Constants::k_FeatureIds);
  std::string computedPrefix = "Computed_";
  std::string featurePhasesName = computedPrefix + nx::core::Constants::k_Phases.str();
  DataPath featurePhasesPath = cellFeatureDataPath.createChildPath(featurePhasesName);

  {
    ComputeFeaturePhasesFilter ffpFilter;
    Arguments args;
    args.insert(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key, std::make_any<DataPath>(cellFeatureDataPath));
    args.insert(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(featurePhasesName));

    auto preflightResult = ffpFilter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = ffpFilter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);

    DataPath exemplaryDataPath = cellFeatureDataPath.createChildPath(nx::core::Constants::k_Phases);
    const Int32Array& featureArrayExemplary = dataStructure.getDataRefAs<Int32Array>(exemplaryDataPath);

    const Int32Array& createdFeatureArray = dataStructure.getDataRefAs<Int32Array>(featurePhasesPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_phases_filter.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeaturePhasesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeaturePhasesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeaturePhasesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeaturePhasesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeaturePhasesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeaturePhasesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeaturePhasesFilter::k_CellFeaturesAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeFeaturePhasesFilter::k_FeaturePhasesArrayName_Key) == "TestArray");
    }
  }
}
