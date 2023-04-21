#include "CliObserver.hpp"

#include "complex/Common/Result.hpp"
#include "complex/Core/Application.hpp"
#include "complex/Pipeline/Pipeline.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>

namespace fs = std::filesystem;
using namespace complex;

inline constexpr int k_FailedLoadingPipeline = -100;
inline constexpr int k_FailedExecutingPipeline = -101;
inline constexpr int k_FailedPreflightingPipeline = -102;
inline constexpr int k_NoArgumentsProvided = -103;
inline constexpr int k_FailedParsingArguments = -105;
inline constexpr int k_ExecutePipelineError = -110;
inline constexpr int k_PreflightPipelineError = -110;
inline constexpr int k_InvalidArgumentError = -120;
inline constexpr int k_LogFileError = -121;
inline constexpr int k_NullLogFileError = -122;

inline constexpr StringLiteral k_HelpParamLong = "--help";
inline constexpr StringLiteral k_ExecuteParamLong = "--execute";
inline constexpr StringLiteral k_PreflightParamLong = "--preflight";
inline constexpr StringLiteral k_LogFileParamLong = "--logfile";

inline constexpr StringLiteral k_HelpParamShort = "-h";
inline constexpr StringLiteral k_ExecuteParamShort = "-e";
inline constexpr StringLiteral k_PreflightParamShort = "-p";
inline constexpr StringLiteral k_LogFileParamShort = "-l";

void LoadApp(complex::Application& app)
{
  // Try loading plugins from the directory that the executable is in.
  // This is the default for developer build trees and CI build trees
  fs::path appPath = app.getCurrentDir();
  app.loadPlugins(appPath, true);

  // For non-windows platforms we need to look in the actual 'Plugins'
  // directory which is up one directory from the executable.
#ifndef _MSC_VER
  {
    appPath = appPath.parent_path();
    // Check if there is a Plugins Folder inside the app package
    if(fs::exists(appPath / "Plugins"))
    {
      appPath = appPath / "Plugins";
      app.loadPlugins(appPath, true);
    }
  }
#endif
}

class CliStream
{
public:
  CliStream() = default;
  ~CliStream() noexcept = default;

  Result<> setLogFile(const std::filesystem::path& filepath)
  {
    if(filepath.empty() || filepath.string().length() == 0)
    {
      std::string errorMessage = "Log file cannot be created with an empty filepath.";
      return complex::MakeErrorResult(k_NullLogFileError, errorMessage);
    }

    const std::ios_base::openmode openmode = std::ios_base::out | std::ios_base::trunc;
    m_LogStream = std::fstream(filepath, openmode);
    if(m_LogStream.is_open() == false)
    {
      std::string errorMessage = fmt::format("Failed to open log file: '{}'", filepath.string());
      return complex::MakeErrorResult(k_LogFileError, errorMessage);
    }

    return {};
  }

  template <typename T>
  CliStream& operator<<(T value)
  {
    std::cout << value;
    if(m_LogStream.is_open())
    {
      m_LogStream << value;
    }
    return *this;
  }
  CliStream& endline()
  {
    std::cout << std::endl;
    if(m_LogStream.is_open())
    {
      m_LogStream << std::endl;
    }
    return *this;
  }

private:
  std::fstream m_LogStream;
};

CliStream cliOut;

enum class ArgumentType
{
  Invalid,
  Execute,
  Preflight,
  Help,
  Logfile
};

struct Argument
{
  ArgumentType type;
  std::string value;

  Argument(ArgumentType targetType, std::string targetValue = "")
  : type(targetType)
  , value(targetValue)
  {
  }
};

using CliArguments = std::vector<Argument>;

inline std::string toString(int argc, char* argv[], int index)
{
  if(index > argc)
  {
    return "";
  }

  return std::string(argv[index]);
}

std::string ParseArgument(int argc, char* argv[], int& index)
{
  std::string argStr;
  if(++index < argc)
  {
    argStr = argv[index];

    // Check if starting another operand
    if(argStr[0] == '-')
    {
      index--;
      argStr = "";
    }
  }
  return argStr;
}

