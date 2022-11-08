#include "CommentParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
CommentParameter::CommentParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid CommentParameter::uuid() const
{
  return ParameterTraits<CommentParameter>::uuid;
}

IParameter::AcceptedTypes CommentParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json CommentParameter::toJson(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  nlohmann::json json = stringValue;
  return json;
}

Result<std::any> CommentParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CommentParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto string = json.get<ValueType>();
  return {{string}};
}

IParameter::UniquePointer CommentParameter::clone() const
{
  return std::make_unique<CommentParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any CommentParameter::defaultValue() const
{
  return defaultString();
}

typename CommentParameter::ValueType CommentParameter::defaultString() const
{
  return m_DefaultValue;
}

Result<> CommentParameter::validate(const std::any& value) const
{
  [[maybe_unused]] const auto& stringValue = GetAnyRef<ValueType>(value);
  return {};
}
} // namespace complex
