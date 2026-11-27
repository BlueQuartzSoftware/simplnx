#include "NeighborListSelectionParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <utility>

using namespace nx::core;

template <>
struct fmt::formatter<nx::core::DataType>
{
  static constexpr format_parse_context::iterator parse(format_parse_context& ctx)
  {
    return ctx.begin();
  }

  static format_context::iterator format(const nx::core::DataType& value, format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", nx::core::DataTypeToString(value));
  }
};

namespace
{
constexpr int32 k_ValidateAllowedTypeError = -206;
} // namespace

namespace nx::core
{
NeighborListSelectionParameter::NeighborListSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue,
                                                               const AllowedTypeSetType& allowedTypes)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(std::move(defaultValue))
, m_AllowedTypes(allowedTypes)
{
  if(allowedTypes.empty())
  {
    throw std::runtime_error("NeighborListSelectionParameter REQUIRES a non-empty AllowedTypes variable. Please report this to the developer.");
  }
}

Uuid NeighborListSelectionParameter::uuid() const
{
  return ParameterTraits<NeighborListSelectionParameter>::uuid;
}

IParameter::AcceptedTypesType NeighborListSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType NeighborListSelectionParameter::getVersion() const
{
  return 1;
}

nlohmann::json NeighborListSelectionParameter::toJsonImpl(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> NeighborListSelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'NeighborListSelectionParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}JSON value for key '{}' is not a string", k_Prefix.view(), name()));
  }
  auto jsonStrValue = json.get<std::string>();
  if(jsonStrValue.empty())
  {
    return {{DataPath()}};
  }
  std::optional<DataPath> path = DataPath::FromString(jsonStrValue);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotValueType, fmt::format("{}Failed to parse '{}' as DataPath", k_Prefix.view(), jsonStrValue));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointerType NeighborListSelectionParameter::clone() const
{
  return std::make_unique<NeighborListSelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedTypes);
}

std::any NeighborListSelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename NeighborListSelectionParameter::ValueType NeighborListSelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

NeighborListSelectionParameter::AllowedTypeSetType NeighborListSelectionParameter::allowedTypes() const
{
  return m_AllowedTypes;
}

Result<> NeighborListSelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> NeighborListSelectionParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(value.empty())
  {
    return nx::core::MakeErrorResult(nx::core::filter_parameter::constants::k_ValidateEmptyValue, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* objectPtr = dataStructure.getData(value);
  if(objectPtr == nullptr)
  {
    return nx::core::MakeErrorResult<>(nx::core::filter_parameter::constants::k_ValidateDoesNotExist, fmt::format("{}Object does not exist at path '{}'", prefix, value.toString()));
  }

  const auto* neighborListPtr = dynamic_cast<const INeighborList*>(objectPtr);
  if(neighborListPtr == nullptr)
  {
    return nx::core::MakeErrorResult<>(nx::core::filter_parameter::constants::k_ValidateTypeError, fmt::format("{}Object at path '{}' is not a neighbor list.", prefix, value.toString()));
  }

  if(!m_AllowedTypes.empty())
  {
    DataType dataType = neighborListPtr->getDataType();
    if(!m_AllowedTypes.contains(dataType))
    {
      return nx::core::MakeErrorResult(k_ValidateAllowedTypeError,
                                       fmt::format("{}DataNeighborList at path '{}' was of type '{}', but only {} are allowed", prefix, value.toString(), dataType, m_AllowedTypes));
    }
  }

  return {};
}

Result<std::any> NeighborListSelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* objectPtr = dataStructure.getData(path);
  return {{objectPtr}};
}
} // namespace nx::core
