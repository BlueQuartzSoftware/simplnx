#include "CliObserver.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/SimplnxPython.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/TimeUtilities.hpp"

#include <fmt/format.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#if SIMPLNX_EMBED_PYTHON
#include "NxPythonEmbed/NxPythonEmbed.hpp"

#include <pybind11/embed.h>
#endif

namespace fs = std::filesystem;
using namespace nx::core;

#if SIMPLNX_EMBED_PYTHON
namespace py = pybind11;
#endif

namespace
{
constexpr int32 k_FailedLoadingPipeline = -100;
constexpr int32 k_FailedExecutingPipeline = -101;
constexpr int32 k_FailedPreflightingPipeline = -102;
constexpr int32 k_NoArgumentsProvided = -103;
constexpr int32 k_FailedParsingArguments = -105;
constexpr int32 k_ExecutePipelineError = -110;
constexpr int32 k_PreflightPipelineError = -110;
constexpr int32 k_InvalidArgumentError = -120;
constexpr int32 k_LogFileError = -121;
constexpr int32 k_NullLogFileError = -122;

constexpr StringLiteralType k_HelpParamLong = "--help";
constexpr StringLiteralType k_ExecuteParamLong = "--execute";
constexpr StringLiteralType k_PreflightParamLong = "--preflight";
constexpr StringLiteralType k_LogFileParamLong = "--logfile";
constexpr StringLiteralType k_ConvertParamLong = "--convert";

constexpr StringLiteralType k_HelpParamShort = "-h";
constexpr StringLiteralType k_ExecuteParamShort = "-e";
constexpr StringLiteralType k_PreflightParamShort = "-p";
constexpr StringLiteralType k_LogFileParamShort = "-l";
constexpr StringLiteralType k_ConvertParamShort = "-c";
constexpr StringLiteralType k_CacheMemoryBudgetParamLong = "--cache-memory-budget";

/**
 * @brief Loads plugins from nxrunner build and application package locations.
 *
 * The first location supports developer and CI build trees. On non-Windows
 * platforms, the second location supports the Plugins directory in an
 * application package.
 */
void LoadApp()
{
  auto app = Application::GetOrCreateInstance();
  // The executable directory is the plugin location in developer and CI build trees.
  fs::path appPath = app->getCurrentDir();
  auto result = app->loadPlugins(appPath, true);
  if(result.invalid())
  {
    fmt::print(stderr, "Error loading plugins from '{}'\n", appPath.string());
  }

  // Non-Windows packages keep plugins in a sibling Plugins directory.
#ifndef _MSC_VER
  {
    appPath = appPath.parent_path();
    // Do not report an error when the optional package directory does not exist.
    if(fs::exists(appPath / "Plugins"))
    {
      appPath = appPath / "Plugins";
      result = app->loadPlugins(appPath, true);
      if(result.invalid())
      {
        fmt::print(stderr, "Error loading plugins from '{}'\n", appPath.string());
      }
    }
  }
#endif
}

/**
 * @class CliStream
 * @brief Mirrors nxrunner output to stdout and an optional log file.
 *
 * The stream always writes to stdout. After setLogFile() succeeds, each later
 * value and line terminator also writes to the truncated log file.
 */
class CliStream
{
public:
  CliStream() = default;
  ~CliStream() noexcept = default;

  /**
   * @brief Opens a log file and truncates its existing content.
   * @param filepath Nonempty destination path.
   * @return An error if the path is empty or the file cannot open.
   */
  Result<> setLogFile(const std::filesystem::path& filepath)
  {
    if(filepath.empty() || filepath.string().empty())
    {
      std::string errorMessage = "Log file cannot be created with an empty filepath.";
      return nx::core::MakeErrorResult(k_NullLogFileError, errorMessage);
    }

    const std::ios_base::openmode openmode = std::ios_base::out | std::ios_base::trunc;
    m_LogStream = std::fstream(filepath, openmode);
    if(!m_LogStream.is_open())
    {
      std::string errorMessage = fmt::format("Failed to open log file: '{}'", filepath.string());
      return nx::core::MakeErrorResult(k_LogFileError, errorMessage);
    }

    return {};
  }

