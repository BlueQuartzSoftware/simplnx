#include "MultiPathSelectionParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
MultiPathSelectionParameter::MultiPathSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
{
}

Uuid MultiPathSelectionParameter::uuid() const
{
  return ParameterTraits<MultiPathSelectionParameter>::uuid;
}

IParameter::AcceptedTypes MultiPathSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json MultiPathSelectionParameter::toJson(const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  nlohmann::json json = nlohmann::json::array();
  for(const auto& path : paths)
  {
    json.push_back(std::move(path.toString()));
  }
  return json;
}

Result<std::any> MultiPathSelectionParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'MultiPathSelectionParameter' JSON Error: ";
  if(!json.is_array())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an array", prefix, name()));
  }

  ValueType paths;

  std::vector<Error> errors;

  for(const auto& item : json)
  {
    if(!item.is_string())
    {
      return MakeErrorResult<std::any>(-3, fmt::format("{}JSON value in array is not a string", prefix));
    }
    auto string = item.get<std::string>();
    auto path = DataPath::FromString(string);
    if(!path.has_value())
    {
      errors.push_back(Error{FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath", prefix.view(), string)});
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

IParameter::UniquePointer MultiPathSelectionParameter::clone() const
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
      errors.push_back(Error{FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty at index {}", prefix, i)});
      continue;
    }
    const DataObject* object = dataStructure.getData(path);
    if(object == nullptr)
    {
      errors.push_back(Error{FilterParameter::Constants::k_Validate_Does_Not_Exist, fmt::format("{}Object does not exist at path '{}'", prefix, path.toString())});
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
  for(const auto& path : paths)
  {
    objects.push_back(dataStructure.getData(path));
  }
  return {{objects}};
}

namespace SIMPLConversion
{
Result<SingleToMultiDataPathSelectionFilterParameterConverter::ValueType> SingleToMultiDataPathSelectionFilterParameterConverter::convert(const nlohmann::json& json)
{
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

  auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "DataArrayCreationFilterParameter");
  if(attributeMatrixNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
  }

  if(attributeMatrixNameResult.value().empty() == false)
  {
    dataPath = dataPath.createChildPath(std::move(attributeMatrixNameResult.value()));

    auto dataArrayNameResult = ReadDataArrayName(json, "DataArrayCreationFilterParameter");
    if(dataArrayNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
    }

    if(dataArrayNameResult.value().empty() == false)
    {
      dataPath = dataPath.createChildPath(std::move(dataArrayNameResult.value()));
    }
  }

  dataPaths.push_back(std::move(dataPath));
  return {std::move(dataPaths)};
}

namespace
{
constexpr StringLiteral k_DataContainersKey = "Data Containers";
constexpr StringLiteral k_AttributeMatricesKey = "Attribute Matricies";
constexpr StringLiteral k_DataArraysKey = "Data Arrays";
constexpr StringLiteral k_NameKey = "Name";
constexpr StringLiteral k_FlagKey = "Flag";

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

Result<DataArraysToRemoveConverter::ValueType> DataArraysToRemoveConverter::convert(const nlohmann::json& json)
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
      if(IsFlagged(dcJson))
      {
        DataPath dcPath({std::move(dataContainerNameResult.value())});
        value.push_back(std::move(dcPath));
      }

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

          if(IsFlagged(amIter))
          {
            DataPath amPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});
            value.push_back(std::move(amPath));
          }

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
                DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value()), std::move(dataArrayNameResult.value())});
                value.push_back(std::move(dataPath));
              }
            }
          }
        }
      }
    }
  }

  return {std::move(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
