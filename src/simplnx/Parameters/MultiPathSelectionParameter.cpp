#include "MultiPathSelectionParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace nx::core
{
MultiPathSelectionParameter::MultiPathSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(std::move(defaultValue))
{
}

Uuid MultiPathSelectionParameter::uuid() const
{
  return ParameterTraits<MultiPathSelectionParameter>::uuid;
}

IParameter::AcceptedTypesType MultiPathSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType MultiPathSelectionParameter::getVersion() const
{
  return 1;
}

nlohmann::json MultiPathSelectionParameter::toJsonImpl(const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  nlohmann::json json = nlohmann::json::array();
  for(const auto& path : paths)
  {
    json.push_back(path.toString());
  }
  return json;
}

Result<std::any> MultiPathSelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'MultiPathSelectionParameter' JSON Error: ";
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

IParameter::UniquePointerType MultiPathSelectionParameter::clone() const
{
  return std::make_unique<MultiPathSelectionParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any MultiPathSelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename MultiPathSelectionParameter::ValueType MultiPathSelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> MultiPathSelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);

  return validatePaths(dataStructure, paths);
}

Result<> MultiPathSelectionParameter::validatePaths(const DataStructure& dataStructure, const ValueType& value) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  std::vector<Error> errors;
  for(usize i = 0; i < value.size(); i++)
  {
    const auto& path = value[i];

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
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}

Result<std::any> MultiPathSelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
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
Result<SingleToMultiDataPathSelectionFilterParameterConverter::ValueType> SingleToMultiDataPathSelectionFilterParameterConverter::Convert(const nlohmann::json& json)
{
  static constexpr StringLiteralType k_AttributeMatrixNameKey = "Attribute Matrix Name";
  static constexpr StringLiteralType k_DataArrayNameKey = "Data Array Name";

  if(!json.is_object())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("MultiDataArraySelectionParameter json '{}' is not an array", json.dump()));
  }

  std::vector<DataPath> dataPaths;

  auto dataContainerNameResult = ReadDataContainerName(json, "DataContainerSelectionFilterParameter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }
  DataPath dataPath({std::move(dataContainerNameResult.value())});

  // Attribute Matrix Name is optional — a DataContainerSelectionFilterParameter has only Data Container Name.
  if(json.contains(k_AttributeMatrixNameKey))
  {
    auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "AttributeMatrixSelectionFilterParameter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }

    if(!attributeMatrixNameResult.value().empty())
    {
      dataPath = dataPath.createChildPath(attributeMatrixNameResult.value());

      // Data Array Name is optional — an AttributeMatrixSelectionFilterParameter does not include it.
      if(json.contains(k_DataArrayNameKey))
      {
        auto dataArrayNameResult = ReadDataArrayName(json, "DataArraySelectionFilterParameter");
        if(dataArrayNameResult.invalid())
        {
          return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
        }

        if(!dataArrayNameResult.value().empty())
        {
          dataPath = dataPath.createChildPath(dataArrayNameResult.value());
        }
      }
    }
  }

  dataPaths.push_back(std::move(dataPath));
  return {std::move(dataPaths)};
}

namespace
{
constexpr StringLiteralType k_DataContainersKey = "Data Containers";
constexpr StringLiteralType k_AttributeMatricesKey = "Attribute Matricies";
constexpr StringLiteralType k_DataArraysKey = "Data Arrays";
constexpr StringLiteralType k_NameKey = "Name";
constexpr StringLiteralType k_FlagKey = "Flag";

Result<std::string> FindName(const nlohmann::json& json)
{
  if(!json.contains(k_NameKey))
  {
    return MakeErrorResult<std::string>(-55, fmt::format("DataArrayToRemove json '{}' does not contain '{}'", json.dump(), k_NameKey));
  }
  if(!json[k_NameKey].is_string())
  {
    return MakeErrorResult<std::string>(-55, fmt::format("DataArrayToRemove json '{}' is not a string", json[k_NameKey].dump(), k_NameKey));
  }

  return {json[k_NameKey].get<std::string>()};
}

bool IsFlagged(const nlohmann::json& json)
{
  if(!json.contains(k_FlagKey))
  {
    return false;
  }
  if(!json[k_FlagKey].is_number_integer())
  {
    return false;
  }
  return json[k_FlagKey].get<int32>() != 0;
}
} // namespace

Result<DataArraysToRemoveConverter::ValueType> DataArraysToRemoveConverter::Convert(const nlohmann::json& json)
{
  if(!json.is_object())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("DataArraysToRemove json '{}' is not an object", json.dump()));
  }

  ValueType value;

  if(json.contains(k_DataContainersKey))
  {
    const auto& dcaJson = json[k_DataContainersKey];
    if(!dcaJson.is_array())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("DataArraysToRemove DataContainer json '{}' is not an array", dcaJson.dump()));
    }

    // Data Containers
    for(const auto& dcJson : dcaJson)
    {
      if(!dcJson.is_object())
      {
        return MakeErrorResult<ValueType>(-3, fmt::format("DataArraysToRemove DataContainer json '{}' is not an object", dcJson.dump()));
      }

      auto dataContainerNameResult = FindName(dcJson);
      if(dataContainerNameResult.invalid())
      {
        return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
      }

      bool exportedAsAM = false;

      // Attribute Matrix
      if(dcJson.contains(k_AttributeMatricesKey))
      {
        const auto& amJson = dcJson[k_AttributeMatricesKey];
        if(!amJson.is_array())
        {
          return MakeErrorResult<ValueType>(-4, fmt::format("DataArraysToRemove DataContainer json '{}' is not an array", amJson.dump()));
        }

        for(const auto& amIter : amJson)
        {
          if(!amIter.is_object())
          {
            return MakeErrorResult<ValueType>(-5, fmt::format("DataArraysToRemove AttributeMatrix json '{}' is not an object", amIter.dump()));
          }

          auto attributeMatrixNameResult = FindName(amIter);
          if(attributeMatrixNameResult.invalid())
          {
            return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
          }

          bool exportedAsDA = false;

          // Data Arrays
          if(amIter.contains(k_DataArraysKey))
          {
            const auto& daJson = amIter[k_DataArraysKey];
            if(!daJson.is_array())
            {
              return MakeErrorResult<ValueType>(-6, fmt::format("DataArraysToRemove DataArray json '{}' is not an array", daJson.dump()));
            }

            for(const auto& daIter : daJson)
            {
              if(!daIter.is_object())
              {
                return MakeErrorResult<ValueType>(-7, fmt::format("DataArraysToRemove DataArray json '{}' is not an object", daIter.dump()));
              }

              auto dataArrayNameResult = FindName(daIter);
              if(dataArrayNameResult.invalid())
              {
                return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
              }

              if(IsFlagged(daIter))
              {
                exportedAsDA = true;
                DataPath dataPath({dataContainerNameResult.value(), attributeMatrixNameResult.value(), dataArrayNameResult.value()});
                value.push_back(std::move(dataPath));
              }
            }
          }
          if(IsFlagged(amIter))
          {
            exportedAsAM = true;
            if(!exportedAsDA)
            {
              DataPath amPath({dataContainerNameResult.value(), attributeMatrixNameResult.value()});
              value.push_back(std::move(amPath));
            }
          }
        }
      }
      if(!exportedAsAM && IsFlagged(dcJson))
      {
        DataPath dcPath({dataContainerNameResult.value()});
        value.push_back(std::move(dcPath));
      }
    }
  }

  return {std::move(value)};
}
} // namespace simpl_conversion
} // namespace nx::core
