#include "ExecuteProcessFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ExecuteProcess.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ExecuteProcessFilter::name() const
{
  return FilterTraits<ExecuteProcessFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ExecuteProcessFilter::className() const
{
  return FilterTraits<ExecuteProcessFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExecuteProcessFilter::uuid() const
{
  return FilterTraits<ExecuteProcessFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExecuteProcessFilter::humanName() const
{
  return "Execute Process";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExecuteProcessFilter::defaultTags() const
{
  return {className(), "Core", "Misc"};
}

//------------------------------------------------------------------------------
Parameters ExecuteProcessFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<StringParameter>(k_Arguments_Key, "Command Line Arguments", "The complete command to execute.", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Blocking_Key, "Should Block", "Whether to block the process while the command executes or not", false));
  params.insert(std::make_unique<Int32Parameter>(k_Timeout_Key, "Timeout (ms)", "The amount of time to wait for the command to start/finish when blocking is selected", 30000));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputLogFile_Key, "Output Log File", "The log file where the output from the process will be stored", "Data/Output/ProcessOutput.txt",
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_Blocking_Key, k_Timeout_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ExecuteProcessFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExecuteProcessFilter::clone() const
{
  return std::make_unique<ExecuteProcessFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExecuteProcessFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pArgumentsValue = filterArgs.value<StringParameter::ValueType>(k_Arguments_Key);

  std::vector<std::string> arguments = ExecuteProcess::splitArgumentsString(pArgumentsValue);
  if(arguments.empty())
  {
    return MakePreflightErrorResult(-4001, "No command line arguments have been specified.");
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> ExecuteProcessFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  ExecuteProcessInputValues inputValues;

  inputValues.Arguments = filterArgs.value<StringParameter::ValueType>(k_Arguments_Key);
  inputValues.Blocking = filterArgs.value<bool>(k_Blocking_Key);
  inputValues.Timeout = filterArgs.value<int32>(k_Timeout_Key);
  inputValues.LogFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputLogFile_Key);

  return ExecuteProcess(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ArgumentsKey = "Arguments";
constexpr StringLiteral k_BlockingKey = "Blocking";
constexpr StringLiteral k_TimeoutKey = "Timeout";
} // namespace SIMPL
} // namespace

Result<Arguments> ExecuteProcessFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ExecuteProcessFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_ArgumentsKey, k_Arguments_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_BlockingKey, k_Blocking_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_TimeoutKey, k_Timeout_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
