#include <catch2/catch.hpp>

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/ExecuteProcessFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
using namespace nx::core;

TEST_CASE("SimplnxCore::ExecuteProcessFilter: Valid filter execution")
{
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
}

TEST_CASE("SimplnxCore::ExecuteProcessFilter: InValid filter execution")
{
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
}