  /**
   * @brief Writes one value to stdout and the open log file.
   * @tparam T Specifies the value type.
   * @param value Value to write.
   * @return This stream.
   */
  template <typename T>
  CliStream& operator<<(const T& value)
  {
    std::cout << value;
    if(m_LogStream.is_open())
    {
      m_LogStream << value;
    }
    return *this;
  }
  /**
   * @brief Writes and flushes a line terminator to each active stream.
   * @return This stream.
   */
  CliStream& endline()
  {
    std::cout << '\n';
    if(m_LogStream.is_open())
    {
      m_LogStream << '\n';
    }
    return *this;
  }

private:
  std::fstream m_LogStream;
};

CliStream CliOut;

/**
 * @enum ArgumentType
 * @brief Identifies an nxrunner command-line operation.
 */
enum class ArgumentType
{
  Invalid,          ///< Marks an unsupported operand.
  Execute,          ///< Runs a pipeline.
  Preflight,        ///< Preflights a pipeline.
  Help,             ///< Displays command help.
  Logfile,          ///< Selects a log file.
  Convert,          ///< Converts a legacy pipeline.
  CacheMemoryBudget ///< Overrides the cache budget for this process.
};

/**
 * @struct Argument
 * @brief Stores one parsed command-line operand and its optional value.
 */
struct Argument
{
  ArgumentType type;
  std::string value;