Result<CliArguments> ParseParameters(int argc, char* argv[])
{
  if(argc < 2)
  {
    std::string ss = "No arguments to parse";
    return complex::MakeErrorResult<CliArguments>(k_NoArgumentsProvided, ss);
  }

  CliArguments args;

  int index = 1;
  std::string arg = argv[index];
  if(arg == k_HelpParamLong || arg == k_HelpParamShort)
  {
    args.emplace_back(ArgumentType::Help);

    if(argc < 3)
    {
      return {args};
    }
    else
    {
      index++;
    }
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
    else
    {
      args.emplace_back(ArgumentType::Invalid, arg);
    }

    index++;
  } while(index < argc);

  return {args};
}

template <typename T = void>
int PrintResult(const Result<T>& result)
{
  if(result.valid())
  {
    return 0;
  }

  for(const auto& warning : result.warnings())
  {
    cliOut << fmt::format("Warning {}: {}", warning.code, warning.message);
    cliOut.endline();
  }
  for(const auto& error : result.errors())
  {
    cliOut << fmt::format("Error {}: {}", error.code, error.message);
    cliOut.endline();
  }
  return result.errors()[0].code;
}

Result<> PreflightPipeline(Pipeline& pipeline)
{
  const CLI::PipelineObserver obs(&pipeline);
  cliOut << "\n-------------------------";
  cliOut.endline();

  if(!pipeline.preflight())
  {
    std::string ss = "Error preflighting pipeline";
    return complex::MakeErrorResult(-2, ss);
  }

  cliOut << "Finished preflighting pipeline";
  cliOut.endline();
  return {};
}

Result<> ExecutePipeline(Pipeline& pipeline)
{
  const CLI::PipelineObserver obs(&pipeline);
  cliOut << "\n-------------------------";
  cliOut.endline();

  if(!pipeline.execute())
  {
    std::string ss = "Error executing pipeline";
    return complex::MakeErrorResult(k_ExecutePipelineError, ss);
  }
  cliOut << "Finished executing pipeline";
  cliOut.endline();
  return {};
}

Result<> ExecutePipeline(const Argument& arg)
{
  std::string pipelinePath = arg.value;
  auto loadPipelineResult = Pipeline::FromFile(pipelinePath);
  if(loadPipelineResult.invalid())
  {
    cliOut << fmt::format("Could not load pipeline at path: '{}'", pipelinePath);
    cliOut.endline();
    return complex::ConvertResult(std::move(loadPipelineResult));
  }

  Pipeline pipeline = loadPipelineResult.value();
  cliOut << fmt::format("Executing pipeline at path: '{}'\n", pipelinePath);
  cliOut.endline();
  return ExecutePipeline(pipeline);
}

Result<> PreflightPipeline(const Argument& arg)
{
  std::string pipelinePath = arg.value;
  auto loadPipelineResult = Pipeline::FromFile(pipelinePath);
  if(loadPipelineResult.invalid())
  {
    cliOut << fmt::format("Could not load pipeline at path: '{}'", pipelinePath);
    cliOut.endline();
    return complex::ConvertResult(std::move(loadPipelineResult));
  }

  cliOut << fmt::format("Preflighting pipeline at path: '{}'\n", pipelinePath);
  cliOut.endline();

  Pipeline pipeline = loadPipelineResult.value();
  return PreflightPipeline(pipeline);
}

