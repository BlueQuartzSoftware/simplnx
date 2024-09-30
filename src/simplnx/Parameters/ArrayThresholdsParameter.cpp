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

//------------------------------------------------------------------------------
IParameter::VersionType ArrayThresholdsParameter::getVersion() const
{
  return 1;
}

nlohmann::json ArrayThresholdsParameter::toJsonImpl(const std::any& value) const
{
  const auto& thresholds = GetAnyRef<ValueType>(value);
  nlohmann::json json = thresholds.toJson();
  return json;
}

Result<std::any> ArrayThresholdsParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
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

namespace SIMPLConversion
{
namespace
{
constexpr StringLiteral k_DataArrayNameKey = "Attribute Array Name";
constexpr StringLiteral k_AttributeMatrixNameKey = "Attribute Matrix Name";
constexpr StringLiteral k_DataContainerNameKey = "Data Container Name";
constexpr StringLiteral k_ComparisonValueKey = "Comparison Value";
constexpr StringLiteral k_ComparisonOperatorKey = "Comparison Operator";
constexpr StringLiteral k_ThresholdValuesKey = "Comparison Values";
constexpr StringLiteral k_UnionOperatorKey = "Union Operator";
constexpr StringLiteral k_ThresholdsKey = "Thresholds";
constexpr StringLiteral k_InvertedKey = "Invert Comparison";
} // namespace

Result<ComparisonSelectionFilterParameterConverter::ValueType> ComparisonSelectionFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_array())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("ComparisonSelectionFilterParameterConverter json '{}' is not an array", json.dump()));
  }

  ParameterType::ValueType::CollectionType thresholdSet;
  // ArrayThreshold;
  for(const auto& iter : json)
  {
    auto comparisonType = static_cast<ArrayThreshold::ComparisonType>(iter[k_ComparisonOperatorKey].get<int32>());
    if(comparisonType == ArrayThreshold::ComparisonType::LessThan)
    {
      comparisonType = ArrayThreshold::ComparisonType::GreaterThan;
    }
    else if(comparisonType == ArrayThreshold::ComparisonType::GreaterThan)
    {
      comparisonType = ArrayThreshold::ComparisonType::LessThan;
    }
    float64 comparisonValue = iter[k_ComparisonValueKey].get<float64>();
    auto dcName = iter[k_DataContainerNameKey].get<std::string>();
    auto amName = iter[k_AttributeMatrixNameKey].get<std::string>();
    auto daName = iter[k_DataArrayNameKey].get<std::string>();
    DataPath arrayPath({dcName, amName, daName});

    auto thresholdPtr = std::make_shared<ArrayThreshold>();
    thresholdPtr->setArrayPath(arrayPath);
    thresholdPtr->setComparisonType(comparisonType);
    thresholdPtr->setComparisonValue(comparisonValue);

    thresholdSet.push_back(thresholdPtr);
  }

  ParameterType::ValueType value;
  value.setArrayThresholds(thresholdSet);

  return {std::move(value)};
}

