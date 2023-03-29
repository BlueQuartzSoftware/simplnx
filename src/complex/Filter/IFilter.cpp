#include "IFilter.hpp"

#include "complex/Filter/DataParameter.hpp"
#include "complex/Filter/ValueParameter.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <vector>

using namespace complex;

namespace
{
template <class T>
void moveResult(complex::Result<T>& result, std::vector<complex::Error>& errors, std::vector<complex::Warning>& warnings)
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

std::pair<Arguments, std::vector<Warning>> GetResolvedArgs(const Arguments& args, const Parameters& params, const IFilter& filter)
{
  Arguments resolvedArgs;
  std::vector<Warning> warnings;

  for(const auto& [name, arg] : args)
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
    if(!args.contains(name))
    {
      resolvedArgs.insert(name, parameter->defaultValue());
    }
  }

  Arguments constructedArgs;
  for(const auto& [name, parameter] : params)
  {
    constructedArgs.insert(name, parameter->construct(resolvedArgs));
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
                                            "argument.\n  Filter='{}'\n  Parameter Name:'{}'\n  Argument Name='{}'\n Argument Type: '{}'.\n The accepted types for this parameter are:\n",
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

namespace complex
{
IFilter::~IFilter() noexcept = default;

IFilter::PreflightResult IFilter::preflight(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  std::stringstream trace;
  try
  {
    trace << "  [IFilter::preflight] Starting Preflight for filter: " << humanName() << std::endl;
    Parameters params = parameters();

    std::vector<Error> errors;

    trace << "  [IFilter::preflight(" << __LINE__ << ")] GetResolvedArgs " << std::endl;
    auto [resolvedArgs, warnings] = GetResolvedArgs(args, params, *this);

    trace << "  [IFilter::preflight(" << __LINE__ << ")] GetGroupedParameters " << std::endl;
    auto [groupedParameters, ungroupedParameters] = GetGroupedParameters(params, resolvedArgs);

    trace << "  [IFilter::preflight (" << __LINE__ << ")] Validating Grouped Parameters " << std::endl;

    for(const auto& [groupKey, dependentKeys] : groupedParameters)
    {
      trace << "      [IFilter::preflight(" << __LINE__ << ")] Validating Group key: " << groupKey << std::endl;
      const auto& parameter = params.at(groupKey);
      Result<> result = ValidateParameter(groupKey, parameter, resolvedArgs, data, *this);
      trace << "      [IFilter::preflight(" << __LINE__ << ")] ExtractResult " << std::endl;
      if(!ExtractResult(std::move(result), errors, warnings))
      {
        continue;
      }
      // Only validate dependent parameters if their parent is valid
      trace << "      [IFilter::preflight(" << __LINE__ << ")] Only validate dependent parameters if their parent is valid " << std::endl;

      for(const auto& key : dependentKeys)
      {
        trace << "        [IFilter::preflight(" << __LINE__ << ")] Validating Key: " << key << std::endl;
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
    trace << "  [IFilter::preflight (" << __LINE__ << ")] Validating Ungrouped Parameters " << std::endl;

    for(const auto& name : ungroupedParameters)
    {
      trace << "      [IFilter::preflight(" << __LINE__ << ")] Validating parameter name: " << name << std::endl;
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

    trace << "  [IFilter::preflight (" << __LINE__ << ")] Running Preflight on filter: " << humanName() << std::endl;
    PreflightResult implResult = preflightImpl(data, resolvedArgs, messageHandler, shouldCancel);
    if(shouldCancel)
    {
      return {MakeErrorResult<OutputActions>(-1, "Filter cancelled")};
    }

    for(auto&& warning : warnings)
    {
      implResult.outputActions.warnings().push_back(std::move(warning));
    }
    trace << "  [IFilter::preflight (" << __LINE__ << ")] COMPLETE on filter: " << humanName() << std::endl;
    return implResult;
  } catch(const std::exception& ex)
  {
    std::cout << "Exception During Preflight:" << std::endl;
    std::cout << "Exception Message: " << ex.what() << std::endl;
    std::cout << "/************* PREFLIGHT_TRACE *****************/" << std::endl;
    std::cout << trace.str();
    std::cout << "/*************************************************************/" << std::endl;
  }
  return {};
}

IFilter::ExecuteResult IFilter::execute(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineFilter, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  std::stringstream trace;
  try
  {
    trace << "  [IFilter::execute (" << __LINE__ << ")] COMPLETE on filter: " << humanName() << std::endl;

    PreflightResult preflightResult = preflight(data, args, messageHandler, shouldCancel);
    if(preflightResult.outputActions.invalid())
    {
      return ExecuteResult{ConvertResult(std::move(preflightResult.outputActions)), std::move(preflightResult.outputValues)};
    }

    OutputActions outputActions = std::move(preflightResult.outputActions.value());
    trace << "  [IFilter::execute (" << __LINE__ << ")] ConvertResult: " << humanName() << std::endl;

    Result<> outputActionsResult = ConvertResult(std::move(preflightResult.outputActions));
    trace << "  [IFilter::execute (" << __LINE__ << ")] applyRegular: " << humanName() << std::endl;

    Result<> actionsResult = outputActions.applyRegular(data, IDataAction::Mode::Execute);

    trace << "  [IFilter::execute (" << __LINE__ << ")] MergeResults: " << humanName() << std::endl;
    Result<> preflightActionsResult = MergeResults(std::move(outputActionsResult), std::move(actionsResult));

    if(preflightActionsResult.invalid())
    {
      return ExecuteResult{std::move(preflightActionsResult), std::move(preflightResult.outputValues)};
    }

    Parameters params = parameters();
    // We can discard the warnings since they're already reported in preflight

    trace << "  [IFilter::execute (" << __LINE__ << ")] GetResolvedArgs: " << humanName() << std::endl;

    auto [resolvedArgs, warnings] = GetResolvedArgs(args, params, *this);

    trace << "  [IFilter::execute (" << __LINE__ << ")] executeImpl: " << humanName() << std::endl;
    Result<> executeImplResult = executeImpl(data, resolvedArgs, pipelineFilter, messageHandler, shouldCancel);
    if(shouldCancel)
    {
      return {MakeErrorResult(-1, "Filter cancelled")};
    }
    trace << "  [IFilter::execute (" << __LINE__ << ")] MergeResults: " << humanName() << std::endl;

    Result<> preflightActionsExecuteResult = MergeResults(std::move(preflightActionsResult), std::move(executeImplResult));

    if(preflightActionsExecuteResult.invalid())
    {
      return ExecuteResult{std::move(preflightActionsExecuteResult), std::move(preflightResult.outputValues)};
    }

    trace << "  [IFilter::execute (" << __LINE__ << ")] applyDeferred: " << humanName() << std::endl;

    Result<> deferredActionsResult = outputActions.applyDeferred(data, IDataAction::Mode::Execute);
    trace << "  [IFilter::execute (" << __LINE__ << ")] MergeResults: " << humanName() << std::endl;

    Result<> finalResult = MergeResults(std::move(preflightActionsExecuteResult), std::move(deferredActionsResult));
    trace << "  [IFilter::execute (" << __LINE__ << ")] COMPLETE and Returning: " << humanName() << std::endl;

    return ExecuteResult{std::move(finalResult), std::move(preflightResult.outputValues)};
  } catch(const std::exception& ex)
  {
    std::cout << "Exception During Preflight:" << std::endl;
    std::cout << "Exception Message: " << ex.what() << std::endl;
    std::cout << "/************* PREFLIGHT_TRACE *****************/" << std::endl;
    std::cout << trace.str();
    std::cout << "/*************************************************************/" << std::endl;
  }
  return {};
}

nlohmann::json IFilter::toJson(const Arguments& args) const
{
  nlohmann::json json;
  Parameters params = parameters();
  for(const auto& [name, param] : params)
  {
    nlohmann::json parameterJson = param->toJson(args.at(name));
    json[name] = std::move(parameterJson);
  }
  return json;
}

Result<Arguments> IFilter::fromJson(const nlohmann::json& json) const
{
  Parameters params = parameters();
  Arguments args;
  std::vector<Error> errors;
  std::vector<Warning> warnings;
  for(const auto& [name, param] : params)
  {
    if(!json.contains(name))
    {
      warnings.push_back(Warning{-1, fmt::format("JSON does not contain key '{}'. Falling back to default value.", name)});
      args.insert(name, param->defaultValue());
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
} // namespace complex
