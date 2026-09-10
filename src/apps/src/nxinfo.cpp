/**
 * @file nxinfo.cpp
 * @brief Command line tool that reports information about simplnx related files.
 *
 * The first supported command dumps the DataStructure hierarchy of a
 * .dream3d file as JSON (default), plain text, or GraphViz DOT. The file is
 * opened in preflight mode so only metadata is read; array values are never
 * loaded, which keeps the cost independent of file size.
 */

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// Exit codes. Keep in sync with src/apps/docs/NX_INFO.md.
constexpr int32 k_NoArgumentsProvided = -100;
constexpr int32 k_FailedParsingArguments = -101;
constexpr int32 k_InputFileMissing = -102;
constexpr int32 k_FailedReadingDataStructure = -103;
constexpr int32 k_FailedOpeningOutputFile = -104; // Failed opening or writing the output file.

constexpr StringLiteral k_HelpParamLong = "--help";
constexpr StringLiteral k_HelpParamShort = "-h";
constexpr StringLiteral k_VersionParamLong = "--version";
constexpr StringLiteral k_VersionParamShort = "-v";
constexpr StringLiteral k_DumpParamLong = "--dump-data-structure";
constexpr StringLiteral k_DumpParamShort = "-d";
constexpr StringLiteral k_OutputParamLong = "--output-file";
constexpr StringLiteral k_OutputParamShort = "-o";
constexpr StringLiteral k_FormatParamLong = "--format";
constexpr StringLiteral k_FormatParamShort = "-f";

enum class OutputFormat
{
  Json,
  Text,
  Dot
};

enum class Command
{
  None,
  Help,
  Version,
  DumpDataStructure
};

struct CliOptions
{
  Command command = Command::None;
  fs::path inputFile;
  std::optional<fs::path> outputFile;
  OutputFormat format = OutputFormat::Json;
};

void PrintUsage(std::ostream& out)
{
  out << "Usage:\n"
      << "  nxinfo --dump-data-structure <file.dream3d> [--output-file <path>] [--format JSON|TEXT|DOT]\n"
      << "  nxinfo --help | -h\n"
      << "  nxinfo --version | -v\n"
      << "\n"
      << "Commands:\n"
      << "  -d, --dump-data-structure <file>  Dump the DataStructure hierarchy of a .dream3d file.\n"
      << "                                    Only metadata is read; array values are never loaded.\n"
      << "  -o, --output-file <path>          Write the dump to <path> instead of stdout.\n"
      << "  -f, --format <JSON|TEXT|DOT>      Output format (case-insensitive). Default: JSON.\n"
      << "  -h, --help                        Show this help.\n"
      << "  -v, --version                     Show version and build date.\n"
      << "\n"
      << "Example:\n"
      << "  nxinfo -d /data/SmallIN100.dream3d -f JSON -o /tmp/SmallIN100.json\n";
}

void PrintVersion(std::ostream& out)
{
  out << fmt::format("nxinfo: Version {} Build Date:{}\n", nx::core::Version::Package(), nx::core::Version::BuildDate());
}

/**
 * @brief Loads plugins from the same paths as nxrunner.
 */
void LoadApp()
{
  auto app = Application::GetOrCreateInstance();
  fs::path appPath = app->getCurrentDir();
  // Disable verbose loading so successful stdout contains only the requested dump.
  auto result = app->loadPlugins(appPath, false);
  if(result.invalid())
  {
    fmt::print(stderr, "Error loading plugins from '{}'\n", appPath.string());
  }

#ifndef _MSC_VER
  {
    appPath = appPath.parent_path();
    if(fs::exists(appPath / "Plugins"))
    {
      appPath = appPath / "Plugins";
      result = app->loadPlugins(appPath, false);
      if(result.invalid())
      {
        fmt::print(stderr, "Error loading plugins from '{}'\n", appPath.string());
      }
    }
  }
#endif
}

/**
 * @brief Re-wraps the first error of a failed Result<From> as a Result<To>.
 * Used because the Result value types here are unrelated, so ConvertResultTo
 * cannot be applied.
 */
template <typename To, typename From>
Result<To> ForwardError(const Result<From>& from)
{
  const Error& error = from.errors().front();
  return MakeErrorResult<To>(error.code, error.message);
}

Result<OutputFormat> ParseFormat(const std::string& value)
{
  const std::string upper = StringUtilities::toUpper(value);
  if(upper == "JSON")
  {
    return {OutputFormat::Json};
  }
  if(upper == "TEXT")
  {
    return {OutputFormat::Text};
  }
  if(upper == "DOT")
  {
    return {OutputFormat::Dot};
  }
  return MakeErrorResult<OutputFormat>(k_FailedParsingArguments, fmt::format("Unknown format '{}'. Expected one of JSON, TEXT, DOT.", value));
}

/**
 * @brief Returns the value following argv[index], or an error when it is
 * missing or looks like another flag.
 */
Result<std::string> TakeValue(int argc, char* argv[], int& index, const std::string& flag)
{
  if(index + 1 >= argc)
  {
    return MakeErrorResult<std::string>(k_FailedParsingArguments, fmt::format("Missing value after '{}'", flag));
  }
  std::string value = argv[++index];
  if(!value.empty() && value[0] == '-')
  {
    return MakeErrorResult<std::string>(k_FailedParsingArguments, fmt::format("Missing value after '{}' (found flag '{}')", flag, value));
  }
  return {std::move(value)};
}

