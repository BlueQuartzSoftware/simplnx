#include "ArraySelectionParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/IDataArray.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
{
ArraySelectionParameter::ArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, bool allowEmpty)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
, m_AllowEmpty(allowEmpty)
{
}

Uuid ArraySelectionParameter::uuid() const
{
  return ParameterTraits<ArraySelectionParameter>::uuid;
}

IParameter::AcceptedTypes ArraySelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json ArraySelectionParameter::toJson(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> ArraySelectionParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ArraySelectionParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a jsonStrValue", prefix.view(), name()));
  }
  auto jsonStrValue = json.get<std::string>();
  if(jsonStrValue.empty())
  {
    return {{DataPath()}};
  }
  std::optional<DataPath> path = DataPath::FromString(jsonStrValue);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath", prefix.view(), jsonStrValue));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointer ArraySelectionParameter::clone() const
{
  return std::make_unique<ArraySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowEmpty);
}

std::any ArraySelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename ArraySelectionParameter::ValueType ArraySelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> ArraySelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> ArraySelectionParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

  if(value.empty() && m_AllowEmpty)
  {
    return {};
  }

  if(value.empty())
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* object = dataStructure.getData(value);
  if(object == nullptr)
  {
    return complex::MakeErrorResult<>(complex::FilterParameter::Constants::k_Validate_Does_Not_Exist, fmt::format("{}Object does not exist at path '{}'", prefix, value.toString()));
  }

  const IDataArray* dataArray = dynamic_cast<const IDataArray*>(object);
  if(dataArray == nullptr)
  {
    return complex::MakeErrorResult<>(complex::FilterParameter::Constants::k_Validate_Type_Error, fmt::format("{}Object at path '{}' must be a DataArray.", prefix, value.toString()));
  }

  return {};
}

Result<std::any> ArraySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {{object}};
}
} // namespace complex
