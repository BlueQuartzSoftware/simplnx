#include "DataGroupCreationParameter.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
DataGroupCreationParameter::DataGroupCreationParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                       const AllowedParentGroupType& allowedParentGroup)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(defaultValue)
, m_AllowedParentGroupType(allowedParentGroup)
{
}

Uuid DataGroupCreationParameter::uuid() const
{
  return ParameterTraits<DataGroupCreationParameter>::uuid;
}

IParameter::AcceptedTypes DataGroupCreationParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json DataGroupCreationParameter::toJson(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> DataGroupCreationParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'DataGroupCreationParameter' JSON Error: ";

  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}The JSON data entry for key '{}' is not a string.", prefix.view(), name()));
  }

  auto valueString = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(valueString);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath for key '{}'.", prefix.view(), valueString, name()));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointer DataGroupCreationParameter::clone() const
{
  return std::make_unique<DataGroupCreationParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedParentGroupType);
}

std::any DataGroupCreationParameter::defaultValue() const
{
  return defaultPath();
}

typename DataGroupCreationParameter::ValueType DataGroupCreationParameter::defaultPath() const
{
  return m_DefaultValue;
}

DataGroupCreationParameter::AllowedParentGroupType DataGroupCreationParameter::allowedParentGroupType() const
{
  return m_AllowedParentGroupType;
}

Result<> DataGroupCreationParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> DataGroupCreationParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

  if(value.empty())
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* object = dataStructure.getData(value);
  if(object != nullptr)
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_ExistingValue, fmt::format("{}Object exists at path '{}'", prefix, value.toString()));
  }

  DataPath parentPath = value.getParent();
  if(parentPath.empty())
  {
    return {};
  }

  const auto* parentGroup = dataStructure.getDataAs<BaseGroup>(parentPath);
  if(parentGroup == nullptr)
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_ExistingValue, fmt::format("{}Parent path '{}' is not a BaseGroup type", prefix, parentPath.toString()));
  }

  if(m_AllowedParentGroupType.empty() || m_AllowedParentGroupType.find(BaseGroup::GroupType::BaseGroup) != m_AllowedParentGroupType.end())
  {
    return {};
  }

  if(m_AllowedParentGroupType.find(parentGroup->getGroupType()) == m_AllowedParentGroupType.end())
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_ExistingValue,
                                    fmt::format("{}Parent path '{}' is of type {} but only {} types are allowed", prefix, parentPath.toString(),
                                                BaseGroup::StringListFromGroupType(AllowedParentGroupType{parentGroup->getGroupType()}), BaseGroup::StringListFromGroupType(m_AllowedParentGroupType)));
  }

  return {};
}

Result<std::any> DataGroupCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {{object}};
}
} // namespace complex