namespace
{
bool isArrayThreshold(const nlohmann::json& json)
{
  return !json.contains(k_ThresholdValuesKey);
}

ArrayThreshold::ComparisonType invertComparison(ArrayThreshold::ComparisonType comparison)
{
  switch(comparison)
  {
  case ArrayThreshold::ComparisonType::GreaterThan:
    return ArrayThreshold::ComparisonType::LessThan;
  case ArrayThreshold::ComparisonType::LessThan:
    return ArrayThreshold::ComparisonType::GreaterThan;
  case ArrayThreshold::ComparisonType::Operator_Equal:
    return ArrayThreshold::ComparisonType::Operator_NotEqual;
  default:
    return comparison;
  }
}

std::shared_ptr<ArrayThreshold> convertArrayThreshold(const DataPath& amPath, const nlohmann::json& json)
{
  auto comparisonType = static_cast<ArrayThreshold::ComparisonType>(json[k_ComparisonOperatorKey].get<int32>());
  if(comparisonType == ArrayThreshold::ComparisonType::LessThan)
  {
    comparisonType = ArrayThreshold::ComparisonType::GreaterThan;
  }
  else if(comparisonType == ArrayThreshold::ComparisonType::GreaterThan)
  {
    comparisonType = ArrayThreshold::ComparisonType::LessThan;
  }
  float64 comparisonValue = json[k_ComparisonValueKey].get<float64>();
  auto daName = json[k_DataArrayNameKey].get<std::string>();
  auto daPath = amPath.createChildPath(daName);

  auto value = std::make_shared<ArrayThreshold>();
  value->setArrayPath(daPath);
  value->setComparisonType(comparisonType);
  value->setComparisonValue(comparisonValue);
  return value;
}

std::vector<std::shared_ptr<ArrayThreshold>> flattenSetThreshold(const std::shared_ptr<ArrayThresholdSet>& thresholdSet, bool parentInverted = false)
{
  std::vector<std::shared_ptr<ArrayThreshold>> flattenedArrays;
  auto thresholds = thresholdSet->getArrayThresholds();
  if(thresholds.empty())
  {
    return {};
  }

  bool inverted = thresholdSet->isInverted();
  if(parentInverted)
  {
    inverted = !inverted;
  }
  auto unionOperator = thresholdSet->getUnionOperator();

  for(const auto& threshold : thresholds)
  {
    if(auto set = std::dynamic_pointer_cast<ArrayThresholdSet>(threshold); set != nullptr)
    {
      auto flattened = flattenSetThreshold(set, inverted);
      flattenedArrays.insert(flattenedArrays.end(), flattened.begin(), flattened.end());
    }
    else
    {
      auto arrayThreshold = std::dynamic_pointer_cast<ArrayThreshold>(threshold);
      if(inverted)
      {
        auto comparisonType = invertComparison(arrayThreshold->getComparisonType());
        arrayThreshold->setComparisonType(comparisonType);
        arrayThreshold->setUnionOperator(unionOperator);
      }
      flattenedArrays.push_back(arrayThreshold);
    }
  }

  return flattenedArrays;
}

std::shared_ptr<ArrayThresholdSet> convertSetThreshold(const DataPath& amPath, const nlohmann::json& json)
{
  ArrayThresholdsParameter::ValueType::CollectionType thresholdSet;

  const auto& arrayThresholdsJson = json[k_ThresholdValuesKey];

  for(const auto& iter : arrayThresholdsJson)
  {
    if(isArrayThreshold(iter))
    {
      thresholdSet.push_back(convertArrayThreshold(amPath, iter));
    }
    else
    {
      thresholdSet.push_back(convertSetThreshold(amPath, iter));
    }
  }

  auto unionOperator = static_cast<IArrayThreshold::UnionOperator>(json[k_UnionOperatorKey].get<int32>());
  bool inverted = json[k_InvertedKey].get<bool>();

  auto value = std::make_shared<ArrayThresholdSet>();
  value->setUnionOperator(unionOperator);
  value->setInverted(inverted);
  value->setArrayThresholds(thresholdSet);
  return value;
}
} // namespace

Result<ComparisonSelectionAdvancedFilterParameterConverter::ValueType> ComparisonSelectionAdvancedFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_object())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("ComparisonSelectionFilterParameterConverter json '{}' is not an object", json.dump()));
  }

  if(!json[k_ThresholdsKey].is_array())
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("ComparisonSelectionFilterParameterConverter json '{}' is not an object", json[k_ThresholdsKey].dump()));
  }

  auto dcName = json[k_DataContainerNameKey].get<std::string>();
  auto amName = json[k_AttributeMatrixNameKey].get<std::string>();
  DataPath amPath({dcName, amName});

  ParameterType::ValueType::CollectionType thresholdSet;
  // ArrayThreshold;
  for(const auto& iter : json[k_ThresholdsKey])
  {
    if(isArrayThreshold(iter))
    {
      thresholdSet.push_back(convertArrayThreshold(amPath, iter));
    }
    else
    {
      auto arrayThresholdSet = convertSetThreshold(amPath, iter);
      auto flattenedThresholds = flattenSetThreshold(arrayThresholdSet);
      thresholdSet.insert(thresholdSet.end(), flattenedThresholds.begin(), flattenedThresholds.end());
    }
  }

  ParameterType::ValueType value;
  value.setArrayThresholds(thresholdSet);

  return {std::move(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
