
#include "SimplnxCore/Filters/RobustAutomaticThresholdFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::RobustAutomaticThresholdFilter: Missing/Empty DataPaths", "[RobustAutomaticThresholdFilter]")
{
  UnitTest::LoadPlugins();

  RobustAutomaticThresholdFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_InputArrayPath_Key, std::make_any<DataPath>(inputPath));

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_GradientMagnitudePath_Key, std::make_any<DataPath>(gradientMagnitudePath));

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RobustAutomaticThresholdFilter: Test Algorithm", "[RobustAutomaticThresholdFilter]")
{
  UnitTest::LoadPlugins();

  RobustAutomaticThresholdFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  args.insertOrAssign(RobustAutomaticThresholdFilter::k_InputArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_GradientMagnitudePath_Key, std::make_any<DataPath>(gradientMagnitudePath));
  args.insertOrAssign(RobustAutomaticThresholdFilter::k_ArrayCreationName_Key, std::make_any<std::string>("Created Array"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RobustAutomaticThresholdFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RobustAutomaticThresholdFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RobustAutomaticThresholdFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RobustAutomaticThresholdFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RobustAutomaticThresholdFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(RobustAutomaticThresholdFilter::k_InputArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RobustAutomaticThresholdFilter::k_GradientMagnitudePath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(RobustAutomaticThresholdFilter::k_ArrayCreationName_Key) == "TestArray");
    }
  }
}
