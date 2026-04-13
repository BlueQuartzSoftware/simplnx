#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ComputeFeaturePhasesBinaryFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_BinaryFeaturePhasesName = "BinaryFeaturePhases";

const DataPath k_ExemplarArray = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BinaryPhases"});
const DataPath k_GeneratedArray = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_BinaryFeaturePhasesName});
} // namespace

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: Valid Filter Execution", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "bin_feature_phases.tar.gz", "bin_feature_phases.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/bin_feature_phases/6_6_find_feature_phases_binary.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeaturePhasesBinaryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_Mask})));
    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_CellDataAMPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData})));

    args.insertOrAssign(ComputeFeaturePhasesBinaryFilter::k_FeaturePhasesArrayName_Key, std::make_any<std::string>(k_BinaryFeaturePhasesName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<int32>(dataStructure, k_ExemplarArray, k_GeneratedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeaturePhasesBinaryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeaturePhasesBinaryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeaturePhasesBinaryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeaturePhasesBinaryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeaturePhasesBinaryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeaturePhasesBinaryFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFeaturePhasesBinaryFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFeaturePhasesBinaryFilter::k_FeaturePhasesArrayName_Key) == "TestArray");
      CHECK(args.value<DataPath>(ComputeFeaturePhasesBinaryFilter::k_CellDataAMPath_Key) == DataPath({"DataContainer", "CellData"}));
    }
  }
}