void DisplayDefaultHelp()
{
  cliOut << "Options:\n";
  cliOut << fmt::format("\t {}|{} <pipeline filepath> [{}|{} <log filepath>]\t", k_ExecuteParamLong, k_ExecuteParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Execute the pipeline at the target filepath. Optionally, create a log file at the specified path.\n";
  cliOut << fmt::format("\t {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_PreflightParamLong, k_PreflightParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Preflight the pipeline at the target filepath. Optionally, create a log file at the specified path.\n";
  cliOut << fmt::format("\t <operand [argument]>  [{}|{} <log filepath>]\t", k_LogFileParamLong, k_LogFileParamShort) << "\t Creates a log file at the specified path.";
  cliOut.endline();
}

void DisplayExecuteHelp()
{
  cliOut << "To execute a target pipeline file:\n\t";
  cliOut << fmt::format("\t {}|{} <pipeline filepath> [{}|{} <log filepath>]\t", k_ExecuteParamLong, k_ExecuteParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Execute the pipeline at the target filepath. Optionally, create a log file at the specified path.";
  cliOut.endline();
}

void DisplayPreflightHelp()
{
  cliOut << "To preflight a target pipeline file:\n\t";
  cliOut << fmt::format("\t {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_PreflightParamLong, k_PreflightParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Preflight the pipeline at the target filepath. Optionally, create a log file at the specified path.";
  cliOut.endline();
}

void DisplayLogfileHelp()
{
  cliOut << "To export output a log file:\n\t";
  cliOut << fmt::format("\t <operand [argument]>  [{}|{} <log filepath>]\t", k_LogFileParamLong, k_LogFileParamShort) << "\t Creates a log file at the specified path.";
  cliOut.endline();
}

Result<> DisplayHelpMenu(const std::vector<Argument>& arguments)
{
  if(arguments.size() == 1)
  {
    DisplayDefaultHelp();
    return {};
  }
  switch(arguments[1].type)
  {
  case ArgumentType::Execute:
    DisplayExecuteHelp();
    return {};
  case ArgumentType::Preflight:
    DisplayPreflightHelp();
    return {};
  case ArgumentType::Logfile:
    DisplayLogfileHelp();
    return {};
  case ArgumentType::Invalid:
  case ArgumentType::Help:
    break;
  }

  std::string ss = "Incorrect Help Syntax";
  return complex::MakeErrorResult(k_FailedParsingArguments, ss);
}

Result<> CreateArgumentError(const Argument& argument)
{
  std::string errorMessage = fmt::format("Failed to parse argument: {}", argument.value);
  return complex::MakeErrorResult(k_InvalidArgumentError, errorMessage);
}

Result<> SetLogFile(const Argument& argument)
{
  std::filesystem::path filepath(argument.value);
  return cliOut.setLogFile(filepath);
}

int main(int argc, char* argv[])
{

  // cliOut << "nxrunner";
  // cliOut.endline();
  // cliOut << "ARGUMENT LISTING START\n";
  // for(int argIndex = 0; argIndex < argc; argIndex++)
  // {
  //   cliOut << "Argument[" << argIndex << "]: " << argv[argIndex] << "\n";
  // }
  // cliOut << "ARGUMENT LISTING END\n";

  Result<CliArguments> parsingResult = ParseParameters(argc, argv);
  if(parsingResult.invalid())
  {
    return PrintResult<CliArguments>(parsingResult);
  }

  CliArguments arguments = parsingResult.value();
  std::vector<Result<>> results;

  // Set log file and check for parsing errors
  for(const Argument& argument : arguments)
  {
    switch(argument.type)
    {
    case ArgumentType::Invalid:
      results.push_back(CreateArgumentError(argument));
      break;
    case ArgumentType::Logfile:
      results.push_back(SetLogFile(argument));
      break;
    case ArgumentType::Execute:
    case ArgumentType::Preflight:
    case ArgumentType::Help:
      break;
    }
  }

  // Load the Complex Application instance and load the plugins
  complex::Application app;
  LoadApp(app);

  int errorCode = 0;
  // Run target operation
  switch(arguments[0].type)
  {
  case ArgumentType::Help:
    PrintResult(DisplayHelpMenu(arguments));
    return errorCode;
    break;
  case ArgumentType::Execute:
    results.push_back(ExecutePipeline(arguments[0]));
    break;
  case ArgumentType::Preflight:
    results.push_back(PreflightPipeline(arguments[0]));
    break;
  default:
    break;
  }

  // Print Results and set error code

  for(const auto& result : results)
  {
    if(int code = PrintResult(result); code != 0)
    {
      errorCode = code;
    }
  }
  return errorCode;
}