  /**
   * @brief Creates a parsed argument.
   * @param targetType Operand type.
   * @param targetValue Operand value, or an empty string when no value exists.
   */
  Argument(ArgumentType targetType, std::string targetValue = "")
  : type(targetType)
  , value(std::move(targetValue))
  {
  }
};

using CliArgumentsType = std::vector<Argument>;

/**
 * @brief Copies one command-line token to a string.
 * @param argc Number of command-line tokens.
 * @param argv Command-line token array.
 * @param index Token index.
 * @return The selected token, or an empty string when index is greater than argc.
 * @pre index must be less than argc for safe access.
 */
std::string ToString(int argc, char* argv[], int index)
{
  if(index > argc)
  {
    return "";
  }

  return std::string(argv[index]);
}

/**
 * @brief Reads the value after an operand and advances its token index.
 * @param argc Number of command-line tokens.
 * @param argv Command-line token array.
 * @param index Operand index, updated to the value index when a value exists.
 * @return The operand value, or an empty string if the next token starts another operand.
 */
std::string ParseArgument(int argc, char* argv[], int& index)
{
  std::string argStr;
  if(++index < argc)
  {
    argStr = argv[index];

    // A leading hyphen starts the next operand, so the current operand has no value.
    if(argStr[0] == '-')
    {
      index--;
      argStr = "";
    }
  }
  return argStr;
}

/**
 * @brief Parses nxrunner operands and their optional values.
 * @param argc Number of command-line tokens.
 * @param argv Command-line token array.
 * @return Parsed arguments, or an error when no operand exists.
 */
Result<CliArgumentsType> ParseParameters(int argc, char* argv[])
{
  if(argc < 2)
  {
    std::string ss = "No arguments to parse";
    return nx::core::MakeErrorResult<CliArgumentsType>(k_NoArgumentsProvided, ss);
  }

  CliArgumentsType args;

  int index = 1;
  std::string arg = argv[index];
  if(arg == k_HelpParamLong || arg == k_HelpParamShort)
  {
    args.emplace_back(ArgumentType::Help);

    if(argc < 3)
    {
      return {args};
    }

    index++;
  }

  do
  {
    arg = argv[index];

    if(arg == k_ExecuteParamLong || arg == k_ExecuteParamShort)
    {
      std::string argStr = ParseArgument(argc, argv, index);
      args.emplace_back(ArgumentType::Execute, argStr);
    }
    else if(arg == k_PreflightParamLong || arg == k_PreflightParamShort)
    {
      std::string argStr = ParseArgument(argc, argv, index);
      args.emplace_back(ArgumentType::Preflight, argStr);
    }
    else if(arg == k_LogFileParamLong || arg == k_LogFileParamShort)
    {
      std::string argStr = ParseArgument(argc, argv, index);
      args.emplace_back(ArgumentType::Logfile, argStr);
    }
    else if(arg == k_ConvertParamLong || arg == k_ConvertParamShort)
    {
      std::string argStr = ParseArgument(argc, argv, index);
      args.emplace_back(ArgumentType::Convert, argStr);
    }
    else if(arg == k_CacheMemoryBudgetParamLong)
    {
      std::string argStr = ParseArgument(argc, argv, index);
      args.emplace_back(ArgumentType::CacheMemoryBudget, argStr);
    }
    else
    {
      args.emplace_back(ArgumentType::Invalid, arg);
    }

    index++;
  } while(index < argc);

  return {args};
}

/**
 * @brief Writes Result diagnostics and converts the first error code to a process code.
 * @tparam T Specifies the Result value type.
 * @param result Result to print.
 * @return 0 for a valid Result, or the first error code for an invalid Result.
 * @pre An invalid Result must contain at least one error.
 */
template <typename T = void>
int PrintResult(const Result<T>& result)
{
  if(result.valid())
  {
    return 0;
  }

  for(const auto& warning : result.warnings())
  {
    CliOut << fmt::format("Warning {}: {}", warning.code, warning.message);
    CliOut.endline();
  }
  for(const auto& error : result.errors())
  {
    CliOut << fmt::format("Error {}: {}", error.code, error.message);
    CliOut.endline();
  }
  return result.errors()[0].code;
}

/**
 * @brief Preflights a loaded pipeline while a CLI observer reports progress.
 * @param pipeline Pipeline to preflight.
 * @return An error if Pipeline::preflight() returns false.
 */
Result<> PreflightPipeline(Pipeline& pipeline)
{
  const cli::PipelineObserver obs(&pipeline);
  CliOut << "\n-------------------------";
  CliOut.endline();

  if(!pipeline.preflight())
  {
    std::string ss = "Error preflighting pipeline";
    return nx::core::MakeErrorResult(-2, ss);
  }

  CliOut << "Finished preflighting pipeline";
  CliOut.endline();
  return {};
}

/**
 * @brief Executes a loaded pipeline while a CLI observer reports progress.
 * @param pipeline Pipeline to execute.
 * @return An error if Pipeline::execute() returns false.
 */
Result<> ExecutePipeline(Pipeline& pipeline)
{
  const cli::PipelineObserver obs(&pipeline);
  CliOut << "\n-------------------------";
  CliOut.endline();

  if(!pipeline.execute())
  {
    std::string ss = "Error executing pipeline";
    return nx::core::MakeErrorResult(k_ExecutePipelineError, ss);
  }
  CliOut << Timestamp() << " Finished executing pipeline";
  CliOut.endline();
  return {};
}

/**
 * @brief Loads and executes the pipeline named by an argument.
 * @param arg Execute operand with a pipeline path value.
 * @return Load diagnostics or the pipeline execution result.
 *
 * A `.json` path is a legacy pipeline. The helper returns its load result and
 * instructs the user to convert the file before execution.
 */
Result<> ExecutePipeline(const Argument& arg)
{
  std::string pipelinePath = arg.value;
  CliOut << "Executing Pipeline: " << pipelinePath << "\n";

  auto loadPipelineResult = Pipeline::FromFile(pipelinePath);

  if(pipelinePath.ends_with(".json"))
  {
    CliOut << "Input file '" << pipelinePath << "' is a legacy DREAM.3D version 6.x formatted pipeline.\n";
    CliOut << "  You will need to run `nxrunner --convert [PATH TO .JSON FILE]` to first convert the\n";
    CliOut << "  pipeline file to the newer format. Please note that the conversion can fail as filters have\n";
    CliOut << "  been updated and previous parameters may not be available in DREAM3D-NX.\n";
    return nx::core::ConvertResult(std::move(loadPipelineResult));
  }

  if(loadPipelineResult.invalid())
  {
    CliOut << fmt::format("Error: Could not load pipeline at path: '{}'", pipelinePath);
    CliOut.endline();
    return nx::core::ConvertResult(std::move(loadPipelineResult));
  }
  if(!loadPipelineResult.m_warnings.empty())
  {
    CliOut << "Input Pipeline Warnings"
           << "\n";
    for(const auto& warning : loadPipelineResult.m_warnings)
    {
      CliOut << fmt::format(" [{}] {}", warning.code, warning.message) << "\n";
    }
  }

  Pipeline pipeline = loadPipelineResult.value();
  CliOut << fmt::format("Executing pipeline at path: '{}'\n", pipelinePath);
  CliOut.endline();
  return ExecutePipeline(pipeline);
}

/**
 * @brief Loads and preflights the pipeline named by an argument.
 * @param arg Preflight operand with a pipeline path value.
 * @return Load diagnostics or the pipeline preflight result.
 *
 * A `.json` path is a legacy pipeline. The helper returns its load result and
 * instructs the user to convert the file before preflight.
 */
Result<> PreflightPipeline(const Argument& arg)
{
  std::string pipelinePath = arg.value;
  CliOut << "Preflight Pipeline: " << pipelinePath << "\n";
  auto loadPipelineResult = Pipeline::FromFile(pipelinePath);

  if(pipelinePath.ends_with(".json"))
  {
    CliOut << "Input file '" << pipelinePath << "' is a legacy DREAM.3D version 6.x formatted pipeline.\n";
    CliOut << "  You will need to run `nxrunner --convert [PATH TO .JSON FILE]` to first convert the\n";
    CliOut << "  pipeline file to the newer format. Please note that the conversion can fail as filters have\n";
    CliOut << "  been updated and previous parameters may not be available in DREAM3D-NX.\n";
    return nx::core::ConvertResult(std::move(loadPipelineResult));
  }

  if(loadPipelineResult.invalid())
  {
    CliOut << fmt::format("Error: Could not load pipeline at path: '{}'", pipelinePath);
    CliOut.endline();
    return nx::core::ConvertResult(std::move(loadPipelineResult));
  }
  if(!loadPipelineResult.m_warnings.empty())
  {
    CliOut << "Preflight Pipeline: Input Pipeline Warnings"
           << "\n";
    for(const auto& warning : loadPipelineResult.m_warnings)
    {
      CliOut << fmt::format(" [{}] {}", warning.code, warning.message) << "\n";
    }
  }

  CliOut << fmt::format("Preflighting pipeline at path: '{}'\n", pipelinePath);
  CliOut.endline();

  Pipeline pipeline = loadPipelineResult.value();
  return PreflightPipeline(pipeline);
}

/**
 * @brief Converts or validates the pipeline named by an argument.
 * @param arg Convert operand with a pipeline path value.
 * @param printConvertedPipeline True to write the converted JSON to CLI output.
 * @param saveConverted True to save converted legacy input as a .d3dpipeline file.
 * @return Conversion or validation diagnostics.
 *
 * A `.json` input uses the legacy converter. A `.d3dpipeline` input receives a
 * validation load and is never rewritten.
 */
Result<> ConvertPipeline(const Argument& arg, bool printConvertedPipeline, bool saveConverted)
{
  std::string pipelinePath = arg.value;
  CliOut << fmt::format("Input File: '{}'", pipelinePath);
  CliOut.endline();

  nx::core::Result<nx::core::Pipeline> loadPipelineResult;
  if(pipelinePath.ends_with(".json"))
  {
    loadPipelineResult = Pipeline::FromSIMPLFile(pipelinePath);
  }
  else if(pipelinePath.ends_with(".d3dpipeline"))
  {
    CliOut << "Input file is already a DREAM3D-NX formatted pipeline file. A sanity check will be run instead. Any warnings will be printed\n";
    loadPipelineResult = nx::core::Pipeline::FromFile(pipelinePath, true);
    saveConverted = false;

    for(const auto& warning : loadPipelineResult.warnings())
    {
      CliOut << fmt::format("Warning ({}): {}\n", warning.code, warning.message);
    }
  }
  else
  {
    CliOut << "Error: Input file extension is not recognized. Aborting execution now.\n";
    return ConvertResult(std::move(loadPipelineResult));
  }

  if(loadPipelineResult.invalid())
  {
    CliOut << fmt::format("Error: Could not convert pipeline at path: '{}'", pipelinePath);
    CliOut.endline();
    return ConvertResult(std::move(loadPipelineResult));
  }

  Pipeline pipeline = std::move(loadPipelineResult.value());
  if(saveConverted)
  {
    std::filesystem::path path(pipelinePath);
    auto extension = path.extension().string();
    pipelinePath.erase(pipelinePath.size() - extension.size());
    pipelinePath += Pipeline::k_Extension;

    std::fstream fout(pipelinePath, std::ios_base::out | std::ios_base::trunc);
    fout << pipeline.toJson().dump(4);
    fout.flush();

    CliOut << fmt::format("Converted File: '{}'", pipelinePath);
    CliOut.endline();
  }
  if(printConvertedPipeline)
  {
    CliOut << pipeline.toJson().dump(4);
    CliOut.endline();
  }
  return ConvertResult(std::move(loadPipelineResult));
}

/**
 * @brief Writes the nxrunner command summary.
 */
void DisplayDefaultHelp()
{
  CliOut << "Options:\n";
  CliOut << fmt::format("  {}|{} <pipeline filepath> [{}|{} <log filepath>]\t", k_ExecuteParamLong, k_ExecuteParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "  Execute the pipeline at the target filepath. Optionally, create a log file at the specified path.\n";
  CliOut << fmt::format("  {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_PreflightParamLong, k_PreflightParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "  Preflight the pipeline at the target filepath. Optionally, create a log file at the specified path.\n";
  CliOut << fmt::format("  {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_ConvertParamLong, k_ConvertParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "  Convert the SIMPL pipeline at the target filepath. Optionally, create a log file at the specified path.";
  CliOut << fmt::format("  <operand [argument]>  [{}|{} <log filepath>]\t", k_LogFileParamLong, k_LogFileParamShort) << "  Creates a log file at the specified path.";
  CliOut << fmt::format("  {} <gigabytes>\t", k_CacheMemoryBudgetParamLong)
         << "  Override the cache memory budget for this run (decimal GB, e.g. 8 or 1.5). This does not cap total process memory or modify saved preferences.\n";
  CliOut.endline();
}

/**
 * @brief Writes help for pipeline execution.
 */
void DisplayExecuteHelp()
{
  CliOut << "To execute a target pipeline file:\n\t";
  CliOut << fmt::format("  {}|{} <pipeline filepath> [{}|{} <log filepath>]\t", k_ExecuteParamLong, k_ExecuteParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "  Execute the pipeline at the target filepath. Optionally, create a log file at the specified path.";
  CliOut.endline();
}

/**
 * @brief Writes help for pipeline preflight.
 */
void DisplayPreflightHelp()
{
  CliOut << "To preflight a target pipeline file:\n\t";
  CliOut << fmt::format("  {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_PreflightParamLong, k_PreflightParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "  Preflight the pipeline at the target filepath. Optionally, create a log file at the specified path.";
  CliOut.endline();
}

/**
 * @brief Writes help for legacy pipeline conversion.
 */
void DisplayConvertHelp()
{
  CliOut << "To convert a target SIMPL pipeline file:\n\t";
  CliOut << fmt::format("  {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_ConvertParamLong, k_ConvertParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "  Convert the SIMPL pipeline at the target filepath. Optionally, create a log file at the specified path.";
  CliOut.endline();
}

/**
 * @brief Writes help for log-file output.
 */
void DisplayLogfileHelp()
{
  CliOut << "To export output a log file:\n\t";
  CliOut << fmt::format("  <operand [argument]>  [{}|{} <log filepath>]\t", k_LogFileParamLong, k_LogFileParamShort) << "  Creates a log file at the specified path.";
  CliOut.endline();
}

/**
 * @brief Writes help for the process cache-budget override.
 */
void DisplayCacheMemoryBudgetHelp()
{
  CliOut << "To override the cache memory budget for this run:\n\t";
  CliOut << fmt::format("  {} <gigabytes>\t", k_CacheMemoryBudgetParamLong)
         << "  Override the cache memory budget for this run only (decimal gigabytes, e.g. 8 or 1.5). This does not cap total process memory or modify saved preferences.";
  CliOut.endline();
}

/**
 * @brief Writes general or operand-specific help.
 * @param arguments Parsed arguments whose second operand selects detailed help.
 * @return An error for unsupported help syntax.
 */
Result<> DisplayHelpMenu(const std::vector<Argument>& arguments)
{
  if(arguments.size() == 1)
  {
    DisplayDefaultHelp();
    return {};
  }
  switch(arguments[1].type)
  {
  case ArgumentType::Execute: {
    DisplayExecuteHelp();
    return {};
  }
  case ArgumentType::Preflight: {
    DisplayPreflightHelp();
    return {};
  }
  case ArgumentType::Convert: {
    DisplayConvertHelp();
    return {};
  }
  case ArgumentType::Logfile: {
    DisplayLogfileHelp();
    return {};
  }
  case ArgumentType::CacheMemoryBudget: {
    DisplayCacheMemoryBudgetHelp();
    return {};
  }
  case ArgumentType::Invalid: {
    [[fallthrough]];
  }
  case ArgumentType::Help: {
    break;
  }
  }

  std::string ss = "Incorrect Help Syntax";
  return nx::core::MakeErrorResult(k_FailedParsingArguments, ss);
}

/**
 * @brief Creates an error for an unsupported command-line argument.
 * @param argument Invalid argument.
 * @return An invalid-argument Result.
 */
Result<> CreateArgumentError(const Argument& argument)
{
  std::string errorMessage = fmt::format("Failed to parse argument: {}", argument.value);
  return nx::core::MakeErrorResult(k_InvalidArgumentError, errorMessage);
}

/**
 * @brief Opens the log file named by an argument.
 * @param argument Logfile operand with a path value.
 * @return The log-stream open result.
 */
Result<> SetLogFile(const Argument& argument)
{
  std::filesystem::path filepath(argument.value);
  return CliOut.setLogFile(filepath);
}
} // namespace

/**
 * @brief Runs the nxrunner command-line interface.
 * @param argc Number of command-line tokens.
 * @param argv Command-line token array.
 * @return 0 on success, or a diagnostic error code on failure.
 */
int main(int argc, char* argv[])
{
  CliOut << fmt::format("nxrunner: Version {} Build Date:{}\n\n", nx::core::Version::Package(), nx::core::Version::BuildDate());
  // cliOut.endline();
  // cliOut << "ARGUMENT LISTING START\n";
  // for(int argIndex = 0; argIndex < argc; argIndex++)
  // {
  //   cliOut << "Argument[" << argIndex << "]: " << argv[argIndex] << "\n";
  // }
  // cliOut << "ARGUMENT LISTING END\n";

  Result<CliArgumentsType> parsingResult = ParseParameters(argc, argv);
  if(parsingResult.invalid())
  {
    return PrintResult<CliArgumentsType>(parsingResult);
  }

  CliArgumentsType arguments = parsingResult.value();
  std::vector<Result<>> results;

  std::optional<uint64> overrideCacheMemoryBudgetBytes;

  // Apply options that must take effect before plugin loading or target execution.
  for(const Argument& argument : arguments)
  {
    switch(argument.type)
    {
    case ArgumentType::Invalid: {
      results.push_back(CreateArgumentError(argument));
      break;
    }
    case ArgumentType::Logfile: {
      results.push_back(SetLogFile(argument));
      break;
    }
    case ArgumentType::CacheMemoryBudget: {
      if(argument.value.empty())
      {
        results.push_back(nx::core::MakeErrorResult(k_InvalidArgumentError, "--cache-memory-budget requires a value in gigabytes (e.g. 8 or 1.5)"));
        break;
      }
      try
      {
        usize parsedChars = 0;
        double gb = std::stod(argument.value, &parsedChars);
        if(parsedChars != argument.value.size() || !std::isfinite(gb) || gb <= 0.0)
        {
          results.push_back(
              nx::core::MakeErrorResult(k_InvalidArgumentError, fmt::format("Invalid value for --cache-memory-budget: '{}' (must be a finite, positive number of gigabytes)", argument.value)));
          break;
        }
        overrideCacheMemoryBudgetBytes = static_cast<uint64>(gb * 1024.0 * 1024.0 * 1024.0);
      } catch(const std::exception&)
      {
        results.push_back(nx::core::MakeErrorResult(k_InvalidArgumentError, fmt::format("Invalid value for --cache-memory-budget: '{}' (expected a positive number of gigabytes)", argument.value)));
      }
      break;
    }
    case ArgumentType::Convert: {
      [[fallthrough]];
    }
    case ArgumentType::Execute: {
      [[fallthrough]];
    }
    case ArgumentType::Preflight: {
      break;
    }
    case ArgumentType::Help:
      PrintResult(DisplayHelpMenu(arguments));
      return 0;
    }
  }

  // Create the Application before the temporary preference override.
  auto app = nx::core::Application::GetOrCreateInstance();
  for(const auto& warning : app->takePreferenceLoadWarnings())
  {
    fmt::print(stderr, "Warning {}: {}\n", warning.code, warning.message);
  }

  /**
   * @struct PreferencesCacheBudgetRestorer
   * @brief Restores the process preference after a command-line cache-budget override.
   *
   * The restorer does not save preferences to disk. It removes the key if the
   * key did not exist before this nxrunner invocation.
   */
  struct PreferencesCacheBudgetRestorer
  {
    Preferences* prefs = nullptr;
    bool was_present = false;
    uint64 original_value = 0;
    /**
     * @brief Restores or removes the cache-budget preference key.
     */
    ~PreferencesCacheBudgetRestorer()
    {
      if(prefs == nullptr)
      {
        return;
      }
      if(was_present)
      {
        prefs->setCacheMemoryBudgetBytes(original_value);
      }
      else
      {
        prefs->removeValue(Preferences::k_CacheMemoryBudgetBytes_Key);
      }
    }
  };
  PreferencesCacheBudgetRestorer cacheBudgetRestorer;

  if(overrideCacheMemoryBudgetBytes.has_value())
  {
    Preferences* preferencesPtr = app->getPreferences();
    if(preferencesPtr != nullptr)
    {
      cacheBudgetRestorer.prefs = preferencesPtr;
      cacheBudgetRestorer.was_present = preferencesPtr->contains(std::string(Preferences::k_CacheMemoryBudgetBytes_Key));
      if(cacheBudgetRestorer.was_present)
      {
        cacheBudgetRestorer.original_value = preferencesPtr->cacheMemoryBudgetBytes();
      }
      preferencesPtr->setCacheMemoryBudgetBytes(*overrideCacheMemoryBudgetBytes);
      // The preference does not update the running manager. Apply the value there too.
      // setBudgetBytes() clamps the request to the machine-safe maximum.
      // This call keeps headless OOC caches from using the manager's default budget.
      // The CLI stays silent when clamping because only the GUI reports that event.
      nx::core::CacheMemoryBudgetManager::Instance().setBudgetBytes(*overrideCacheMemoryBudgetBytes);
    }
  }

  LoadApp();

#if SIMPLNX_EMBED_PYTHON
  nx::python::OutputCallback outputCallback = [](const std::string& message) { std::cout << message << "\n"; };

  nx::python::SetupPythonEnvironmentVars(outputCallback);

  std::set<std::string> pythonPlugins = nx::python::GetPythonPluginListFromEnvironment();

  py::scoped_interpreter guard{};

  nx::python::PluginLoadErrorCallback pluginLoadErrorCallback = [](const nx::python::PluginLoadErrorInfo& errorInfo) {
    std::string exceptionType = nx::python::ExceptionTypeToString(errorInfo.type);
    std::string text = fmt::format("{} exception while while attempting to import '{}': ", exceptionType, errorInfo.pluginName);
    std::cout << text << "\n";
    std::cout << errorInfo.message << "\n";
  };
  nx::python::PythonErrorCallback pythonErrorCallback = [](const nx::python::PythonErrorInfo& errorInfo) {
    std::string exceptionType = nx::python::ExceptionTypeToString(errorInfo.type);
    std::string text = fmt::format("{} exception while importing plugins: ", exceptionType);
    std::cout << text;
    std::cout << errorInfo.message;
  };

  try
  {
    auto manualImportFinder = nx::python::ManualImportFinderHolder::Create();
    manualImportFinder.addToMetaPath();
    nx::python::LoadPythonPlugins(pythonPlugins, outputCallback, pluginLoadErrorCallback, pythonErrorCallback);
    nx::python::LoadInstalledPythonPlugins(outputCallback, pluginLoadErrorCallback, pythonErrorCallback);
  } catch(const std::exception& exception)
  {
    std::cout << "Aborting python plugin loading due to exception: \n";
    std::cout << exception.what();
  }
#endif

  int errorCode = 0;
  // Run only the first target operation. Earlier parsing handled process-wide options.
  switch(arguments[0].type)
  {
  case ArgumentType::Help: {
    PrintResult(DisplayHelpMenu(arguments));
    return errorCode;
  }
  case ArgumentType::Execute: {
    try
    {
      CliOut << "###### EXECUTE MODE ########\n";
      auto result = ExecutePipeline(arguments[0]);
      results.push_back(result);
    }
#if SIMPLNX_EMBED_PYTHON
    catch(const py::error_already_set& exception)
    {
      fmt::print("Python exception: {}\n", exception.what());
      return 1;
    }
#endif
    catch(const std::exception& exception)
    {
      fmt::print("Exception: {}\n", exception.what());
      return 1;
    }
    break;
  }
  case ArgumentType::Preflight: {
    try
    {
      CliOut << "###### PREFLIGHT MODE ########\n";
      auto result = PreflightPipeline(arguments[0]);
      results.push_back(result);
    }
#if SIMPLNX_EMBED_PYTHON
    catch(const py::error_already_set& exception)
    {
      fmt::print("Python exception: {}\n", exception.what());
      return 1;
    }
#endif
    catch(const std::exception& exception)
    {
      fmt::print("Exception: {}\n", exception.what());
      return 1;
    }
    break;
  }
  case ArgumentType::Convert: {
    auto result = ConvertPipeline(arguments[0], false, true);
    results.push_back(result);
    break;
  }
  default: {
    break;
  }
  }

  // Print all collected diagnostics. The last nonzero result becomes the process code.
  for(const auto& result : results)
  {
    if(int code = PrintResult(result); code != 0)
    {
      errorCode = code;
    }
  }
  return errorCode;
}
