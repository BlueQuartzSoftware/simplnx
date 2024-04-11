#include "IFilter.hpp"

#include "simplnx/Filter/DataParameter.hpp"
#include "simplnx/Filter/ValueParameter.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <limits> // For std::numeric_limits
#include <utility> // For std::pair

using namespace nx::core;

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

IFilter::PreflightResult IFilter::preflight(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  Parameters params = parameters();

  std::vector<Error> errors;

  auto [resolvedArgs, warnings] = GetResolvedArgs(args, params, *this);

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

  PreflightResult implResult = preflightImpl(data, resolvedArgs, messageHandler, shouldCancel);
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

IFilter::ExecuteResult IFilter::execute(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineFilter, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  PreflightResult preflightResult = preflight(dataStructure, args, messageHandler, shouldCancel);
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
  auto [resolvedArgs, warnings] = GetResolvedArgs(args, params, *this);

  Result<> executeImplResult = executeImpl(dataStructure, resolvedArgs, pipelineFilter, messageHandler, shouldCancel);
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
  Parameters params = parameters();
  for(const auto& [name, param] : params)
  {
    nlohmann::json parameterJson = param->toJson(args.at(name));
    json[name] = std::move(parameterJson);
  }
  return json;
}

// Assuming levenshteinDistance function is defined as before
int levenshteinDistance(const std::string &s1, const std::string &s2) {
  const size_t len1 = s1.size(), len2 = s2.size();
  std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

  d[0][0] = 0;
  for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
  for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

  for(unsigned int i = 1; i <= len1; ++i)
    for(unsigned int j = 1; j <= len2; ++j)
      d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1)});

  return d[len1][len2];
}

// Function to find best matching word pairs based on Levenshtein distance
std::vector<std::pair<std::string, std::string>> findBestMatches(const std::vector<std::string> &vec1, const std::vector<std::string> &vec2) {
  std::vector<std::pair<std::string, std::string>> bestPairs;
  for (const auto &word1 : vec1) {
    int bestDistance = std::numeric_limits<int>::max();
    std::string bestMatch;
    for (const auto &word2 : vec2) {
      int currentDistance = levenshteinDistance(word1, word2);
      if (currentDistance < bestDistance) {
        bestDistance = currentDistance;
        bestMatch = word2;
      }
    }
    bestPairs.emplace_back(word1, bestMatch);
  }
  return bestPairs;
}


Result<Arguments> IFilter::fromJson(const nlohmann::json& json) const
{
  Parameters params = parameters();
  Arguments args;
  std::vector<Error> errors;
  std::vector<Warning> warnings;

  std::vector<std::string> paramKeyNotFound;
  std::vector<std::string> jsonKeyNotFound;

  for(const auto& [name, param] : params)
  {
    if(!json.contains(name))
    {
      //warnings.push_back(Warning{-5432, fmt::format("PARAMETER_KEY_NOT_FOUND_IN_JSON | '{}' | Parameter Key '{}' missing from the JSON", className(), name)});
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

  for(auto& [key, val] : json.items())
  {
    if(!params.contains(key))
    {
      //warnings.push_back(Warning{-5433, fmt::format("JSON_KEY_NOT_FOUND_IN_PARAMETER | '{}' | JSON Key '{}' missing from the parameter list", className(), key)});
      jsonKeyNotFound.push_back(key);
      continue;
    }
  }

  auto bestMatches = findBestMatches(jsonKeyNotFound, paramKeyNotFound);
  for(const auto& match: bestMatches)
  {
    if(!match.first.empty() && !match.second.empty())
    {
      warnings.push_back(Warning{-5434, fmt::format("JSON Parameter Mismatch Suggestion: '{}': {}  ===>  {}", className(), match.first, match.second)});
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
