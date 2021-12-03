#include "ImageGeometrySelectionParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
ImageGeometrySelectionParameter ::ImageGeometrySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(defaultValue)
{
}

Uuid ImageGeometrySelectionParameter::uuid() const
{
  return ParameterTraits<ImageGeometrySelectionParameter>::uuid;
}

IParameter::AcceptedTypes ImageGeometrySelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json ImageGeometrySelectionParameter::toJson(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> ImageGeometrySelectionParameter::fromJson(const nlohmann::json& json) const
{
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("JSON value for key \"{}\" is not a string", name()));
  }
  auto string = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(string);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(-3, fmt::format("Failed to parse \"{}\" as DataPath", string));
  }
  return {std::move(*path)};
}

IParameter::UniquePointer ImageGeometrySelectionParameter::clone() const
{
  return std::make_unique<ImageGeometrySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any ImageGeometrySelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename ImageGeometrySelectionParameter::ValueType ImageGeometrySelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> ImageGeometrySelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> ImageGeometrySelectionParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
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

  const ImageGeom* imageGeom = dynamic_cast<const ImageGeom*>(object);
  if(imageGeom == nullptr)
  {
    return complex::MakeErrorResult<>(-2, fmt::format("Object at path '{}' is not a ImageGeometry.", value.toString()));
  }

  return {};
}

Result<std::any> ImageGeometrySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  ImageGeom* object = dataStructure.getDataAs<ImageGeom>(path);
  return {object};
}
} // namespace complex
