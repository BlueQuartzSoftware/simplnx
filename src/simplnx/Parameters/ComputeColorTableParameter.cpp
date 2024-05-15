#include "ComputeColorTableParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/util/ColorTable.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
ComputeColorTableParameter::ComputeColorTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid ComputeColorTableParameter::uuid() const
{
  return ParameterTraits<ComputeColorTableParameter>::uuid;
}

IParameter::AcceptedTypes ComputeColorTableParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json ComputeColorTableParameter::toJson(const std::any& value) const
{
  const auto& nameStr = GetAnyRef<ValueType>(value);
  nlohmann::json json = nameStr;
  return json;
}

Result<std::any> ComputeColorTableParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ComputeColorTableParameter' JSON Error: ";
  if(!json.empty() && !json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an object", prefix, name()));
  }

  return {{std::make_any<ValueType>(json.get<ValueType>())}};
}

IParameter::UniquePointer ComputeColorTableParameter::clone() const
{
  return std::make_unique<ComputeColorTableParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any ComputeColorTableParameter::defaultValue() const
{
  return defaultJson();
}

typename ComputeColorTableParameter::ValueType ComputeColorTableParameter::defaultJson() const
{
  return m_DefaultValue;
}

Result<> ComputeColorTableParameter::validate(const std::any& value) const
{
  [[maybe_unused]] const auto& json = GetAnyRef<ValueType>(value);
  return {};
}
} // namespace nx::core
