#include "ArraySelectionParameter.hpp"

#include "ArraySelectionValidation.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <utility>

#include <memory>
#include <stdexcept>

namespace nx::core
{
ArraySelectionParameter::ArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                 const AllowedArrayTypes& allowedArrayTypes, const AllowedDataTypes& allowedDataTypes, AllowedComponentShapesType requiredComps, DataLocation location)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
, m_AllowedArrayTypes(allowedArrayTypes)
, m_AllowedDataTypes(allowedDataTypes)
, m_RequiredComponentShapes(std::move(requiredComps))
, m_Location(location)
{
  if(allowedArrayTypes.empty() || allowedArrayTypes.contains(IArray::ArrayType::Any))
  {
    throw std::runtime_error(fmt::format(
        "ArraySelectionParameter requires a nonempty AllowedArrayTypes set of concrete array kinds. Please report this to the developer. \n  Parameter Name:{}\n  Human Name:{}", name, humanName));
  }

  if(allowedDataTypes.dataTypes().empty() && (allowedArrayTypes.contains(IArray::ArrayType::DataArray) || allowedArrayTypes.contains(IArray::ArrayType::NeighborListArray)))
  {
    throw std::runtime_error(fmt::format(
        "ArraySelectionParameter requires a nonempty AllowedDataTypes set when DataArray or NeighborListArray is allowed. Please report this to the developer. \n  Parameter Name:{}\n  Human Name:{}",
        name, humanName));
  }
}

Uuid ArraySelectionParameter::uuid() const
{
  return ParameterTraits<ArraySelectionParameter>::uuid;
}

IParameter::AcceptedTypesType ArraySelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

IParameter::VersionType ArraySelectionParameter::getVersion() const
{
  return 1;
}

nlohmann::json ArraySelectionParameter::toJsonImpl(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> ArraySelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'ArraySelectionParameter' JSON Error: ";
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

IParameter::UniquePointerType ArraySelectionParameter::clone() const
{
  return std::make_unique<ArraySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedArrayTypes, m_AllowedDataTypes, m_RequiredComponentShapes, m_Location);
}

std::any ArraySelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename ArraySelectionParameter::ValueType ArraySelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

ArraySelectionParameter::AllowedArrayTypes ArraySelectionParameter::allowedArrayTypes() const
{
  return m_AllowedArrayTypes;
}

DataTypeSetType ArraySelectionParameter::allowedDataTypes() const
{
  return m_AllowedDataTypes.dataTypes();
}

ArraySelectionParameter::AllowedComponentShapesType ArraySelectionParameter::requiredComponentShapes() const
{
  return m_RequiredComponentShapes;
}

bool ArraySelectionParameter::allowsOutOfCore() const
{
  return (m_Location == DataLocation::Any) || (m_Location == DataLocation::OutOfCore);
}

bool ArraySelectionParameter::allowsInMemory() const
{
  return (m_Location == DataLocation::Any) || (m_Location == DataLocation::InMemory);
}

ArraySelectionParameter::DataLocation ArraySelectionParameter::allowedDataLocations() const
{
  return m_Location;
}

Result<> ArraySelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> ArraySelectionParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
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

  const auto* iArrayPtr = dynamic_cast<const IArray*>(objectPtr);
  if(iArrayPtr == nullptr)
  {
    return nx::core::MakeErrorResult<>(nx::core::filter_parameter::constants::k_ValidateTypeError, fmt::format("{}Object at path '{}' must be an array.", prefix, value.toString()));
  }

  if(auto error = detail::ValidateArraySelection(*iArrayPtr, value, m_AllowedArrayTypes, m_AllowedDataTypes.dataTypes(), m_RequiredComponentShapes, prefix); error.has_value())
  {
    return MakeErrorResult(error->code, std::move(error->message));
  }

  const auto* dataArray = dynamic_cast<const IDataArray*>(iArrayPtr);
  if(dataArray != nullptr && m_Location != DataLocation::Any)
  {
    const IDataStore::StoreType storeType = dataArray->getStoreType();
    const bool matches =
        (m_Location == DataLocation::InMemory && storeType == IDataStore::StoreType::InMemory) || (m_Location == DataLocation::OutOfCore && storeType == IDataStore::StoreType::OutOfCore);
    if(storeType != IDataStore::StoreType::Empty && !matches)
    {
      const char* actualLocationNamePtr = "unsupported";
      if(storeType == IDataStore::StoreType::InMemory)
      {
        actualLocationNamePtr = "in-memory";
      }
      else if(storeType == IDataStore::StoreType::OutOfCore)
      {
        actualLocationNamePtr = "out-of-core";
      }

      const char* requestedLocationNamePtr = "unsupported";
      if(m_Location == DataLocation::InMemory)
      {
        requestedLocationNamePtr = "in-memory";
      }
      else

          if(m_Location == DataLocation::OutOfCore)
      {

        requestedLocationNamePtr = "out-of-core";
      }

      return MakeErrorResult(filter_parameter::constants::k_ValidateDataLocationError,
                             fmt::format("{}DataArray at path '{}' has '{}' storage; required storage is '{}'.", prefix, value.toString(), actualLocationNamePtr, requestedLocationNamePtr));
    }
  }
  return {};
}

Result<std::any> ArraySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* objectPtr = dataStructure.getData(path);
  return {{objectPtr}};
}

namespace simpl_conversion
{
Result<DataArraySelectionFilterParameterConverter::ValueType> DataArraySelectionFilterParameterConverter::Convert(const nlohmann::json& json)
{
  auto dataContainerNameResult = ReadDataContainerName(json, "DataArraySelectionFilterParameter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }

  auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "DataArraySelectionFilterParameter");
  if(attributeMatrixNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
  }

  auto dataArrayNameResult = ReadDataArrayName(json, "DataArraySelectionFilterParameter");
  if(dataArrayNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
  }

  DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value()), std::move(dataArrayNameResult.value())});

  return {std::move(dataPath)};
}
} // namespace simpl_conversion
} // namespace nx::core
