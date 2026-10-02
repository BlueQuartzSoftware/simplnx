#include "NxInfo.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ostream>
#include <set>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
/**
 * @struct NxInfoRunResult
 * @brief Stores one captured nxinfo invocation.
 */
struct NxInfoRunResult
{
  int ExitCode = 0;
  std::string Output;
  std::string Errors;
};

/**
 * @class FailingStreamBuffer
 * @brief Rejects all writes to exercise nxinfo output-failure handling.
 */
class FailingStreamBuffer : public std::streambuf
{
protected:
  /**
   * @brief Rejects a block write.
   * @param charactersPtr Ignored source characters.
   * @param characterCount Ignored character count.
   * @return Zero to indicate that no characters were written.
   */
  std::streamsize xsputn(const char* charactersPtr, std::streamsize characterCount) override
  {
    static_cast<void>(charactersPtr);
    static_cast<void>(characterCount);
    return 0;
  }

  /**
   * @brief Rejects a single-character write.
   * @param character Ignored character.
   * @return End-of-file to indicate failure.
   */
  int_type overflow(int_type character) override
  {
    static_cast<void>(character);
    return traits_type::eof();
  }
};

/**
 * @brief Runs nxinfo with captured streams and without plugin loading.
 * @param arguments Command-line arguments excluding the executable name.
 * @return Exit code and captured output streams.
 */
NxInfoRunResult runNxInfo(const std::vector<std::string>& arguments)
{
  std::ostringstream output;
  std::ostringstream errors;
  const nxinfo::LoadPluginsFunctionType doNotLoadPlugins = [](std::ostream&) {};
  const int exitCode = nxinfo::Run(arguments, {.Output = output, .Error = errors}, doNotLoadPlugins);
  return {.ExitCode = exitCode, .Output = output.str(), .Errors = errors.str()};
}

/**
 * @brief Writes a minimal DREAM3D file for an nxinfo test.
 * @param fileName Unique file name in the test-output directory.
 * @return Path of the written DREAM3D file.
 */
fs::path createNxInfoInputFile(std::string_view fileName)
{
  const fs::path inputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  DataStructure dataStructure;
  REQUIRE(DataGroup::Create(dataStructure, "Test Group") != nullptr);
  const Result<> writeResult = DREAM3D::WriteFile(inputPath, dataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  return inputPath;
}

/**
 * @brief Lists direct children of a directory for temporary-file cleanup checks.
 * @param directory Directory to inspect.
 * @return Sorted set of child paths.
 */
std::set<fs::path> listDirectoryChildren(const fs::path& directory)
{
  std::set<fs::path> children;
  for(const fs::directory_entry& entry : fs::directory_iterator(directory))
  {
    children.insert(entry.path());
  }
  return children;
}

/**
 * @brief Finds a hierarchy node by its exported path.
 * @param hierarchy Root hierarchy document.
 * @param path Data path string to find.
 * @return Pointer to the matching node, or nullptr when no node matches.
 */
const nlohmann::json* findNodeByPath(const nlohmann::json& hierarchy, std::string_view path)
{
  std::vector<const nlohmann::json*> pendingNodes;
  for(const auto& object : hierarchy.at("objects"))
  {
    pendingNodes.emplace_back(&object);
  }

  while(!pendingNodes.empty())
  {
    const nlohmann::json* nodePtr = pendingNodes.back();
    pendingNodes.pop_back();
    if(nodePtr->at("path").get<std::string>() == path)
    {
      return nodePtr;
    }
    for(const auto& child : nodePtr->at("children"))
    {
      pendingNodes.emplace_back(&child);
    }
  }
  return nullptr;
}
} // namespace

TEST_CASE("NXInfo::GenerateTestFile", "[.GenerateTestData]")
{
  const fs::path inputPath = createNxInfoInputFile("nxinfo_ctest_input.dream3d");
  REQUIRE(fs::is_regular_file(inputPath));

  const fs::path invalidInputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "nxinfo_ctest_invalid.dream3d";
  std::ofstream invalidInput(invalidInputPath, std::ios::trunc);
  invalidInput << "not a DREAM3D file";
  invalidInput.close();
  REQUIRE(fs::is_regular_file(invalidInputPath));
}

