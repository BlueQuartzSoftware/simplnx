#include "CliObserver.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/SimplnxPython.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>

#if SIMPLNX_EMBED_PYTHON
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

constexpr StringLiteral k_HelpParamLong = "--help";
constexpr StringLiteral k_ExecuteParamLong = "--execute";
constexpr StringLiteral k_PreflightParamLong = "--preflight";
constexpr StringLiteral k_LogFileParamLong = "--logfile";
constexpr StringLiteral k_ConvertParamLong = "--convert";
constexpr StringLiteral k_ConvertOutputParamLong = "--convert-output";

constexpr StringLiteral k_HelpParamShort = "-h";
constexpr StringLiteral k_ExecuteParamShort = "-e";
constexpr StringLiteral k_PreflightParamShort = "-p";
constexpr StringLiteral k_LogFileParamShort = "-l";
constexpr StringLiteral k_ConvertParamShort = "-c";
constexpr StringLiteral k_ConvertOutputParamShort = "-co";

void LoadApp()
{
  auto app = Application::GetOrCreateInstance();
  // Try loading plugins from the directory that the executable is in.
  // This is the default for developer build trees and CI build trees
  fs::path appPath = app->getCurrentDir();
  app->loadPlugins(appPath, true);

  // For non-windows platforms we need to look in the actual 'Plugins'
  // directory which is up one directory from the executable.
#ifndef _MSC_VER
  {
    appPath = appPath.parent_path();
    // Check if there is a Plugins Folder inside the app package
    if(fs::exists(appPath / "Plugins"))
    {
      appPath = appPath / "Plugins";
      app->loadPlugins(appPath, true);
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
      return nx::core::MakeErrorResult(k_NullLogFileError, errorMessage);
    }

    const std::ios_base::openmode openmode = std::ios_base::out | std::ios_base::trunc;
    m_LogStream = std::fstream(filepath, openmode);
    if(m_LogStream.is_open() == false)
    {
      std::string errorMessage = fmt::format("Failed to open log file: '{}'", filepath.string());
      return nx::core::MakeErrorResult(k_LogFileError, errorMessage);
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
  Logfile,
  Convert,
  ConvertOutput
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

std::string toString(int argc, char* argv[], int index)
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
    return nx::core::MakeErrorResult<CliArguments>(k_NoArgumentsProvided, ss);
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
    else if(arg == k_ConvertParamLong || arg == k_ConvertParamShort)
    {
      std::string argStr = ParseArgument(argc, argv, index);
      args.emplace_back(ArgumentType::Convert, argStr);
    }
    else if(arg == k_ConvertOutputParamLong || arg == k_ConvertOutputParamShort)
    {
      std::string argStr = ParseArgument(argc, argv, index);
      args.emplace_back(ArgumentType::ConvertOutput, argStr);
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
    return nx::core::MakeErrorResult(-2, ss);
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
    return nx::core::MakeErrorResult(k_ExecutePipelineError, ss);
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
    return nx::core::ConvertResult(std::move(loadPipelineResult));
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
    return nx::core::ConvertResult(std::move(loadPipelineResult));
  }

  cliOut << fmt::format("Preflighting pipeline at path: '{}'\n", pipelinePath);
  cliOut.endline();

  Pipeline pipeline = loadPipelineResult.value();
  return PreflightPipeline(pipeline);
}

Result<> ConvertPipeline(const Argument& arg, bool saveConverted = false)
{
  std::string pipelinePath = arg.value;
  auto loadPipelineResult = Pipeline::FromSIMPLFile(pipelinePath);
  if(loadPipelineResult.invalid())
  {
    cliOut << fmt::format("Could not convert pipeline at path: '{}'", pipelinePath);
    cliOut.endline();
    return ConvertResult(std::move(loadPipelineResult));
  }

  cliOut << fmt::format("Converted SIMPL pipeline at path: '{}'\n", pipelinePath);
  cliOut.endline();

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

    cliOut << fmt::format("Saved converted pipeline at path: '{}'\n", pipelinePath);
    cliOut.endline();
  }

  cliOut << pipeline.toJson().dump(4);
  cliOut.endline();
  return ConvertResult(std::move(loadPipelineResult));
}

void DisplayDefaultHelp()
{
  cliOut << "Options:\n";
  cliOut << fmt::format("\t {}|{} <pipeline filepath> [{}|{} <log filepath>]\t", k_ExecuteParamLong, k_ExecuteParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Execute the pipeline at the target filepath. Optionally, create a log file at the specified path.\n";
  cliOut << fmt::format("\t {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_PreflightParamLong, k_PreflightParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Preflight the pipeline at the target filepath. Optionally, create a log file at the specified path.\n";
  cliOut << fmt::format("\t {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_ConvertParamLong, k_ConvertParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Convert the SIMPL pipeline at the target filepath. Optionally, create a log file at the specified path.";
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

void DisplayConvertHelp()
{
  cliOut << "To convert a target SIMPL pipeline file:\n\t";
  cliOut << fmt::format("\t {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_ConvertParamLong, k_ConvertParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Convert the SIMPL pipeline at the target filepath. Optionally, create a log file at the specified path.";
  cliOut.endline();
}

void DisplayConvertOutputHelp()
{
  cliOut << "To convert and save a target SIMPL pipeline file:\n\t";
  cliOut << fmt::format("\t {}|{} <pipeline filepath>  [{}|{} <log filepath>]\t", k_ConvertOutputParamLong, k_ConvertOutputParamShort, k_LogFileParamLong, k_LogFileParamShort)
         << "\t Convert the SIMPL pipeline at the target filepath and saves the converted version with the updated extension. Optionally, create a log file at the specified path.";
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
  case ArgumentType::ConvertOutput: {
    DisplayConvertOutputHelp();
    return {};
  }
  case ArgumentType::Logfile: {
    DisplayLogfileHelp();
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

Result<> CreateArgumentError(const Argument& argument)
{
  std::string errorMessage = fmt::format("Failed to parse argument: {}", argument.value);
  return nx::core::MakeErrorResult(k_InvalidArgumentError, errorMessage);
}

Result<> SetLogFile(const Argument& argument)
{
  std::filesystem::path filepath(argument.value);
  return cliOut.setLogFile(filepath);
}

#if SIMPLNX_EMBED_PYTHON
std::vector<std::string> GetPythonPluginList()
{
  auto* var = std::getenv("SIMPLNX_PYTHON_PLUGINS");
  if(var == nullptr)
  {
    return {};
  }

  return StringUtilities::split(var, ';');
}
#endif
} // namespace

int main(int argc, char* argv[])
{
  cliOut << fmt::format("nxrunner: Version {} Build Date:{}\n\n", nx::core::Version::Package(), nx::core::Version::BuildDate());
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
    case ArgumentType::Invalid: {
      results.push_back(CreateArgumentError(argument));
      break;
    }
    case ArgumentType::Logfile: {
      results.push_back(SetLogFile(argument));
      break;
    }
    case ArgumentType::Convert: {
      [[fallthrough]];
    }
    case ArgumentType::ConvertOutput: {
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

  // Load the Simplnx Application instance and load the plugins
  auto app = nx::core::Application::GetOrCreateInstance();
  LoadApp();

#if SIMPLNX_EMBED_PYTHON
  py::scoped_interpreter guard{};

  try
  {
    auto cx = py::module_::import(SIMPLNX_PYTHON_MODULE);

    auto pythonPlugins = GetPythonPluginList();

    fmt::print("Loading Python plugins: {}\n", pythonPlugins);

    for(const auto& pluginName : pythonPlugins)
    {
      fmt::print("Attempting to load Python plugin: '{}'\n", pluginName);
      auto mod = py::module_::import(pluginName.c_str());
      cx.attr("load_python_plugin")(mod);
      auto pluginPath = mod.attr("__file__").cast<std::string>();
      fmt::print("Successfully loaded Python plugin '{}' from '{}'\n", pluginName, pluginPath);
    }
  } catch(const py::error_already_set& exception)
  {
    fmt::print("Python exception while importing plugins: {}\n", exception.what());
    return 1;
  } catch(const std::exception& exception)
  {
    fmt::print("C++ exception while importing plugins: {}\n", exception.what());
    return 1;
  }
#endif

  int errorCode = 0;
  // Run target operation
  switch(arguments[0].type)
  {
  case ArgumentType::Help: {
    PrintResult(DisplayHelpMenu(arguments));
    return errorCode;
  }
  case ArgumentType::Execute: {
    try
    {
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
    auto result = ConvertPipeline(arguments[0]);
    results.push_back(result);
    break;
  }
  case ArgumentType::ConvertOutput: {
    auto result = ConvertPipeline(arguments[0], true);
    results.push_back(result);
    break;
  }
  default: {
    break;
  }
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
