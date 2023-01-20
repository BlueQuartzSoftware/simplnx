#include "ExecuteProcessFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ExecuteProcess.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
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
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ExecuteProcessFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<StringParameter>(k_Arguments_Key, "Command Line Arguments", "The complete command to execute.", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Blocking_Key, "Should Block", "Whether to block the process while the command executes or not", false));
  params.insert(std::make_unique<Int32Parameter>(k_Timeout_Key, "Timeout (ms)", "The amount of time to wait for the command to start/finish when blocking is selected", 30000));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_Blocking_Key, k_Timeout_Key, true);

  return params;
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
  auto pBlockingValue = filterArgs.value<bool>(k_Blocking_Key);
  auto pTimeoutValue = filterArgs.value<int32>(k_Timeout_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<std::string> arguments = ExecuteProcess::splitArgumentsString(pArgumentsValue);
  if(arguments.empty())
  {
    return MakePreflightErrorResult(-4001, "No command line arguments have been specified.");
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ExecuteProcessFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  ExecuteProcessInputValues inputValues;

  inputValues.Arguments = filterArgs.value<StringParameter::ValueType>(k_Arguments_Key);
  inputValues.Blocking = filterArgs.value<bool>(k_Blocking_Key);
  inputValues.Timeout = filterArgs.value<int32>(k_Timeout_Key);

  return ExecuteProcess(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