TEST_CASE("NXInfo::Run reports missing arguments")
{
  const NxInfoRunResult result = runNxInfo({});

  REQUIRE(result.ExitCode == -100);
  REQUIRE(result.Output.empty());
  REQUIRE(result.Errors.find("No arguments provided") != std::string::npos);
  REQUIRE(result.Errors.find("Usage:") != std::string::npos);
}

TEST_CASE("NXInfo::Run handles informational and invalid arguments")
{
  SECTION("Help")
  {
    const NxInfoRunResult result = runNxInfo({"--help"});
    REQUIRE(result.ExitCode == 0);
    REQUIRE(result.Output.find("Usage:") == 0);
    REQUIRE(result.Output.find("Each dump, output, and format option may be specified only once") != std::string::npos);
    REQUIRE(result.Output.find("Values beginning with '-'") != std::string::npos);
    REQUIRE(result.Errors.empty());
  }

  SECTION("Version")
  {
    const NxInfoRunResult result = runNxInfo({"--version"});
    REQUIRE(result.ExitCode == 0);
    REQUIRE(result.Output.find("nxinfo: Version") == 0);
    REQUIRE(result.Errors.empty());
  }

  SECTION("Unknown argument")
  {
    const NxInfoRunResult result = runNxInfo({"--unknown"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Output.empty());
    REQUIRE(result.Errors.find("Unknown argument '--unknown'") != std::string::npos);
  }

  SECTION("Missing value")
  {
    const NxInfoRunResult result = runNxInfo({"--format"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Errors.find("Missing value after '--format'") != std::string::npos);
  }

  SECTION("Invalid format")
  {
    const NxInfoRunResult result = runNxInfo({"--dump-data-structure", "missing.dream3d", "--format", "XML"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Errors.find("Unknown format 'XML'. Expected one of JSON, TEXT, DOT.") != std::string::npos);
  }

  SECTION("No command")
  {
    const NxInfoRunResult result = runNxInfo({"--format", "JSON"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Errors.find("No command given") != std::string::npos);
  }

  SECTION("Help combined with another option")
  {
    const NxInfoRunResult result = runNxInfo({"--help", "--version"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Output.empty());
    REQUIRE(result.Errors.find("cannot be combined") != std::string::npos);
  }

  SECTION("Version combined with a dump command")
  {
    const NxInfoRunResult result = runNxInfo({"-d", "missing.dream3d", "--version"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Output.empty());
    REQUIRE(result.Errors.find("cannot be combined") != std::string::npos);
  }
}

TEST_CASE("NXInfo::Run rejects repeated options")
{
  SECTION("Dump command")
  {
    const NxInfoRunResult result = runNxInfo({"-d", "first.dream3d", "--dump-data-structure", "second.dream3d"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Errors.find("provided more than once") != std::string::npos);
    REQUIRE(result.Errors.find("'-d'") != std::string::npos);
    REQUIRE(result.Errors.find("'--dump-data-structure'") != std::string::npos);
  }

  SECTION("Output file")
  {
    const NxInfoRunResult result = runNxInfo({"-d", "missing.dream3d", "-o", "first.json", "--output-file", "second.json"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Errors.find("provided more than once") != std::string::npos);
    REQUIRE(result.Errors.find("'-o'") != std::string::npos);
    REQUIRE(result.Errors.find("'--output-file'") != std::string::npos);
  }

  SECTION("Format")
  {
    const NxInfoRunResult result = runNxInfo({"-d", "missing.dream3d", "-f", "JSON", "--format", "TEXT"});
    REQUIRE(result.ExitCode == -101);
    REQUIRE(result.Errors.find("provided more than once") != std::string::npos);
    REQUIRE(result.Errors.find("'-f'") != std::string::npos);
    REQUIRE(result.Errors.find("'--format'") != std::string::npos);
  }
}

TEST_CASE("NXInfo::Run consumes dash-prefixed option values literally")
{
  const NxInfoRunResult result = runNxInfo({"--dump-data-structure", "-missing.dream3d"});

  REQUIRE(result.ExitCode == -102);
  REQUIRE(result.Errors.find("'-missing.dream3d'") != std::string::npos);
}

TEST_CASE("NXInfo::Run reports unusable input paths")
{
  const fs::path inputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / std::string(1024, 'x');
  NxInfoRunResult result;

  REQUIRE_NOTHROW(result = runNxInfo({"-d", inputPath.string()}));
  REQUIRE(result.ExitCode == -102);
  REQUIRE(result.Output.empty());
  REQUIRE(result.Errors.find("Error -102:") != std::string::npos);
  REQUIRE(result.Errors.find(inputPath.string()) != std::string::npos);
  REQUIRE(result.Errors.find("Check the path") != std::string::npos);
}

TEST_CASE("NXInfo::Run loads plugins only before reading valid input")
{
  const fs::path inputPath = createNxInfoInputFile("nxinfo_plugin_loader_test.dream3d");

  SECTION("Valid input")
  {
    std::size_t pluginLoadCallCount = 0;
    std::ostringstream output;
    std::ostringstream errors;
    const nxinfo::LoadPluginsFunctionType countPluginLoads = [&pluginLoadCallCount](std::ostream&) { ++pluginLoadCallCount; };
    const int result = nxinfo::Run({"-d", inputPath.string()}, {.Output = output, .Error = errors}, countPluginLoads);
    REQUIRE(result == 0);
    REQUIRE(pluginLoadCallCount == 1);
  }

  SECTION("Help")
  {
    std::size_t pluginLoadCallCount = 0;
    std::ostringstream output;
    std::ostringstream errors;
    const nxinfo::LoadPluginsFunctionType countPluginLoads = [&pluginLoadCallCount](std::ostream&) { ++pluginLoadCallCount; };
    const int result = nxinfo::Run({"--help"}, {.Output = output, .Error = errors}, countPluginLoads);
    REQUIRE(result == 0);
    REQUIRE(pluginLoadCallCount == 0);
  }

  SECTION("Missing input")
  {
    std::size_t pluginLoadCallCount = 0;
    std::ostringstream output;
    std::ostringstream errors;
    const nxinfo::LoadPluginsFunctionType countPluginLoads = [&pluginLoadCallCount](std::ostream&) { ++pluginLoadCallCount; };
    const int result = nxinfo::Run({"-d", "missing.dream3d"}, {.Output = output, .Error = errors}, countPluginLoads);
    REQUIRE(result == -102);
    REQUIRE(pluginLoadCallCount == 0);
  }
}

TEST_CASE("NXInfo::Run dumps a DREAM3D hierarchy in every format")
{
  const fs::path inputPath = createNxInfoInputFile("nxinfo_formats_test.dream3d");

  SECTION("JSON is the default")
  {
    const NxInfoRunResult result = runNxInfo({"--dump-data-structure", inputPath.string()});
    REQUIRE(result.ExitCode == 0);
    REQUIRE(result.Errors.empty());
    const nlohmann::json output = nlohmann::json::parse(result.Output);
    REQUIRE(output.at("schema_version").get<int>() == 1);
    REQUIRE(output.at("objects").at(0).at("name").get<std::string>() == "Test Group");
  }

  SECTION("TEXT")
  {
    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-f", "text"});
    INFO(result.Errors);
    REQUIRE(result.ExitCode == 0);
    REQUIRE(result.Errors.empty());
    REQUIRE(result.Output.find("|--Test Group") != std::string::npos);
  }

  SECTION("DOT")
  {
    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-f", "DoT"});
    INFO(result.Errors);
    REQUIRE(result.ExitCode == 0);
    REQUIRE(result.Errors.empty());
    REQUIRE(result.Output.find("digraph DataGraph") != std::string::npos);
    REQUIRE(result.Output.find("Test Group") != std::string::npos);
  }
}

TEST_CASE("NXInfo::Run reports DREAM3D read failures")
{
  const fs::path inputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "nxinfo_invalid_input.dream3d";
  {
    std::ofstream output(inputPath, std::ios::trunc);
    output << "not a DREAM3D file";
  }

  const NxInfoRunResult result = runNxInfo({"-d", inputPath.string()});

  REQUIRE(result.ExitCode == -103);
  REQUIRE(result.Output.empty());
  REQUIRE(result.Errors.find(inputPath.string()) != std::string::npos);
  REQUIRE(result.Errors.find("valid DREAM3D file") != std::string::npos);
}

TEST_CASE("NXInfo::Run dumps a legacy DREAM3D hierarchy")
{
  const fs::path inputPath = fs::path(unit_test::k_SourceDir.view()) / "test" / "Data" / "LegacyData.dream3d";
  REQUIRE(fs::is_regular_file(inputPath));

  const NxInfoRunResult result = runNxInfo({"-d", inputPath.string()});

  INFO(result.Errors);
  REQUIRE(result.ExitCode == 0);
  REQUIRE(result.Errors.empty());
  const nlohmann::json hierarchy = nlohmann::json::parse(result.Output);
  const nlohmann::json* neighborListPtr = findNodeByPath(hierarchy, "Small IN100/Grain Data/NeighborList");
  REQUIRE(neighborListPtr != nullptr);
  REQUIRE(neighborListPtr->at("type") == "NeighborList<int32>");
  REQUIRE(neighborListPtr->at("num_tuples").get<uint64>() > 0);
}

TEST_CASE("NXInfo::Run writes output files")
{
  const fs::path inputPath = createNxInfoInputFile("nxinfo_output_test.dream3d");

  SECTION("Successful output")
  {
    const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "nxinfo_unit_test.json";
    {
      std::ofstream existingOutput(outputPath, std::ios::trunc);
      existingOutput << "existing output must be replaced atomically";
    }
    const std::set<fs::path> childrenBefore = listDirectoryChildren(outputPath.parent_path());

    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", outputPath.string()});
    const std::set<fs::path> childrenAfter = listDirectoryChildren(outputPath.parent_path());

    INFO(result.Errors);
    REQUIRE(result.ExitCode == 0);
    REQUIRE(result.Output.empty());
    REQUIRE(result.Errors.empty());
    REQUIRE(childrenAfter == childrenBefore);
    REQUIRE(fs::is_regular_file(outputPath));
    REQUIRE(fs::file_size(outputPath) > 0);
    std::ifstream input(outputPath);
    const nlohmann::json json = nlohmann::json::parse(input);
    REQUIRE(json.at("objects").at(0).at("name").get<std::string>() == "Test Group");
  }

  SECTION("Dash-prefixed output path")
  {
    const fs::path outputPath = "-nxinfo-unit-test.json";
    fs::remove(outputPath);

    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", outputPath.string()});
    std::error_code fileSizeError;
    const auto outputSize = fs::file_size(outputPath, fileSizeError);
    fs::remove(outputPath);

    REQUIRE(result.ExitCode == 0);
    REQUIRE_FALSE(fileSizeError);
    REQUIRE(outputSize > 0);
  }

  SECTION("Unwritable output path")
  {
    const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "missing-directory" / "nxinfo.json";
    fs::remove_all(outputPath.parent_path());

    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", outputPath.string()});

    REQUIRE(result.ExitCode == -104);
    REQUIRE(result.Output.empty());
    REQUIRE(result.Errors.find(outputPath.string()) != std::string::npos);
    REQUIRE(result.Errors.find("Create the directory") != std::string::npos);
  }

  SECTION("Destination is a directory")
  {
    const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "nxinfo_output_directory";
    fs::remove_all(outputPath);
    REQUIRE(fs::create_directory(outputPath));
    const fs::path markerPath = outputPath / "marker.txt";
    {
      std::ofstream marker(markerPath, std::ios::trunc);
      marker << "preserve this directory";
    }

    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", outputPath.string()});

    REQUIRE(result.ExitCode == -104);
    REQUIRE(result.Output.empty());
    REQUIRE(result.Errors.find(outputPath.string()) != std::string::npos);
    REQUIRE(result.Errors.find("existing destination, if any, was preserved") != std::string::npos);
    REQUIRE(fs::is_regular_file(markerPath));
    fs::remove_all(outputPath);
  }
}

TEST_CASE("NXInfo::Run reports stream write failures")
{
  const fs::path inputPath = createNxInfoInputFile("nxinfo_stream_failure_test.dream3d");

  SECTION("Hierarchy dump")
  {
    FailingStreamBuffer outputBuffer;
    std::ostream output(&outputBuffer);
    std::ostringstream errors;

    const int result = nxinfo::Run({"-d", inputPath.string()}, {.Output = output, .Error = errors});

    REQUIRE(result == -104);
    REQUIRE(errors.str().find("standard output") != std::string::npos);
  }

  SECTION("Help")
  {
    FailingStreamBuffer outputBuffer;
    std::ostream output(&outputBuffer);
    std::ostringstream errors;

    const int result = nxinfo::Run({"--help"}, {.Output = output, .Error = errors});

    REQUIRE(result == -104);
    REQUIRE(errors.str().find("standard output") != std::string::npos);
  }

  SECTION("Version")
  {
    FailingStreamBuffer outputBuffer;
    std::ostream output(&outputBuffer);
    std::ostringstream errors;

    const int result = nxinfo::Run({"--version"}, {.Output = output, .Error = errors});

    REQUIRE(result == -104);
    REQUIRE(errors.str().find("standard output") != std::string::npos);
  }
}

TEST_CASE("NXInfo::Run preserves the input when the output identifies the same file")
{
  const fs::path inputPath = createNxInfoInputFile("nxinfo_same_file_test.dream3d");
  const auto originalSize = fs::file_size(inputPath);

  SECTION("Identical path")
  {
    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", inputPath.string()});
    REQUIRE(result.ExitCode == -104);
    REQUIRE(result.Errors.find("identify the same file") != std::string::npos);
  }

  SECTION("Equivalent path")
  {
    const fs::path equivalentPath = inputPath.parent_path() / "." / inputPath.filename();
    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", equivalentPath.string()});
    REQUIRE(result.ExitCode == -104);
    REQUIRE(result.Errors.find("identify the same file") != std::string::npos);
  }

  SECTION("Hard link")
  {
    const fs::path hardLinkPath = inputPath.parent_path() / "nxinfo_same_file_hard_link.dream3d";
    fs::remove(hardLinkPath);
    std::error_code hardLinkError;
    fs::create_hard_link(inputPath, hardLinkPath, hardLinkError);
    REQUIRE_FALSE(hardLinkError);

    const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", hardLinkPath.string()});
    fs::remove(hardLinkPath);

    REQUIRE(result.ExitCode == -104);
    REQUIRE(result.Errors.find("identify the same file") != std::string::npos);
  }

  SECTION("Symbolic link")
  {
    const fs::path symbolicLinkPath = inputPath.parent_path() / "nxinfo_same_file_symbolic_link.dream3d";
    fs::remove(symbolicLinkPath);
    std::error_code symbolicLinkError;
    fs::create_symlink(inputPath, symbolicLinkPath, symbolicLinkError);
    if(symbolicLinkError)
    {
      WARN("The platform did not permit symbolic-link creation: " << symbolicLinkError.message());
    }
    else
    {
      const NxInfoRunResult result = runNxInfo({"-d", inputPath.string(), "-o", symbolicLinkPath.string()});
      fs::remove(symbolicLinkPath);

      REQUIRE(result.ExitCode == -104);
      REQUIRE(result.Errors.find("identify the same file") != std::string::npos);
    }
  }

  REQUIRE(fs::file_size(inputPath) == originalSize);
  const Result<DataStructure> readResult = DREAM3D::ImportDataStructureFromFile(inputPath, true);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
}