Result<CliOptions> ParseArguments(int argc, char* argv[])
{
  if(argc < 2)
  {
    return MakeErrorResult<CliOptions>(k_NoArgumentsProvided, "No arguments provided");
  }

  CliOptions options;
  for(int index = 1; index < argc; ++index)
  {
    const std::string arg = argv[index];

    if(arg == k_HelpParamLong || arg == k_HelpParamShort)
    {
      options.command = Command::Help;
      return {std::move(options)};
    }
    if(arg == k_VersionParamLong || arg == k_VersionParamShort)
    {
      options.command = Command::Version;
      return {std::move(options)};
    }
    if(arg == k_DumpParamLong || arg == k_DumpParamShort)
    {
      auto value = TakeValue(argc, argv, index, arg);
      if(value.invalid())
      {
        return ForwardError<CliOptions>(value);
      }
      options.command = Command::DumpDataStructure;
      options.inputFile = value.value();
      continue;
    }
    if(arg == k_OutputParamLong || arg == k_OutputParamShort)
    {
      auto value = TakeValue(argc, argv, index, arg);
      if(value.invalid())
      {
        return ForwardError<CliOptions>(value);
      }
      options.outputFile = fs::path(value.value());
      continue;
    }
    if(arg == k_FormatParamLong || arg == k_FormatParamShort)
    {
      auto value = TakeValue(argc, argv, index, arg);
      if(value.invalid())
      {
        return ForwardError<CliOptions>(value);
      }
      auto format = ParseFormat(value.value());
      if(format.invalid())
      {
        return ForwardError<CliOptions>(format);
      }
      options.format = format.value();
      continue;
    }

    return MakeErrorResult<CliOptions>(k_FailedParsingArguments, fmt::format("Unknown argument '{}'", arg));
  }

  if(options.command == Command::None)
  {
    return MakeErrorResult<CliOptions>(k_FailedParsingArguments, "No command given. Expected --dump-data-structure <file>.");
  }

  return {std::move(options)};
}

void WriteDump(const DataStructure& dataStructure, OutputFormat format, std::ostream& out)
{
  switch(format)
  {
  case OutputFormat::Json:
    out << dataStructure.exportHierarchyAsJson().dump(2) << '\n';
    break;
  case OutputFormat::Text:
    dataStructure.exportHierarchyAsText(out);
    break;
  case OutputFormat::Dot:
    dataStructure.exportHierarchyAsGraphViz(out);
    break;
  }
}

int PrintErrors(const std::vector<Error>& errors)
{
  for(const auto& error : errors)
  {
    fmt::print(stderr, "Error {}: {}\n", error.code, error.message);
  }
  return errors.empty() ? k_FailedParsingArguments : errors.front().code;
}

int RunDumpDataStructure(const CliOptions& options)
{
  if(!fs::exists(options.inputFile) || !fs::is_regular_file(options.inputFile))
  {
    fmt::print(stderr, "Error {}: Input file does not exist or is not a regular file: '{}'\n", k_InputFileMissing, options.inputFile.string());
    return k_InputFileMissing;
  }

  LoadApp();

  Result<DataStructure> readResult = DREAM3D::ImportDataStructureFromFile(options.inputFile, /*preflight=*/true);
  if(readResult.invalid())
  {
    fmt::print(stderr, "Error {}: Failed to read DataStructure from '{}'\n", k_FailedReadingDataStructure, options.inputFile.string());
    PrintErrors(readResult.errors());
    return k_FailedReadingDataStructure;
  }

  if(options.outputFile.has_value())
  {
    std::ofstream outFile(options.outputFile.value(), std::ios_base::out | std::ios_base::trunc);
    if(!outFile.is_open())
    {
      fmt::print(stderr, "Error {}: Failed to open output file '{}'\n", k_FailedOpeningOutputFile, options.outputFile.value().string());
      return k_FailedOpeningOutputFile;
    }
    WriteDump(readResult.value(), options.format, outFile);
    outFile.flush();
    if(!outFile.good())
    {
      fmt::print(stderr, "Error {}: Failed to write output file '{}'\n", k_FailedOpeningOutputFile, options.outputFile.value().string());
      return k_FailedOpeningOutputFile;
    }
    return 0;
  }

  WriteDump(readResult.value(), options.format, std::cout);
  return 0;
}
} // namespace

int main(int argc, char* argv[])
{
  Result<CliOptions> parsed = ParseArguments(argc, argv);
  if(parsed.invalid())
  {
    const int code = PrintErrors(parsed.errors());
    PrintUsage(std::cerr);
    return code;
  }

  const CliOptions& options = parsed.value();
  switch(options.command)
  {
  case Command::Help:
    PrintUsage(std::cout);
    return 0;
  case Command::Version:
    PrintVersion(std::cout);
    return 0;
  case Command::DumpDataStructure:
    return RunDumpDataStructure(options);
  case Command::None:
    break;
  }

  PrintUsage(std::cerr);
  return k_FailedParsingArguments;
}
