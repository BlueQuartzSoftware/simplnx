#include "StackFileListInfoParameter.hpp"

#include <fmt/core.h>

namespace fs = std::filesystem;

namespace complex
{

StackFileListInfoParameter::StackFileListInfoParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid StackFileListInfoParameter::uuid() const
{
  return ParameterTraits<StackFileListInfoParameter>::uuid;
}

IParameter::AcceptedTypes StackFileListInfoParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json StackFileListInfoParameter::toJson(const std::any& value) const
{
  auto stackFileListInfo = std::any_cast<ValueType>(value);
  nlohmann::json json = stackFileListInfo.toJson();
  return json;
}

Result<std::any> StackFileListInfoParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_object())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a string", key)}})};
  }

  ValueType value;
  value.fromJson(jsonValue);
  return {value};
}

IParameter::UniquePointer StackFileListInfoParameter::clone() const
{
  return std::make_unique<StackFileListInfoParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any StackFileListInfoParameter::defaultValue() const
{
  return m_DefaultValue;
}

Result<> StackFileListInfoParameter::validate(const std::any& value) const
{
  auto input = std::any_cast<ValueType>(value);
  if(input.getStartIndex() < input.getEndIndex())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "StartIndex must be greater than EndIndex"}})};
  }
  if(input.getOrdering() < 0 || input.getOrdering() > 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "Ordering must be ZERO (0) or ONE (1)"}})};
  }
  return {};
}

} // namespace complex
