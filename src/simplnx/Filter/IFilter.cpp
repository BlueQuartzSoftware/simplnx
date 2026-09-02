#include "IFilter.hpp"

#include "simplnx/Filter/DataParameter.hpp"
#include "simplnx/Filter/ValueParameter.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Computes the integer percent used to drive a progress bar, clamped to [0, 100]. A zero
 * denominator yields 0 rather than dividing by zero.
 * @param current
 * @param max
 * @return
 */
int32 CalculateProgressBarValue(usize current, usize max)
{
  if(max == 0)
  {
    return 0;
  }
  auto percent = static_cast<int32>(static_cast<float64>(current) / static_cast<float64>(max) * 100.0);
  return std::clamp(percent, 0, 100);
}
} // namespace

void IFilter::MessageHandler::sendProgressMessage(std::string message, int32 percent) const
{
  sendMessage(Message{Message::Type::Progress, std::move(message), std::clamp(percent, 0, 100)});
}

void IFilter::MessageHandler::sendProgressCount(std::string label, usize current, usize max) const
{
  sendMessage(Message{Message::Type::Progress, fmt::format("{}: {}/{}", label, current, max), CalculateProgressBarValue(current, max)});
}

void IFilter::MessageHandler::sendProgressPercent(std::string label, usize current, usize max, int32 decimals) const
{
  const float64 percent = (max == 0) ? 0.0 : (static_cast<float64>(current) / static_cast<float64>(max) * 100.0);
  sendMessage(Message{Message::Type::Progress, fmt::format("{}: {:.{}f}%", label, percent, decimals), CalculateProgressBarValue(current, max)});
}

namespace
{
template <class T>
void moveResult(nx::core::Result<T>& result, std::vector<nx::core::Error>& errors, std::vector<nx::core::Warning>& warnings)
{
  for(auto& warning : result.warnings())
  {
    warnings.push_back(std::move(warning));
  }
  if(!result.valid())
  {
    for(auto& error : result.errors())
    {
      errors.push_back(std::move(error));
    }
  }
}

std::pair<Arguments, std::vector<Warning>> GetResolvedArgs(const Arguments& filterArgs, const Parameters& params, const IFilter& filter, const ExecutionContext& executionContext)
{
  Arguments resolvedArgs;
  std::vector<Warning> warnings;

  for(const auto& [name, arg] : filterArgs)
  {
    if(!params.contains(name))
    {
      warnings.push_back(Warning{-1, fmt::format("The list of arguments for Filter '{}' contained the argument key '{}' which is not an accepted argument key. The accepted Keys are:\n{}",
                                                 filter.humanName(), name, fmt::join(params.getKeys(), ", "))});

      continue;
    }
    resolvedArgs.insert(name, arg);
  }

  for(const auto& [name, parameter] : params)
  {
    if(!filterArgs.contains(name))
    {
      resolvedArgs.insert(name, parameter->defaultValue());
    }
  }

  Arguments constructedArgs;
  for(const auto& [name, parameter] : params)
  {
    constructedArgs.insert(name, parameter->construct(resolvedArgs, executionContext));
  }

  return {std::move(constructedArgs), std::move(warnings)};
}

std::pair<std::map<std::string, std::vector<std::string>>, std::set<std::string>> GetGroupedParameters(const Parameters& params, const Arguments& args)
{
  std::set<std::string> ungroupedParameters;
  for(const auto& [name, parameter] : params)
  {
    ungroupedParameters.insert(name);
  }

  std::map<std::string, std::vector<std::string>> groupedParameters;

  std::vector<std::string> groupKeys = params.getGroupKeys();
  for(const auto& groupKey : groupKeys)
  {
    ungroupedParameters.erase(groupKey);
    std::vector<std::string> childKeys = params.getKeysInGroup(groupKey);
    for(const auto& childKey : childKeys)
    {
      ungroupedParameters.erase(childKey);
    }
    groupedParameters.insert({groupKey, std::move(childKeys)});
  }

  return {std::move(groupedParameters), std::move(ungroupedParameters)};
}

Result<> ValidateParameter(std::string_view name, const AnyParameter& parameter, const Arguments& args, const DataStructure& data, const IFilter& filter)
{
  const auto& arg = args.at(name);

  IParameter::AcceptedTypes acceptedTypes = parameter->acceptedTypes();
  if(std::find(acceptedTypes.cbegin(), acceptedTypes.cend(), arg.type()) == acceptedTypes.cend())
  {
    std::stringstream acceptedTypesStr;
    for(const auto& acceptedType : acceptedTypes)
    {
      acceptedTypesStr << "  " << acceptedType.name() << std::endl;
    }
    throw std::invalid_argument(fmt::format("A mismatch between the argument types for a parameter was detected. This can happen if the improper type is specified when creating a parameter "
                                            "argument or if this filter is being called from another filter where the other filter is NOT using the correct parameter type.\n  Filter='{}'\n  "
                                            "Parameter Name:'{}'\n  Argument Name='{}'\n Argument Type: '{}'.\n The accepted types for this parameter are:\n",
                                            filter.humanName(), parameter->humanName(), name, arg.type().name(), acceptedTypesStr.str()));
  }

  switch(parameter->type())
  {
  case IParameter::Type::Value: {
    const auto& valueParameter = dynamic_cast<const ValueParameter&>(parameter.getRef());
    Result result = valueParameter.validate(arg);
    return result;
  }
  case IParameter::Type::Data: {
    const auto& dataStructureParameter = dynamic_cast<const DataParameter&>(parameter.getRef());
    Result result = dataStructureParameter.validate(data, arg);
    return result;
  }
  default:
    throw std::runtime_error("Invalid parameter type");
  }
}
} // namespace

