#include "GeometrySelectionParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
GeometrySelectionParameter ::GeometrySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                        const AllowedTypes& allowTypes)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(defaultValue)
, m_AllowedTypes(allowTypes)
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
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("JSON value for key '{}' is not a string", name()));
  }
  auto string = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(string);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(-3, fmt::format("Failed to parse '{}' as DataPath", string));
  }
  return {std::move(*path)};
}

IParameter::UniquePointer GeometrySelectionParameter::clone() const
{
  return std::make_unique<GeometrySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedTypes);
}

std::any GeometrySelectionParameter::defaultValue() const
{
  return defaultPath();
}

GeometrySelectionParameter::AllowedTypes GeometrySelectionParameter::getAllowedTypes() const
{
  return m_AllowedTypes;
}

typename GeometrySelectionParameter::ValueType GeometrySelectionParameter::defaultPath() const
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
  if(value.empty())
  {
    return complex::MakeErrorResult<>(-1, "DataPath cannot be empty");
  }

  const DataObject* object = dataStructure.getData(value);
  if(object == nullptr)
  {
    return complex::MakeErrorResult<>(-2, fmt::format("Object does not exists at path '{}'", value.toString()));
  }

  const AbstractGeometry* abstractGeometry = dataStructure.getDataAs<AbstractGeometry>(value);
  if(abstractGeometry == nullptr)
  {
    return complex::MakeErrorResult<>(-2, fmt::format("Object at path '{}' is not a subclass of AbstractGeometry.", value.toString()));
  }

  // First look for DataObject::Type::Any, if that is in the allowed types then it doesn't
  // matter what else is in the
  if(std::find(m_AllowedTypes.begin(), m_AllowedTypes.end(), DataObject::Type::Any) != std::end(m_AllowedTypes))
  {
    return {};
  }
  // Look for the actual geometry type that the user selected in the allowed list
  if(std::find(m_AllowedTypes.begin(), m_AllowedTypes.end(), object->getDataObjectType()) != std::end(m_AllowedTypes))
  {
    return {};
  }

  return complex::MakeErrorResult<>(-2, fmt::format("Object at path '{}' is not an accepted geometry type.", value.toString()));
}

Result<std::any> GeometrySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  ImageGeom* object = dataStructure.getDataAs<ImageGeom>(path);
  return {object};
}
} // namespace complex
