#include "CreateColorMapParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/util/ColorTable.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace nx::core
{
CreateColorMapParameter::CreateColorMapParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(std::move(defaultValue))
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

//------------------------------------------------------------------------------
IParameter::VersionType CreateColorMapParameter::getVersion() const
{
  return 1;
}

nlohmann::json CreateColorMapParameter::toJsonImpl(const std::any& value) const
{
  const auto& nameStr = GetAnyRef<ValueType>(value);
  nlohmann::json json = nameStr;
  return json;
}

Result<std::any> CreateColorMapParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'CreateColorMapParameter' JSON Error: ";
  if(!json.empty() && !json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an object", k_Prefix, name()));
  }

  return {{std::make_any<ValueType>(json.get<ValueType>())}};
}

IParameter::UniquePointerType CreateColorMapParameter::clone() const
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

namespace simpl_conversion
{
namespace
{
constexpr StringLiteralType k_SelectedPresetKey = "SelectedPresetName";
constexpr StringLiteralType k_PresetNameKey = "Name";
constexpr StringLiteralType k_RGBPointsKey = "RGBPoints";
} // namespace

Result<CreateColorMapFilterParameterConverter::ValueType> CreateColorMapFilterParameterConverter::Convert(const nlohmann::json& json1, const nlohmann::json& json2)
{
  if(!json1.is_string())
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("CreateColorMapFilterParameterConverter json1 '{}' is not a string", json2.dump()));
  }
  if(!json2.is_array())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("CreateColorMapFilterParameterConverter json2 '{}' is not an array", json1.dump()));
  }

  auto presetName = json1.get<std::string>();
  auto rgbPoints = json2.get<std::vector<float64>>();

  nlohmann::json value = nlohmann::json::object();
  value[k_PresetNameKey] = presetName;
  value[k_RGBPointsKey] = rgbPoints;

  return {std::move(value)};
}
} // namespace simpl_conversion
} // namespace nx::core