namespace nx::core
{
IFilter::~IFilter() noexcept = default;

IFilter::PreflightResult IFilter::preflight(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                            const ExecutionContext& executionContext) const
{
  Parameters params = parameters();

  std::vector<Error> errors;

  auto [resolvedArgs, warnings] = GetResolvedArgs(args, params, *this, executionContext);

  auto [groupedParameters, ungroupedParameters] = GetGroupedParameters(params, resolvedArgs);

  for(const auto& [groupKey, dependentKeys] : groupedParameters)
  {
    const auto& parameter = params.at(groupKey);
    Result<> result = ValidateParameter(groupKey, parameter, resolvedArgs, data, *this);
    if(!ExtractResult(std::move(result), errors, warnings))
    {
      continue;
    }
    // Only validate dependent parameters if their parent is valid
    for(const auto& key : dependentKeys)
    {
      const auto& dependentParameter = params.at(key);
      if(!params.isParameterActive(key, resolvedArgs))
      {
        continue;
      }
      Result<> dependentResult = ValidateParameter(key, dependentParameter, resolvedArgs, data, *this);
      if(!ExtractResult(std::move(dependentResult), errors, warnings))
      {
        continue;
      }
    }
  }

  // Validate ungrouped parameters
  for(const auto& name : ungroupedParameters)
  {
    const auto& parameter = params.at(name);
    Result<> result = ValidateParameter(name, parameter, resolvedArgs, data, *this);

    if(!ExtractResult(std::move(result), errors, warnings))
    {
      continue;
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors)), std::move(warnings)};
  }

  PreflightResult implResult = preflightImpl(data, resolvedArgs, messageHandler, shouldCancel, executionContext);
  if(shouldCancel)
  {
    return {MakeErrorResult<OutputActions>(-1, "Filter cancelled")};
  }

  for(auto&& warning : warnings)
  {
    implResult.outputActions.warnings().push_back(std::move(warning));
  }

  return implResult;
}

IFilter::ExecuteResult IFilter::execute(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineFilter, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  PreflightResult preflightResult = preflight(dataStructure, filterArgs, messageHandler, shouldCancel, executionContext);
  if(preflightResult.outputActions.invalid())
  {
    return ExecuteResult{ConvertResult(std::move(preflightResult.outputActions)), std::move(preflightResult.outputValues)};
  }

  OutputActions outputActions = std::move(preflightResult.outputActions.value());

  Result<> outputActionsResult = ConvertResult(std::move(preflightResult.outputActions));

  Result<> actionsResult = outputActions.applyRegular(dataStructure, IDataAction::Mode::Execute);

  Result<> preflightActionsResult = MergeResults(std::move(outputActionsResult), std::move(actionsResult));

  if(preflightActionsResult.invalid())
  {
    return ExecuteResult{std::move(preflightActionsResult), std::move(preflightResult.outputValues)};
  }

  Parameters params = parameters();
  // We can discard the warnings since they're already reported in preflight
  auto [resolvedArgs, warnings] = GetResolvedArgs(filterArgs, params, *this, executionContext);

  Result<> executeImplResult = executeImpl(dataStructure, resolvedArgs, pipelineFilter, messageHandler, shouldCancel, executionContext);
  if(shouldCancel)
  {
    return {MakeErrorResult(-1, "Filter cancelled")};
  }

  Result<> preflightActionsExecuteResult = MergeResults(std::move(preflightActionsResult), std::move(executeImplResult));

  if(preflightActionsExecuteResult.invalid())
  {
    return ExecuteResult{std::move(preflightActionsExecuteResult), std::move(preflightResult.outputValues)};
  }
  // Apply any deferred actions
  Result<> deferredActionsResult = outputActions.applyDeferred(dataStructure, IDataAction::Mode::Execute);

  // Validate the Geometry and Attribute Matrix objects
  Result<> validGeometryAndAttributeMatrices = MergeResults(dataStructure.validateGeometries(), dataStructure.validateAttributeMatrices());
  validGeometryAndAttributeMatrices = MergeResults(validGeometryAndAttributeMatrices, deferredActionsResult);

  // Merge all the results together.
  Result<> finalResult = MergeResults(std::move(preflightActionsExecuteResult), std::move(validGeometryAndAttributeMatrices));

  return ExecuteResult{std::move(finalResult), std::move(preflightResult.outputValues)};
}

