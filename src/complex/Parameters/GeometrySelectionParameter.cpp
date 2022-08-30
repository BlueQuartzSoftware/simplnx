#include "GeometrySelectionParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace complex
{
GeometrySelectionParameter::GeometrySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                       const AllowedTypes& allowedTypes)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
, m_AllowedTypes(allowedTypes)
{
}

Uuid GeometrySelectionParameter::uuid() const
{
  return ParameterTraits<GeometrySelectionParameter>::uuid;
}

IParameter::AcceptedTypes GeometrySelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json GeometrySelectionParameter::toJson(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> GeometrySelectionParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'GeometrySelectionParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto string = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(string);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath", prefix.view(), string));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointer GeometrySelectionParameter::clone() const
{
  return std::make_unique<GeometrySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedTypes);
}

std::any GeometrySelectionParameter::defaultValue() const
{
  return defaultPath();
}

const GeometrySelectionParameter::AllowedTypes& GeometrySelectionParameter::allowedTypes() const
{
  return m_AllowedTypes;
}

const GeometrySelectionParameter::ValueType& GeometrySelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> GeometrySelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> GeometrySelectionParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

  if(value.empty())
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}Geometry Path cannot be empty", prefix));
  }

  const DataObject* object = dataStructure.getData(value);
  if(object == nullptr)
  {
    return complex::MakeErrorResult<>(complex::FilterParameter::Constants::k_Validate_Does_Not_Exist, fmt::format("{}Object does not exist at path '{}'", prefix, value.toString()));
  }

  const IGeometry* geometry = dynamic_cast<const IGeometry*>(object);
  if(geometry == nullptr)
  {
    return MakeErrorResult(-3, fmt::format("Object at path '{}' is not a subclass of IGeometry.", value.toString()));
  }

  // Look for the actual geometry type that the user selected in the allowed set
  if(m_AllowedTypes.count(geometry->getGeomType()) > 0)
  {
    return {};
  }

  return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_AllowedType_Error,
                                  fmt::format("{}Geometry at path '{}' was of type '{}', but only {} are allowed", prefix, value.toString(), geometry->getTypeName(),
                                              IGeometry::StringListFromGeometryType(m_AllowedTypes)));
}

Result<std::any> GeometrySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  IGeometry* object = dataStructure.getDataAs<IGeometry>(path);
  return {{object}};
}
} // namespace complex
