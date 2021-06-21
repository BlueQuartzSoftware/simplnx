#include "IFilter.hpp"

#include <vector>

#include <fmt/format.h>

namespace complex
{
IFilter::DataCheckResult IFilter::dataCheck(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  Parameters params = parameters();

  if(args.size() != params.size())
  {
    throw std::invalid_argument("Invalid number of arguments");
  }

  std::vector<Error> errors;
  std::vector<Warning> warnings;

  for(auto&& [name, parameter] : params)
  {
    const std::any& arg = args.at(name);

    if(parameter->valueType().hash_code() != arg.type().hash_code())
    {
      throw std::invalid_argument("Invalid argument type");
    }

    switch(parameter->type())
    {
    case IParameter::Type::Value: {
      auto valueParameter = static_cast<ValueParameter*>(parameter.get());
      if(!valueParameter->validate(arg))
      {
        errors.push_back(Error{-1, fmt::format("Invalid arg: {}", name)});
        continue;
      }
      break;
    }
    case IParameter::Type::Data: {
      // resolve data structure args and replace values in copy
      auto dataStructureParameter = static_cast<DataParameter*>(parameter.get());
      if(!dataStructureParameter->validate(data, arg))
      {
        errors.push_back(Error{-2, fmt::format("DataObject does not exist")});
        continue;
      }
      break;
    }
    }
  }

  if(!errors.empty())
  {
    return DataCheckResult{nonstd::make_unexpected(std::move(errors)), std::move(warnings)};
  }

  auto implResult = dataCheckImpl(data, args, messageHandler);

  for(auto&& warning : warnings)
  {
    implResult.warnings.push_back(std::move(warning));
  }

  return implResult;
}

IFilter::ExecuteResult IFilter::execute(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler)
{
  // determine required parameters

  // substitute defaults

  // resolve dependencies

  auto result = dataCheck(data, args, messageHandler);
  if(result.hasErrors())
  {
    return ExecuteResult::makeExecuteResult(std::move(result));
  }

  // apply actions

  return executeImpl(data, args, messageHandler);
}

nlohmann::json IFilter::toJson(const Arguments& args) const
{
  nlohmann::json json;
  Parameters params = parameters();
  for(auto&& [name, param] : params)
  {
    nlohmann::json parameterJson = param->toJson(args.at(name));
    json[name] = std::move(parameterJson);
  }
  return json;
}

nonstd::expected<Arguments, Result> IFilter::fromJson(const nlohmann::json& json) const
{
  Parameters params = parameters();
  Arguments args;
  for(auto&& [name, param] : params)
  {
    std::any value = param->fromJson(json);
    args.insert(name, std::move(value));
  }
  return args;
}
} // namespace complex
