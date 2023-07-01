#include "MultiArraySelectionParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/DataStructure/IArray.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace complex
{
MultiArraySelectionParameter::MultiArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                           const AllowedTypes& allowedTypes, const AllowedDataTypes& allowedDataTypes, AllowedComponentShapes requiredComps)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
, m_AllowedTypes(allowedTypes)
, m_AllowedDataTypes(allowedDataTypes)
, m_RequiredComponentShapes(requiredComps)
{
}

Uuid MultiArraySelectionParameter::uuid() const
{
  return ParameterTraits<MultiArraySelectionParameter>::uuid;
}

IParameter::AcceptedTypes MultiArraySelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json MultiArraySelectionParameter::toJson(const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  nlohmann::json json = nlohmann::json::array();
  for(const auto& path : paths)
  {
    json.push_back(path.toString());
  }
  return json;
}

Result<std::any> MultiArraySelectionParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'MultiArraySelectionParameter' JSON Error: ";
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

IParameter::UniquePointer MultiArraySelectionParameter::clone() const
{
  return std::make_unique<MultiArraySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedTypes, m_AllowedDataTypes, m_RequiredComponentShapes);
}

std::any MultiArraySelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename MultiArraySelectionParameter::ValueType MultiArraySelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

MultiArraySelectionParameter::AllowedTypes MultiArraySelectionParameter::allowedTypes() const
{
  return m_AllowedTypes;
}

MultiArraySelectionParameter::AllowedDataTypes MultiArraySelectionParameter::allowedDataTypes() const
{
  return m_AllowedDataTypes;
}

MultiArraySelectionParameter::AllowedComponentShapes MultiArraySelectionParameter::requiredComponentShapes() const
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
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

  std::vector<Error> errors;
  for(usize i = 0; i < value.size(); i++)
  {
    const auto& path = value.at(i);

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
    const auto* dataArray = dataStructure.getDataAs<IArray>(path);
    if(dataArray == nullptr)
    {
      errors.push_back(Error{FilterParameter::Constants::k_Validate_Type_Error, fmt::format("{}Object at path '{}' is not an IArray type", prefix, path.toString())});
      continue;
    }
    if(m_AllowedTypes.find(IArray::ArrayType::Any) == m_AllowedTypes.end() && m_AllowedTypes.count(dataArray->getArrayType()) == 0)
    {
      errors.push_back(Error{FilterParameter::Constants::k_Validate_Type_Error, fmt::format("{}Array at path '{}' was of type {}, but only {} are allowed", prefix, path.toString(),
                                                                                            dataArray->getTypeName(), IArray::StringListFromArrayType(m_AllowedTypes))});
      continue;
    }
    if(!m_RequiredComponentShapes.empty())
    {
      std::string compStr;
      bool foundMatch = false;
      for(const auto& compShape : m_RequiredComponentShapes)
      {
        if(compShape == dataArray->getComponentShape())
        {
          foundMatch = true;
          break;
        }
        compStr += StringUtilities::number(compShape[0]);
        for(usize j = 1; j < compShape.size(); ++j)
        {
          compStr += " x " + StringUtilities::number(compShape[j]);
        }
        compStr += " or ";
      }
      if(!foundMatch)
      {
        errors.push_back(
            Error{complex::FilterParameter::Constants::k_Validate_TupleShapeValue, fmt::format("{}Object at path '{}' must have a component shape of {}.", prefix, path.toString(), compStr)});
      }
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
  for(const auto& path : paths)
  {
    objects.push_back(dataStructure.getData(path));
  }
  return {{objects}};
}
} // namespace complex
