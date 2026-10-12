#include "NxInfo.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <H5Epublic.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace nx::core::nxinfo
{
namespace
{
// Exit codes. Keep in sync with src/apps/docs/NX_INFO.md.
constexpr int32 k_NoArgumentsProvided = -100;
constexpr int32 k_FailedParsingArguments = -101;
constexpr int32 k_InputFileMissing = -102;
constexpr int32 k_FailedReadingDataStructure = -103;
constexpr int32 k_OutputFailure = -104;

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

enum class OutputFormat : uint8
{
  Json,
  Text,
  Dot
};

enum class Command : uint8
{
  Help,
  Version,
  DumpDataStructure
};

struct CliOptions
{
  std::optional<Command> SelectedCommand;
  fs::path InputFile;
  std::optional<fs::path> OutputFile;
  OutputFormat Format = OutputFormat::Json;
  std::optional<std::string> DumpFlag;
  std::optional<std::string> OutputFlag;
  std::optional<std::string> FormatFlag;
};

/**
 * @class ScopedHdf5ErrorHandler
 * @brief Suppresses native HDF5 diagnostics and restores the prior handler.
 */
class ScopedHdf5ErrorHandler
{
public:
  /**
   * @brief Saves and disables the current thread's HDF5 error handler.
   */
  ScopedHdf5ErrorHandler()
  : m_IsActive(H5Eget_auto2(H5E_DEFAULT, &m_ErrorFunction, &m_ClientDataPtr) >= 0)
  {
    if(m_IsActive && H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr) < 0)
    {
      m_IsActive = false;
    }
  }

  /**
   * @brief Restores the saved HDF5 error handler.
   */
  ~ScopedHdf5ErrorHandler() noexcept
  {
    if(m_IsActive)
    {
      H5Eset_auto2(H5E_DEFAULT, m_ErrorFunction, m_ClientDataPtr);
    }
  }

  ScopedHdf5ErrorHandler(const ScopedHdf5ErrorHandler&) = delete;
  ScopedHdf5ErrorHandler(ScopedHdf5ErrorHandler&&) = delete;
  ScopedHdf5ErrorHandler& operator=(const ScopedHdf5ErrorHandler&) = delete;
  ScopedHdf5ErrorHandler& operator=(ScopedHdf5ErrorHandler&&) = delete;

private:
  H5E_auto2_t m_ErrorFunction = nullptr;
  void* m_ClientDataPtr = nullptr;
  bool m_IsActive = false;
};

/**
 * @brief Writes command usage.
 * @param outputStream Stream that receives the usage text.
 */
void printUsage(std::ostream& outputStream)
{
  outputStream << "Usage:\n"
               << "  nxinfo --dump-data-structure <file.dream3d> [--output-file <path>] [--format JSON|TEXT|DOT]\n"
               << "  nxinfo --help | -h\n"
               << "  nxinfo --version | -v\n"
               << "\n"
               << "Commands:\n"
               << "  -d, --dump-data-structure <file>  Dump the DataStructure hierarchy of a .dream3d file.\n"
               << "                                    Most numeric array values are not loaded.\n"
               << "                                    Some legacy geometry, NeighborList, and Statistics payloads may load.\n"
               << "                                    Legacy StringArrays allocate one empty placeholder per tuple.\n"
               << "  -o, --output-file <path>          Write the dump to <path> instead of stdout.\n"
               << "  -f, --format <JSON|TEXT|DOT>      Output format (case-insensitive). Default: JSON.\n"
               << "  -h, --help                        Show this help.\n"
               << "  -v, --version                     Show version and build date.\n"
               << "\n"
               << "Each dump, output, and format option may be specified only once.\n"
               << "Help and version options must be used by themselves.\n"
               << "Values beginning with '-' are consumed literally.\n"
               << "\n"
               << "Example:\n"
               << "  nxinfo -d /data/SmallIN100.dream3d -f JSON -o /tmp/SmallIN100.json\n";
}

/**
 * @brief Writes the package version and build date.
 * @param outputStream Stream that receives the version text.
 */
void printVersion(std::ostream& outputStream)
{
  outputStream << fmt::format("nxinfo: Version {} Build Date: {}\n", Version::Package(), Version::BuildDate());
}

/**
 * @brief Flushes standard output and reports a write failure.
 * @param streams Success and diagnostic output streams.
 * @return Zero when standard output is healthy, or the output-failure code.
 */
int finishStandardOutput(OutputStreams streams)
{
  streams.Output.get().flush();
  if(streams.Output.get().good())
  {
    return 0;
  }

  streams.Error.get() << fmt::format("Error {}: Failed to write to standard output. Check that the receiving process or output destination is writable.\n", k_OutputFailure);
  return k_OutputFailure;
}

/**
 * @brief Loads plugins from the same paths as nxrunner.
 * @param errorStream Stream that receives plugin-loading diagnostics.
 */
void loadApp(std::ostream& errorStream)
{
  const auto printLoadWarnings = [&errorStream](const fs::path& pluginPath, const Result<>& result) {
    errorStream << fmt::format("Warning: Failed to load one or more plugins from '{}'. Some file types may be unavailable.\n", pluginPath.string());
    for(const auto& error : result.errors())
    {
      errorStream << fmt::format("Warning {}: {}\n", error.code, error.message);
    }
  };

  const auto appPtr = Application::GetOrCreateInstance();
  const fs::path appPath = appPtr->getCurrentDir();
  // Disable verbose loading so successful stdout contains only the requested dump.
  const auto appDirectoryResult = appPtr->loadPlugins(appPath, false);
  if(appDirectoryResult.invalid())
  {
    printLoadWarnings(appPath, appDirectoryResult);
  }

#ifndef _WIN32
  const fs::path pluginPath = appPath.parent_path() / "Plugins";
  std::error_code pluginStatusError;
  const bool pluginDirectoryExists = fs::is_directory(pluginPath, pluginStatusError);
  if(pluginStatusError)
  {
    errorStream << fmt::format("Warning: Could not inspect plugin directory '{}': {}. Some file types may be unavailable.\n", pluginPath.string(), pluginStatusError.message());
  }
  else if(pluginDirectoryExists)
  {
    const auto pluginDirectoryResult = appPtr->loadPlugins(pluginPath, false);
    if(pluginDirectoryResult.invalid())
    {
      printLoadWarnings(pluginPath, pluginDirectoryResult);
    }
  }
#endif
}

// Result success values use inherited aggregate state. Designated initializers cannot name inherited members.
// NOLINTBEGIN(modernize-use-designated-initializers)
/**
 * @brief Parses a case-insensitive output-format name.
 * @param value Format name supplied by the user.
 * @return Parsed format or an argument error listing the valid formats.
 */
Result<OutputFormat> parseFormat(const std::string& value)
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
 * @brief Consumes the value following an option.
 * @param arguments Command-line arguments excluding the executable name.
 * @param index Index of the option; advanced to its value on success.
 * @param flag Option spelling used in diagnostics.
 * @return The option value or a missing-value error.
 */
Result<std::string> takeValue(const std::vector<std::string>& arguments, usize& index, const std::string& flag)
{
  if(index + 1 >= arguments.size())
  {
    return MakeErrorResult<std::string>(k_FailedParsingArguments, fmt::format("Missing value after '{}'.", flag));
  }
  return {arguments[++index]};
}

/**
 * @brief Records the first spelling of an option and rejects repeated use.
 * @param firstFlag Receives the first option spelling.
 * @param currentFlag Current option spelling.
 * @return Valid result for first use or an argument error for repeated use.
 */
Result<> recordOption(std::optional<std::string>& firstFlag, const std::string& currentFlag)
{
  if(firstFlag.has_value())
  {
    return MakeErrorResult(k_FailedParsingArguments, fmt::format("Option '{}' was provided more than once (first as '{}'). Provide each option only once.", currentFlag, firstFlag.value()));
  }
  firstFlag = currentFlag;
  return {};
}

/**
 * @brief Parses and validates nxinfo command-line arguments.
 * @param arguments Command-line arguments excluding the executable name.
 * @return Parsed options or a detailed argument error.
 */
Result<CliOptions> parseArguments(const std::vector<std::string>& arguments)
{
  if(arguments.empty())
  {
    return MakeErrorResult<CliOptions>(k_NoArgumentsProvided, "No arguments provided.");
  }

  CliOptions options;
  for(usize index = 0; index < arguments.size(); ++index)
  {
    const std::string& arg = arguments[index];

    if(arg == k_HelpParamLong || arg == k_HelpParamShort)
    {
      if(arguments.size() != 1)
      {
        return MakeErrorResult<CliOptions>(k_FailedParsingArguments, fmt::format("Option '{}' cannot be combined with other arguments. Use it by itself.", arg));
      }
      options.SelectedCommand = Command::Help;
      return {std::move(options)};
    }
    if(arg == k_VersionParamLong || arg == k_VersionParamShort)
    {
      if(arguments.size() != 1)
      {
        return MakeErrorResult<CliOptions>(k_FailedParsingArguments, fmt::format("Option '{}' cannot be combined with other arguments. Use it by itself.", arg));
      }
      options.SelectedCommand = Command::Version;
      return {std::move(options)};
    }
    if(arg == k_DumpParamLong || arg == k_DumpParamShort)
    {
      auto recordResult = recordOption(options.DumpFlag, arg);
      if(recordResult.invalid())
      {
        return ConvertInvalidResult<CliOptions>(std::move(recordResult));
      }
      auto value = takeValue(arguments, index, arg);
      if(value.invalid())
      {
        return ConvertInvalidResult<CliOptions>(std::move(value));
      }
      options.SelectedCommand = Command::DumpDataStructure;
      options.InputFile = std::move(value.value());
      continue;
    }
    if(arg == k_OutputParamLong || arg == k_OutputParamShort)
    {
      auto recordResult = recordOption(options.OutputFlag, arg);
      if(recordResult.invalid())
      {
        return ConvertInvalidResult<CliOptions>(std::move(recordResult));
      }
      auto value = takeValue(arguments, index, arg);
      if(value.invalid())
      {
        return ConvertInvalidResult<CliOptions>(std::move(value));
      }
      options.OutputFile.emplace(std::move(value.value()));
      continue;
    }
    if(arg == k_FormatParamLong || arg == k_FormatParamShort)
    {
      auto recordResult = recordOption(options.FormatFlag, arg);
      if(recordResult.invalid())
      {
        return ConvertInvalidResult<CliOptions>(std::move(recordResult));
      }
      auto value = takeValue(arguments, index, arg);
      if(value.invalid())
      {
        return ConvertInvalidResult<CliOptions>(std::move(value));
      }
      auto format = parseFormat(value.value());
      if(format.invalid())
      {
        return ConvertInvalidResult<CliOptions>(std::move(format));
      }
      options.Format = format.value();
      continue;
    }

    return MakeErrorResult<CliOptions>(k_FailedParsingArguments, fmt::format("Unknown argument '{}'.", arg));
  }

  if(!options.SelectedCommand.has_value())
  {
    return MakeErrorResult<CliOptions>(k_FailedParsingArguments, "No command given. Expected '--dump-data-structure <file>'.");
  }

  return {std::move(options)};
}
// NOLINTEND(modernize-use-designated-initializers)

/**
 * @brief Serializes a DataStructure hierarchy in the selected format.
 * @param dataStructure Data structure to serialize.
 * @param format Output format.
 * @param outputStream Stream that receives the serialized hierarchy.
 * @return True if the stream accepts and flushes the complete hierarchy.
 */
bool writeDump(const DataStructure& dataStructure, OutputFormat format, std::ostream& outputStream)
{
  switch(format)
  {
  case OutputFormat::Json:
    outputStream << std::setw(2) << dataStructure.exportHierarchyAsJson() << '\n';
    break;
  case OutputFormat::Text:
    dataStructure.exportHierarchyAsText(outputStream);
    break;
  case OutputFormat::Dot:
    dataStructure.exportHierarchyAsGraphViz(outputStream);
    break;
  }
  outputStream.flush();
  return outputStream.good();
}

/**
 * @brief Writes Result errors using the nxinfo diagnostic format.
 * @param errors Errors to write.
 * @param errorStream Stream that receives the diagnostics.
 * @return The first error code, or the generic argument-error code for an empty list.
 */
int printErrors(const std::vector<Error>& errors, std::ostream& errorStream)
{
  for(const auto& error : errors)
  {
    errorStream << fmt::format("Error {}: {}\n", error.code, error.message);
  }
  return errors.empty() ? k_FailedParsingArguments : errors.front().code;
}

/**
 * @brief Loads and serializes the requested DREAM3D file.
 * @param options Validated dump options.
 * @param streams Success and diagnostic output streams.
 * @param loadPluginsFunction Function called before the DREAM3D input is read.
 * @return Zero on success or an input/output error code.
 */
int runDumpDataStructure(const CliOptions& options, OutputStreams streams, const LoadPluginsFunctionType& loadPluginsFunction)
{
  std::error_code inputStatusError;
  const fs::file_status inputStatus = fs::status(options.InputFile, inputStatusError);
  if(inputStatusError)
  {
    if(inputStatusError == std::errc::no_such_file_or_directory)
    {
      streams.Error.get() << fmt::format("Error {}: Input file '{}' does not exist. Check the path and try again.\n", k_InputFileMissing, options.InputFile.string());
      return k_InputFileMissing;
    }
    streams.Error.get() << fmt::format("Error {}: Could not inspect input file '{}': {}. Check the path and permissions.\n", k_InputFileMissing, options.InputFile.string(),
                                       inputStatusError.message());
    return k_InputFileMissing;
  }
  if(!fs::is_regular_file(inputStatus))
  {
    streams.Error.get() << fmt::format("Error {}: Input path '{}' is not a regular file. Select a DREAM3D file and try again.\n", k_InputFileMissing, options.InputFile.string());
    return k_InputFileMissing;
  }
  if(options.OutputFile.has_value())
  {
    const fs::path& outputPath = *options.OutputFile;
    const fs::path outputDirectory = outputPath.has_parent_path() ? outputPath.parent_path() : fs::path{"."};
    std::error_code outputDirectoryError;
    const bool outputDirectoryExists = fs::is_directory(outputDirectory, outputDirectoryError);
    if(outputDirectoryError || !outputDirectoryExists)
    {
      const std::string cause = outputDirectoryError ? fmt::format(": {}", outputDirectoryError.message()) : "";
      streams.Error.get() << fmt::format("Error {}: Output file '{}' cannot be created because directory '{}' is not available{}. Create the directory or choose another output path.\n",
                                         k_OutputFailure, outputPath.string(), outputDirectory.string(), cause);
      return k_OutputFailure;
    }

    std::error_code sameFileError;
    const bool pathsIdentifySameFile = fs::equivalent(options.InputFile, outputPath, sameFileError);
    if(!sameFileError && pathsIdentifySameFile)
    {
      streams.Error.get() << fmt::format("Error {}: Input file '{}' and output file '{}' identify the same file. Choose a different output path.\n", k_OutputFailure, options.InputFile.string(),
                                         outputPath.string());
      return k_OutputFailure;
    }
  }

  loadPluginsFunction(streams.Error.get());

  const Result<DataStructure> readResult = [&options] {
    const ScopedHdf5ErrorHandler scopedHdf5ErrorHandler;
    return DREAM3D::ImportDataStructureFromFile(options.InputFile, /*preflight=*/true);
  }();
  if(readResult.invalid())
  {
    streams.Error.get() << fmt::format("Error {}: Failed to read the DataStructure from '{}'. Confirm that it is a valid DREAM3D file.\n", k_FailedReadingDataStructure, options.InputFile.string());
    printErrors(readResult.errors(), streams.Error.get());
    return k_FailedReadingDataStructure;
  }

  if(options.OutputFile.has_value())
  {
    const fs::path& outputPath = *options.OutputFile;
    auto atomicFileResult = AtomicFile::Create(outputPath);
    if(atomicFileResult.invalid())
    {
      streams.Error.get() << fmt::format("Error {}: Could not prepare output file '{}'. Check the path and write permissions.\n", k_OutputFailure, outputPath.string());
      printErrors(atomicFileResult.errors(), streams.Error.get());
      return k_OutputFailure;
    }
    AtomicFile atomicFile = std::move(atomicFileResult.value());

    std::ofstream outputFileStream(atomicFile.tempFilePath(), std::ios_base::out | std::ios_base::trunc);
    if(!outputFileStream.is_open())
    {
      streams.Error.get() << fmt::format("Error {}: Failed to open temporary output for '{}'. Check available disk space and write permissions.\n", k_OutputFailure, outputPath.string());
      return k_OutputFailure;
    }
    const bool writeSucceeded = writeDump(readResult.value(), options.Format, outputFileStream);
    outputFileStream.close();
    if(!writeSucceeded || outputFileStream.fail())
    {
      streams.Error.get() << fmt::format("Error {}: Failed to write output file '{}'. Check available disk space and permissions.\n", k_OutputFailure, outputPath.string());
      return k_OutputFailure;
    }

    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      streams.Error.get() << fmt::format("Error {}: Failed to replace output file '{}'. The existing destination, if any, was preserved.\n", k_OutputFailure, outputPath.string());
      printErrors(commitResult.errors(), streams.Error.get());
      return k_OutputFailure;
    }
    return 0;
  }

  if(!writeDump(readResult.value(), options.Format, streams.Output.get()))
  {
    streams.Error.get() << fmt::format("Error {}: Failed to write the DataStructure hierarchy to standard output. Check that the receiving process or output destination is writable.\n",
                                       k_OutputFailure);
    return k_OutputFailure;
  }
  return 0;
}
} // namespace

int Run(const std::vector<std::string>& arguments, OutputStreams streams, const LoadPluginsFunctionType& loadPluginsFunction)
{
  const Result<CliOptions> parsed = parseArguments(arguments);
  if(parsed.invalid())
  {
    const int code = printErrors(parsed.errors(), streams.Error.get());
    printUsage(streams.Error.get());
    return code;
  }

  const CliOptions& options = parsed.value();
  if(!options.SelectedCommand.has_value())
  {
    streams.Error.get() << fmt::format("Error {}: No command was selected. Use '--help' to list available commands.\n", k_FailedParsingArguments);
    return k_FailedParsingArguments;
  }
  if(*options.SelectedCommand == Command::Help)
  {
    printUsage(streams.Output.get());
    return finishStandardOutput(streams);
  }
  if(*options.SelectedCommand == Command::Version)
  {
    printVersion(streams.Output.get());
    return finishStandardOutput(streams);
  }
  return runDumpDataStructure(options, streams, loadPluginsFunction);
}

int Run(const std::vector<std::string>& arguments, OutputStreams streams)
{
  return Run(arguments, streams, loadApp);
}
} // namespace nx::core::nxinfo
