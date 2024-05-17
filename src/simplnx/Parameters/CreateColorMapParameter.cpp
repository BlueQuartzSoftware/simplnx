#include "CreateColorMapParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/util/ColorTable.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
CreateColorMapParameter::CreateColorMapParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid CreateColorMapParameter::uuid() const
{
  return ParameterTraits<CreateColorMapParameter>::uuid;
}

IParameter::AcceptedTypes CreateColorMapParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json CreateColorMapParameter::toJson(const std::any& value) const
{
  const auto& nameStr = GetAnyRef<ValueType>(value);
  nlohmann::json json = nameStr;
  return json;
}

Result<std::any> CreateColorMapParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CreateColorMapParameter' JSON Error: ";
  if(!json.empty() && !json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an object", prefix, name()));
  }

  return {{std::make_any<ValueType>(json.get<ValueType>())}};
}

IParameter::UniquePointer CreateColorMapParameter::clone() const
{
  return std::make_unique<CreateColorMapParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any CreateColorMapParameter::defaultValue() const
{
  return defaultJson();
}

typename CreateColorMapParameter::ValueType CreateColorMapParameter::defaultJson() const
{
  return m_DefaultValue;
}

Result<> CreateColorMapParameter::validate(const std::any& value) const
{
  [[maybe_unused]] const auto& json = GetAnyRef<ValueType>(value);
  return {};
}
} // namespace nx::core
