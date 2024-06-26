#include "ArrayThresholdsParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
ArrayThresholdsParameter::ArrayThresholdsParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue,
                                                   AllowedComponentShapes requiredComps)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(defaultValue)
, m_RequiredComponentShapes(requiredComps)
{
}

Uuid ArrayThresholdsParameter::uuid() const
{
  return ParameterTraits<ArrayThresholdsParameter>::uuid;
}

IParameter::AcceptedTypes ArrayThresholdsParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json ArrayThresholdsParameter::toJson(const std::any& value) const
{
  const auto& thresholds = GetAnyRef<ValueType>(value);
  nlohmann::json json = thresholds.toJson();
  return json;
}

Result<std::any> ArrayThresholdsParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ArrayThresholdsParameter' JSON Error: ";
  auto thresholds = ArrayThresholdSet::FromJson(json);
  if(thresholds == nullptr)
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as ArrayThresholdSet", prefix.view(), name()));
  }
  return {{std::move(*thresholds)}};
}

IParameter::UniquePointer ArrayThresholdsParameter::clone() const
{
  return std::make_unique<ArrayThresholdsParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any ArrayThresholdsParameter::defaultValue() const
{
  return defaultPath();
}

typename ArrayThresholdsParameter::ValueType ArrayThresholdsParameter::defaultPath() const
{
  return m_DefaultValue;
}

ArrayThresholdsParameter::AllowedComponentShapes ArrayThresholdsParameter::requiredComponentShapes() const
{
  return m_RequiredComponentShapes;
}

Result<> ArrayThresholdsParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& threshold = GetAnyRef<ValueType>(value);

  return validatePaths(dataStructure, threshold);
}

Result<> ArrayThresholdsParameter::validatePath(const DataStructure& dataStructure, const DataPath& dataPath) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(dataPath.empty())
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* object = dataStructure.getData(dataPath);
  if(object == nullptr)
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_ExistingValue, fmt::format("{}Object does not exist at path '{}'", prefix, dataPath.toString()));
  }

  const auto* dataArray = dynamic_cast<const IDataArray*>(object);
  if(dataArray == nullptr)
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Type_Error, fmt::format("{}Object at path '{}' must be a DataArray.", prefix, dataPath.toString()));
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
      for(usize i = 1; i < compShape.size(); ++i)
      {
        compStr += " x " + StringUtilities::number(compShape[i]);
      }
      compStr += " or ";
    }
    if(!foundMatch)
    {
      return nx::core::MakeErrorResult<>(nx::core::FilterParameter::Constants::k_Validate_TupleShapeValue,
                                         fmt::format("{}Object at path '{}' must have a component shape of {}.", prefix, dataPath.toString(), compStr));
    }
  }

  return {};
}

Result<> ArrayThresholdsParameter::validatePaths(const DataStructure& dataStructure, const ValueType& value) const
{
  auto paths = value.getRequiredPaths();
  for(const auto& path : paths)
  {
    Result<> validationResult = validatePath(dataStructure, path);
    if(validationResult.invalid())
    {
      return validationResult;
    }
  }

  return {};
}

Result<std::any> ArrayThresholdsParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  return {};
}
} // namespace nx::core