nlohmann::json IFilter::toJson(const Arguments& args) const
{
  nlohmann::json json;
  json["parameters_version"] = parametersVersion();
  Parameters params = parameters();
  for(const auto& [name, param] : params)
  {
    try
    {
      nlohmann::json parameterJson;
      if(args.contains(name))
      {
        parameterJson = param->toJson(args.at(name));
      }
      else
      {
        parameterJson = param->toJson(param->defaultValue());
      }
      json[name] = std::move(parameterJson);
    } catch(const std::exception& e)
    {
      throw std::runtime_error(fmt::format("While serializing the filter '{}' to JSON an exception was thrown with message:\n {}", className(), e.what()));
    }
  }
  return json;
}

Result<Arguments> IFilter::fromJson(const nlohmann::json& json) const
{
  Parameters params = parameters();
  Arguments args;
  std::vector<Error> errors;
  std::vector<Warning> warnings;

  std::vector<std::string> paramKeyNotFound;
  std::vector<std::string> jsonKeyNotFound;

  // Check that each key from the filter's parameters appears in the JSON.
  for(const auto& [name, param] : params)
  {
    if(!json.contains(name))
    {
      warnings.push_back(Warning{-5432, fmt::format("Parameter key not found in JSON for filter: '{}'\n    Parameter Key '{}' missing from the JSON", className(), name)});
      args.insert(name, param->defaultValue());
      paramKeyNotFound.push_back(name);
      continue;
    }
    const auto& jsonValue = json[name];
    Result<std::any> jsonResult = param->fromJson(jsonValue);
    moveResult(jsonResult, errors, warnings);
    if(jsonResult.invalid())
    {
      continue;
    }
    args.insert(name, std::move(jsonResult.value()));
  }

  // Check if any keys from the JSON do NOT appear in the Filter's set of parameters
  for(auto& [key, val] : json.items())
  {
    if(key != "parameters_version" && !params.contains(key))
    {
      warnings.push_back(
          Warning{-5433, fmt::format("JSON has parameter key that does not exist in filter '{}' parameter key set.\n    JSON Key '{}' missing from the parameter list", className(), key)});
      jsonKeyNotFound.push_back(key);
    }
  }

  // Now run an N^2 comparison between those to try and find any commonality, i.e.,
  // is one of the keys pretty "close" to another key. That is what is going to be
  // suggested below. This relies on an algorithm to determine what is "close". This
  // may not be correct or even remotely close.
  auto bestMatches = StringUtilities::FindBestMatches(jsonKeyNotFound, paramKeyNotFound);
  for(const auto& match : bestMatches)
  {
    if(!std::get<0>(match).empty() && !std::get<1>(match).empty())
    {
      warnings.push_back(Warning{-5434, fmt::format("Filter '{}': JSON Parameter Warning\n    JSON Parameter Key '{}' is not an accepted Parameter Key for the filter. Closest match is "
                                                    "'{}' with a match distance of {}.\n    Suggested change is '{}' ==> '{}' (This is *ONLY* a suggestion.)\n    You can open the pipeline file in a "
                                                    "text editor and make the changes if those changes make sense.",
                                                    className(), std::get<0>(match), std::get<1>(match), std::get<2>(match), std::get<0>(match), std::get<1>(match))});
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {std::move(args), std::move(warnings)};
}

std::vector<std::string> IFilter::defaultTags() const
{
  return {};
}

Arguments IFilter::getDefaultArguments() const
{
  Arguments args;

  for(const auto& [key, param] : parameters())
  {
    args.insertOrAssign(key, param->defaultValue());
  }

  return args;
}
} // namespace nx::core
