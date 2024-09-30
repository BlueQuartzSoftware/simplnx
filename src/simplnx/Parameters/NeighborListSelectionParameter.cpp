#include "NeighborListSelectionParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>

using namespace nx::core;

template <>
struct fmt::formatter<nx::core::DataType>
{
  constexpr format_parse_context::iterator parse(format_parse_context& ctx)
  {
    return ctx.begin();
  }

  format_context::iterator format(const nx::core::DataType& value, format_context& ctx) const
  {
    return fmt::format_to(ctx.out(), "{}", nx::core::DataTypeToString(value));
  }
};

namespace
{
constexpr int32 k_Validate_AllowedType_Error = -206;
} // namespace

namespace nx::core
{
NeighborListSelectionParameter::NeighborListSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                               const AllowedTypes& allowedTypes)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
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

IParameter::AcceptedTypes NeighborListSelectionParameter::acceptedTypes() const
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

Result<std::any> NeighborListSelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'NeighborListSelectionParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a string", prefix.view(), name()));
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

IParameter::UniquePointer NeighborListSelectionParameter::clone() const
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

NeighborListSelectionParameter::AllowedTypes NeighborListSelectionParameter::allowedTypes() const
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
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* object = dataStructure.getData(value);
  if(object == nullptr)
  {
    return nx::core::MakeErrorResult<>(nx::core::FilterParameter::Constants::k_Validate_Does_Not_Exist, fmt::format("{}Object does not exist at path '{}'", prefix, value.toString()));
  }

  const auto* neighborList = dynamic_cast<const INeighborList*>(object);
  if(neighborList == nullptr)
  {
    return nx::core::MakeErrorResult<>(nx::core::FilterParameter::Constants::k_Validate_Type_Error, fmt::format("{}Object at path '{}' is not a neighbor list.", prefix, value.toString()));
  }

  if(!m_AllowedTypes.empty())
  {
    DataType dataType = neighborList->getDataType();
    if(m_AllowedTypes.count(dataType) == 0)
    {
      return nx::core::MakeErrorResult(k_Validate_AllowedType_Error,
                                       fmt::format("{}DataNeighborList at path '{}' was of type '{}', but only {} are allowed", prefix, value.toString(), dataType, m_AllowedTypes));
    }
  }

  return {};
}

Result<std::any> NeighborListSelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {{object}};
}
} // namespace nx::core
