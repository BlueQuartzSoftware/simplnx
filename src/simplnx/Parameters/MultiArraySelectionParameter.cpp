#include "MultiArraySelectionParameter.hpp"

#include "ArraySelectionValidation.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <utility>

#include <stdexcept>

namespace nx::core
{
MultiArraySelectionParameter::MultiArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                           const AllowedArrayTypes& allowedArrayTypes, const AllowedDataTypesType& allowedDataTypes, AllowedComponentShapesType requiredComps)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
, m_AllowedArrayTypes(allowedArrayTypes)
, m_AllowedDataTypes(allowedDataTypes)
, m_RequiredComponentShapes(requiredComps)
{
  if(allowedArrayTypes.empty())
  {
    throw std::runtime_error(
        fmt::format("MultiArraySelectionParameter requires a nonempty AllowedArrayTypes set. Please report this to the developer.\n  Parameter Name:{}\n  Human Name:{}", name, humanName));
  }

  using Kind = IArray::ArrayType;
  if(allowedDataTypes.dataTypes().empty() && (allowedArrayTypes.contains(Kind::DataArray) || allowedArrayTypes.contains(Kind::NeighborListArray) || allowedArrayTypes.contains(Kind::Any)))
  {
    throw std::runtime_error(fmt::format("MultiArraySelectionParameter requires a nonempty AllowedDataTypes set when DataArray, NeighborListArray or Any is allowed. Please report this to the "
                                         "developer. \n  Parameter Name:{}\n  Human Name:{}",
                                         name, humanName));
  }
}

Uuid MultiArraySelectionParameter::uuid() const
{
  return ParameterTraits<MultiArraySelectionParameter>::uuid;
}

IParameter::AcceptedTypesType MultiArraySelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType MultiArraySelectionParameter::getVersion() const
{
  return 1;
}

nlohmann::json MultiArraySelectionParameter::toJsonImpl(const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  nlohmann::json json = nlohmann::json::array();
  for(const auto& path : paths)
  {
    json.push_back(path.toString());
  }
  return json;
}

Result<std::any> MultiArraySelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'MultiArraySelectionParameter' JSON Error: ";
  if(!json.is_array())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an array", k_Prefix, name()));
  }

  ValueType paths;

  std::vector<Error> errors;

  for(const auto& item : json)
  {
    if(!item.is_string())
    {
      return MakeErrorResult<std::any>(-3, fmt::format("{}JSON value in array is not a string", k_Prefix));
    }
    auto string = item.get<std::string>();
    auto path = DataPath::FromString(string);
    if(!path.has_value())
    {
      errors.push_back(Error{.code = filter_parameter::constants::k_JsonValueNotValueType, .message = fmt::format("{}Failed to parse '{}' as DataPath", k_Prefix.view(), string)});
      continue;
    }
    paths.push_back(std::move(*path));
  }

  if(!errors.empty())
  {
    return {{nonstd::make_unexpected(std::move(errors))}};
  }

  return {{std::move(paths)}};
}

IParameter::UniquePointerType MultiArraySelectionParameter::clone() const
{
  return std::make_unique<MultiArraySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedArrayTypes, m_AllowedDataTypes, m_RequiredComponentShapes);
}

std::any MultiArraySelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename MultiArraySelectionParameter::ValueType MultiArraySelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

MultiArraySelectionParameter::AllowedArrayTypes MultiArraySelectionParameter::allowedArrayTypes() const
{
  return m_AllowedArrayTypes;
}

DataTypeSetType MultiArraySelectionParameter::allowedDataTypes() const
{
  return m_AllowedDataTypes.dataTypes();
}

MultiArraySelectionParameter::AllowedComponentShapesType MultiArraySelectionParameter::requiredComponentShapes() const
{
  return m_RequiredComponentShapes;
}

Result<> MultiArraySelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);

  return validatePaths(dataStructure, paths);
}

Result<> MultiArraySelectionParameter::validatePaths(const DataStructure& dataStructure, const ValueType& value) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  std::vector<Error> errors;
  for(usize i = 0; i < value.size(); i++)
  {
    const auto& path = value.at(i);

    if(path.empty())
    {
      errors.push_back(Error{.code = filter_parameter::constants::k_ValidateEmptyValue, .message = fmt::format("{}DataPath cannot be empty at index {}", prefix, i)});
      continue;
    }
    const DataObject* objectPtr = dataStructure.getData(path);
    if(objectPtr == nullptr)
    {
      errors.push_back(Error{.code = filter_parameter::constants::k_ValidateDoesNotExist, .message = fmt::format("{}Object does not exist at path '{}'", prefix, path.toString())});
      continue;
    }
    const auto* array = dynamic_cast<const IArray*>(objectPtr);
    if(array == nullptr)
    {
      errors.push_back(Error{.code = filter_parameter::constants::k_ValidateTypeError, .message = fmt::format("{}Object at path '{}' is not an IArray type", prefix, path.toString())});
      continue;
    }
    if(auto error = detail::ValidateArraySelection(*array, path, m_AllowedArrayTypes, m_AllowedDataTypes.dataTypes(), m_RequiredComponentShapes, prefix); error.has_value())
    {
      errors.push_back(std::move(*error));
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}

Result<std::any> MultiArraySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  std::vector<DataObject*> objects;
  objects.reserve(paths.size());
  for(const auto& path : paths)
  {
    objects.push_back(dataStructure.getData(path));
  }
  return {{objects}};
}

namespace simpl_conversion
{
Result<MultiDataArraySelectionFilterParameterConverter::ValueType> MultiDataArraySelectionFilterParameterConverter::Convert(const nlohmann::json& json)
{
  if(!json.is_array())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("MultiDataArraySelectionParameter json '{}' is not an array", json.dump()));
  }

  std::vector<DataPath> dataPaths;

  for(const auto& iter : json)
  {
    auto dataContainerNameResult = ReadDataContainerName(iter, "DataContainerSelectionFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    auto attributeMatrixNameResult = ReadAttributeMatrixName(iter, "DataArrayCreationFilterParameter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }

    auto dataArrayNameResult = ReadDataArrayName(iter, "DataArrayCreationFilterParameter");
    if(dataArrayNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value()), std::move(dataArrayNameResult.value())});
    dataPaths.push_back(std::move(dataPath));
  }

  return {std::move(dataPaths)};
}
} // namespace simpl_conversion
} // namespace nx::core
