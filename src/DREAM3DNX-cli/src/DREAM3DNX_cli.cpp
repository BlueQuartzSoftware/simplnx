#include "CliObserver.hpp"

#include "complex/Common/Result.hpp"
#include "complex/Core/Application.hpp"
#include "complex/Pipeline/Pipeline.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
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

inline constexpr StringLiteral k_HelpParamLong = "--help";
inline constexpr StringLiteral k_ExecuteParamLong = "--execute";
inline constexpr StringLiteral k_PreflightParamLong = "--preflight";

inline constexpr StringLiteral k_HelpParamShort = "-h";
inline constexpr StringLiteral k_ExecuteParamShort = "-e";
inline constexpr StringLiteral k_PreflightParamShort = "-p";

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

enum class ArgumentType
{
  Execute,
  Preflight,
  Help
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

Result<CliArguments> ParseParameters(int argc, char* argv[])
{
  if(argc < 2)
  {
    std::string ss = "No arguments to parse";
    return complex::MakeErrorResult<CliArguments>(k_NoArgumentsProvided, ss);
  }

  CliArguments args;

  int index = 1;
  std::string arg = argv[index++];
  if(arg == k_HelpParamLong || arg == k_HelpParamShort)
  {
    args.emplace_back(ArgumentType::Help);

    if(argc < 3)
    {
      return {args};
    }
    else
    {
      arg = argv[index++];
    }
  }

  std::string argStr;
  if(index < argc)
  {
    argStr = argv[index];
  }

  if(arg == k_ExecuteParamLong || arg == k_ExecuteParamShort)
  {
    args.emplace_back(ArgumentType::Execute, argStr);
  }
  else if(arg == k_PreflightParamLong || arg == k_PreflightParamShort)
  {
    args.emplace_back(ArgumentType::Preflight, argStr);
  }

  if(argc > index)
  {
    std::string ss = "Too many arguments to parse";
    return complex::MakeErrorResult<CliArguments>(k_FailedParsingArguments, ss);
  }

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
    std::cout << fmt::format("Warning {}: {}", warning.code, warning.message) << std::endl;
  }
  for(const auto& error : result.errors())
  {
    std::cout << fmt::format("Error {}: {}", error.code, error.message) << std::endl;
  }
  return result.errors()[0].code;
}

Result<> PreflightPipeline(Pipeline& pipeline)
{
  const CLI::PipelineObserver obs(&pipeline);
  std::cout << "\n-------------------------" << std::endl;

  if(!pipeline.preflight())
  {
    std::string ss = "Error preflighting pipeline";
    return complex::MakeErrorResult(-2, ss);
  }

  std::cout << "Finished preflighting pipeline" << std::endl;
  return {};
}

Result<> ExecutePipeline(Pipeline& pipeline)
{
  const CLI::PipelineObserver obs(&pipeline);
  std::cout << "\n-------------------------" << std::endl;

  if(!pipeline.execute())
  {
    std::string ss = "Error executing pipeline";
    return complex::MakeErrorResult(k_ExecutePipelineError, ss);
  }
  std::cout << "Finished executing pipeline" << std::endl;
  return {};
}

Result<> ExecutePipeline(const Argument& arg)
{
  std::string pipelinePath = arg.value;
  auto loadPipelineResult = Pipeline::FromFile(pipelinePath);
  if(loadPipelineResult.invalid())
  {
    std::cout << fmt::format("Could not load pipeline at path: '{}'", pipelinePath) << std::endl;
    return complex::ConvertResult(std::move(loadPipelineResult));
  }

  Pipeline pipeline = loadPipelineResult.value();
  std::cout << fmt::format("Executing pipeline at path: '{}'\n", pipelinePath) << std::endl;
  return ExecutePipeline(pipeline);
}

Result<> PreflightPipeline(const Argument& arg)
{
  std::string pipelinePath = arg.value;
  auto loadPipelineResult = Pipeline::FromFile(pipelinePath);
  if(loadPipelineResult.invalid())
  {
    std::cout << fmt::format("Could not load pipeline at path: '{}'", pipelinePath) << std::endl;
    return complex::ConvertResult(std::move(loadPipelineResult));
  }

  std::cout << fmt::format("Preflighting pipeline at path: '{}'\n", pipelinePath) << std::endl;

  Pipeline pipeline = loadPipelineResult.value();
  return PreflightPipeline(pipeline);
}

void DisplayDefaultHelp()
{
  std::cout << "Options:\n";
  std::cout << fmt::format("\t {} <filepath>\t", k_ExecuteParamLong) << "\t Execute the pipeline at the taget filepath\n";
  std::cout << fmt::format("\t {} <filepath>\t", k_ExecuteParamShort) << "\t Execute the pipeline at the taget filepath\n";
  std::cout << fmt::format("\t {} <filepath>\t", k_PreflightParamLong) << "\t Preflight the pipeline at the taget filepath" << std::endl;
  std::cout << fmt::format("\t {} <filepath>\t", k_PreflightParamShort) << "\t Preflight the pipeline at the taget filepath" << std::endl;
}

void DisplayExecuteHelp()
{
  std::cout << "To execute a target pipeline file:\n\t";
  std::cout << k_ExecuteParamLong.str() << " <pipeline filepath>\n\t";
  std::cout << k_ExecuteParamShort.str() << " <pipeline filepath>" << std::endl;
}

void DisplayPreflightHelp()
{
  std::cout << "To preflight a target pipeline file:\n\t";
  std::cout << k_PreflightParamLong.str() << " <pipeline filepath>\n\t";
  std::cout << k_PreflightParamShort.str() << " <pipeline filepath>" << std::endl;
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
  }

  std::string ss = "Incorrect Help Syntax";
  return complex::MakeErrorResult(k_FailedParsingArguments, ss);
}

int main(int argc, char* argv[])
{
  std::cout << "DREAM.3D NX CLI" << std::endl;
  complex::Application app;
  LoadApp(app);

  Result<CliArguments> parsingResult = ParseParameters(argc, argv);
  if(parsingResult.invalid())
  {
    return PrintResult<CliArguments>(parsingResult);
  }

  CliArguments arguments = parsingResult.value();
  switch(arguments[0].type)
  {
  case ArgumentType::Help:
    return PrintResult(DisplayHelpMenu(arguments));
  case ArgumentType::Execute:
    return PrintResult(ExecutePipeline(arguments[0]));
  case ArgumentType::Preflight:
    return PrintResult(PreflightPipeline(arguments[0]));
  }
}
