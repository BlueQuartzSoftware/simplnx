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

#include "SimplnxCore/Filters/Algorithms/ExecuteProcess.hpp"
#include "SimplnxCore/Filters/ExecuteProcessFilter.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @brief Counts the non-overlapping occurrences of a marker in a text.
 * @param text Text to search.
 * @param marker Non-empty marker to count.
 * @return Number of occurrences.
 */
usize CountOccurrences(std::string_view text, std::string_view marker)
{
  usize count = 0;
  for(usize position = text.find(marker); position != std::string_view::npos; position = text.find(marker, position + marker.size()))
  {
    ++count;
  }
  return count;
}
} // namespace

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

TEST_CASE("SimplnxCore::ExecuteProcessFilter: Argument separator regression", "[SimplnxCore][ExecuteProcessFilter]")
{
  // Oracle: spaces outside quotes separate tokens; spaces inside quotes are literal.
  // Each command below therefore has exactly the explicitly listed arguments.
  const std::vector<std::pair<std::string, std::vector<std::string>>> cases = {
      {R"("/a b/prog" --help)", {"/a b/prog", "--help"}}, {"prog  --help", {"prog", "--help"}}, {R"(prog "a b" --help)", {"prog", "a b", "--help"}},
      {R"(  "a b"   " c d "  )", {"a b", " c d "}},       {"prog --help", {"prog", "--help"}},
  };
  for(const auto& [command, expected] : cases)
  {
    DYNAMIC_SECTION(command)
    {
      const auto actual = ExecuteProcess::splitArgumentsString(command);
      REQUIRE(actual == expected);
    }
  }
}

TEST_CASE("SimplnxCore::ExecuteProcessFilter: Execution captures standard error", "[SimplnxCore][ExecuteProcessFilter]")
{
  const bool blocking = GENERATE(true, false);
  CAPTURE(blocking);
  ExecuteProcessFilter filter;
  DataStructure dataStructure;
  auto args = filter.getDefaultArguments();
  const fs::path logPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("c01-process-{}.log", blocking ? "blocking" : "nonblocking");
#if NDEBUG
  const fs::path childPath = fs::path(unit_test::k_BuildDir.view()) / "SimplnxCoreProcessOutputTestChild";
#else
  const fs::path childPath = fs::path(unit_test::k_BuildDir.view()) / "SimplnxCoreProcessOutputTestChild_d";
#endif
  args.insertOrAssign(ExecuteProcessFilter::k_Arguments_Key, std::make_any<std::string>(fmt::format("\"{}\"", childPath.string())));
  args.insertOrAssign(ExecuteProcessFilter::k_Blocking_Key, std::make_any<bool>(blocking));
  args.insertOrAssign(ExecuteProcessFilter::k_Timeout_Key, std::make_any<int32>(5000));
  args.insertOrAssign(ExecuteProcessFilter::k_OutputLogFile_Key, std::make_any<fs::path>(logPath));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  std::ifstream log(logPath);
  REQUIRE(log.is_open());
  std::stringstream content;
  content << log.rdbuf();
  const std::string logText = content.str();
  // The deterministic child writes each fixed marker once to its own stream and returns zero.
  // The log must therefore hold each marker exactly once.
  CHECK(CountOccurrences(logText, "C01_STDOUT") == 1);
  CHECK(CountOccurrences(logText, "C01_STDERR") == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
