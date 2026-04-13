#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/ExecuteProcessFilter.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
using namespace nx::core;

TEST_CASE("SimplnxCore::ExecuteProcessFilter: Valid filter execution")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExecuteProcessFilter filter;
  DataStructure ds;
  Arguments args;

  fs::path processOutput(fmt::format("{}/ExecuteProcessUnitTestOutput.txt", unit_test::k_BuildDir));
#if NDEBUG // release build
  std::string testCommand = fmt::format("{}/nxrunner --help", unit_test::k_BuildDir);
#else
  std::string testCommand = fmt::format("{}/nxrunner_d --help", unit_test::k_BuildDir);
#endif

  // Create default Parameters for the filter.
  args.insertOrAssign(ExecuteProcessFilter::k_Arguments_Key, std::make_any<StringParameter::ValueType>(testCommand));
  args.insertOrAssign(ExecuteProcessFilter::k_Blocking_Key, std::make_any<bool>(false));
  args.insertOrAssign(ExecuteProcessFilter::k_Timeout_Key, std::make_any<int32>(5000));
  args.insertOrAssign(ExecuteProcessFilter::k_OutputLogFile_Key, std::make_any<FileSystemPathParameter::ValueType>(processOutput));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  std::ifstream processOutputFile(processOutput);
  REQUIRE(processOutputFile.is_open());
  std::stringstream buffer;
  buffer << processOutputFile.rdbuf();

  std::string firstLine;
  std::vector<std::string> outputLines = nx::core::StringUtilities::split(buffer.str(), '\n');
  firstLine = StringUtilities::trimmed(outputLines[0]);
  const std::string correctOutput = fmt::format("nxrunner: Version {} Build Date:{}", nx::core::Version::Package(), nx::core::Version::BuildDate());
  REQUIRE(firstLine == correctOutput);

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ExecuteProcessFilter: InValid filter execution")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExecuteProcessFilter filter;
  DataStructure ds;
  Arguments args;

  fs::path processOutput(fmt::format("{}/ExecuteProcessUnitTestOutput.dream3d", unit_test::k_BinaryTestOutputDir));
  args.insertOrAssign(ExecuteProcessFilter::k_Blocking_Key, std::make_any<bool>(false));
  args.insertOrAssign(ExecuteProcessFilter::k_Timeout_Key, std::make_any<int32>(5000));
  args.insertOrAssign(ExecuteProcessFilter::k_OutputLogFile_Key, std::make_any<FileSystemPathParameter::ValueType>(processOutput));

  SECTION("program not found")
  {
    args.insertOrAssign(ExecuteProcessFilter::k_Arguments_Key, std::make_any<StringParameter::ValueType>("adfshjads"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -56410);
  }

  SECTION("no command line arguments")
  {
    args.insertOrAssign(ExecuteProcessFilter::k_Arguments_Key, std::make_any<StringParameter::ValueType>(""));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ExecuteProcessFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ExecuteProcessFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ExecuteProcessFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ExecuteProcessFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ExecuteProcessFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(ExecuteProcessFilter::k_Arguments_Key) == "TestName");
      CHECK(args.value<bool>(ExecuteProcessFilter::k_Blocking_Key) == true);
      CHECK(args.value<int32>(ExecuteProcessFilter::k_Timeout_Key) == 5);
    }
  }
}
