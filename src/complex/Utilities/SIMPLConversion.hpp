#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Filter/Arguments.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataTypeParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <nlohmann/json.hpp>

#include <string_view>

namespace complex::SIMPLConversion
{
template <class ConverterT>
Result<> ConvertParameter(Arguments& args, const nlohmann::json& json, std::string_view simplKey, const std::string& complexKey)
{
  if(json.contains(simplKey))
  {
    auto result = ConverterT::convert(json[simplKey]);
    if(result.valid())
    {
      args.insertOrAssign(complexKey, std::make_any<typename ConverterT::ValueType>(std::move(result.value())));
    }

    return ConvertResult(std::move(result));
  }

  return {};
}

/*
"Attribute Matrix Name": "AttributeMatrix",
"Data Array Name": "DataArray",
"Data Container Name": "DataContainer"
*/

inline constexpr StringLiteral k_DataContainerNameKey = "Data Container Name";
inline constexpr StringLiteral k_AttributeMatrixNameKey = "Attribute Matrix Name";
inline constexpr StringLiteral k_DataArrayNameKey = "Data Array Name";

inline Result<std::string> ReadDataContainerName(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.contains(k_DataContainerNameKey))
  {
    return MakeErrorResult<std::string>(-1, fmt::format("{} json '{}' does not contain '{}'", parameterName, json.dump(), k_DataContainerNameKey));
  }

  const auto& dataContainerNameJson = json[k_DataContainerNameKey];

  if(!dataContainerNameJson.is_string())
  {
    return MakeErrorResult<std::string>(-2, fmt::format("{} '{}' value '{}' is not a string", parameterName, k_DataContainerNameKey, dataContainerNameJson.dump()));
  }

  auto dataContainerName = dataContainerNameJson.get<std::string>();

  return {std::move(dataContainerName)};
}

inline Result<std::string> ReadAttributeMatrixName(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.contains(k_AttributeMatrixNameKey))
  {
    return MakeErrorResult<std::string>(-3, fmt::format("{} json '{}' does not contain '{}'", parameterName, json.dump(), k_AttributeMatrixNameKey));
  }

  const auto& attributeMatrixNameJson = json[k_AttributeMatrixNameKey];

  if(!attributeMatrixNameJson.is_string())
  {
    return MakeErrorResult<std::string>(-4, fmt::format("{} '{}' value '{}' is not a string", parameterName, k_AttributeMatrixNameKey, attributeMatrixNameJson.dump()));
  }

  auto attributeMatrixName = attributeMatrixNameJson.get<std::string>();

  return {std::move(attributeMatrixName)};
}

inline Result<std::string> ReadDataArrayName(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.contains(k_DataArrayNameKey))
  {
    return MakeErrorResult<std::string>(-5, fmt::format("{} json '{}' does not contain '{}'", parameterName, json.dump(), k_DataArrayNameKey));
  }

  const auto& dataArrayNameJson = json[k_DataArrayNameKey];

  if(!dataArrayNameJson.is_string())
  {
    return MakeErrorResult<std::string>(-6, fmt::format("{} '{}' value '{}' is not a string", parameterName, k_DataArrayNameKey, dataArrayNameJson.dump()));
  }

  auto dataArrayName = dataArrayNameJson.get<std::string>();

  return {std::move(dataArrayName)};
}

inline Result<ChoicesParameter::ValueType> ConvertChoicesParameter(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.is_number_integer())
  {
    return MakeErrorResult<ChoicesParameter::ValueType>(-1, fmt::format("{} json '{}' is not an integer", parameterName, json.dump()));
  }

  auto value = json.get<int32>();

  if(value < 0)
  {
    return MakeErrorResult<ChoicesParameter::ValueType>(-1, fmt::format("{} value '{}' is negative", parameterName, value));
  }

  return {static_cast<ChoicesParameter::ValueType>(value)};
}

struct ScalarTypeParameterConverter
{
  using ParameterType = DataTypeParameter;
  using ValueType = ParameterType::ValueType;

  /*
    enum class Type : int32_t
    {
      Int8 = 0,
      UInt8,
      Int16,
      UInt16,
      Int32,
      UInt32,
      Int64,
      UInt64,
      Float,
      Double,
      Bool,
      SizeT
    };
  */

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("ScalarTypeParameter json '{}' is not an integer", json.dump()));
    }

    auto value = json.get<int32>();

    switch(value)
    {
    case 0: {
      return {DataType::int8};
    }
    case 1: {
      return {DataType::uint8};
    }
    case 2: {
      return {DataType::int16};
    }
    case 3: {
      return {DataType::uint16};
    }
    case 4: {
      return {DataType::int32};
    }
    case 5: {
      return {DataType::uint32};
    }
    case 6: {
      return {DataType::int64};
    }
    case 7: {
      return {DataType::uint64};
    }
    case 8: {
      return {DataType::float32};
    }
    case 9: {
      return {DataType::float64};
    }
    case 10: {
      return {DataType::boolean};
    }
    case 11: {
      return {DataType::uint64};
    }
    }

    return MakeErrorResult<ValueType>(-2, fmt::format("Unknown ScalarTypeParameter value '{}'", value));
  }
};

struct ScalarTypeParameterToNumericTypeConverter
{
  using ParameterType = NumericTypeParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataTypeResult = SIMPLConversion::ScalarTypeParameterConverter::convert(json);
    if(dataTypeResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataTypeResult));
    }

    DataType dataType = dataTypeResult.value();

    std::optional<NumericType> numericType = ConvertDataTypeToNumericType(dataType);

    if(!numericType.has_value())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("ScalarTypeParameter unable to convert DataType '{}' to NumericType", DataTypeToString(dataType)));
    }

    return {*numericType};
  }
};

template <class T>
struct IntFilterParameterConverter
{
  using ParameterType = NumberParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("IntFilterParameter json '{}' is not an integer", json.dump()));
    }

    if constexpr(std::is_unsigned_v<T>)
    {
      if(!json.is_number_unsigned())
      {
        return MakeErrorResult<ValueType>(-1, fmt::format("IntFilterParameter json '{}' is not unsigned", json.dump()));
      }
    }

    auto value = json.get<T>();

    return {value};
  }
};

struct LinkedChoicesFilterParameterConverter
{
  using ParameterType = ChoicesParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    return ConvertChoicesParameter(json, "LinkedChoicesFilterParameter");
  }
};

struct StringFilterParameterConverter
{
  using ParameterType = StringParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("StringFilterParameter json '{}' is not a string", json.dump()));
    }

    auto value = json.get<std::string>();

    return {std::move(value)};
  }
};

struct RangeFilterParameterConverter
{
  using ParameterType = VectorParameter<float64>;
  using ValueType = ParameterType::ValueType;

  static inline constexpr StringLiteral k_MaxKey = "Max";
  static inline constexpr StringLiteral k_MinKey = "Min";

  /*
  "Max": 0,
  "Min": 0
  */

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.contains(k_MaxKey))
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("RangeFilterParameter json '{}' does not contain '{}'", json.dump(), k_MaxKey));
    }

    const auto& maxJson = json[k_MaxKey];

    if(!maxJson.is_number())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("RangeFilterParameter '{}' value '{}' is not a number", k_MaxKey, maxJson.dump()));
    }

    auto maxValue = maxJson.get<float64>();

    if(!json.contains(k_MinKey))
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("RangeFilterParameter json '{}' does not contain '{}'", json.dump(), k_MinKey));
    }

    const auto& minJson = json[k_MinKey];

    if(!minJson.is_number())
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("RangeFilterParameter '{}' value '{}' is not a number", k_MinKey, minJson.dump()));
    }

    auto minValue = minJson.get<float64>();

    std::vector<float64> value;
    value.push_back(minValue);
    value.push_back(maxValue);

    return {std::move(value)};
  }
};

struct DataArrayCreationFilterParameterConverter
{
  using ParameterType = ArrayCreationParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "DataArrayCreationFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "DataArrayCreationFilterParameter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }

    auto dataArrayNameResult = ReadDataArrayName(json, "DataArrayCreationFilterParameter");
    if(dataArrayNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value()), std::move(dataArrayNameResult.value())});

    return {std::move(dataPath)};
  }
};

struct DataContainerCreationFilterParameterConverter
{
  using ParameterType = DataGroupCreationParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "DataContainerCreationFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value())});

    return {std::move(dataPath)};
  }
};

struct ChoiceFilterParameterConverter
{
  using ParameterType = ChoicesParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    return ConvertChoicesParameter(json, "ChoiceFilterParameter");
  }
};

struct DynamicTableFilterParameterConverter
{
  using ParameterType = DynamicTableParameter;
  using ValueType = ParameterType::ValueType;

  static inline constexpr StringLiteral k_TableDataKey = "Table Data";

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.contains(k_TableDataKey))
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("DynamicTableFilterParameter json '{}' does not contain '{}'", json.dump(), k_TableDataKey));
    }

    const auto& tableDataJson = json[k_TableDataKey];

    if(!tableDataJson.is_array())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("DynamicTableFilterParameter '{}' value '{}' is not an array", k_TableDataKey, json.dump()));
    }

    DynamicTableInfo::TableDataType table;

    for(usize i = 0; i < tableDataJson.size(); i++)
    {
      const auto& jsonValue = tableDataJson.at(i);

      if(!jsonValue.is_array())
      {
        return MakeErrorResult<ValueType>(-3, fmt::format("DynamicTableFilterParameter '{}' index {} with value '{}' is not an array", k_TableDataKey, i, jsonValue.dump()));
      }

      DynamicTableInfo::RowType row;

      for(usize j = 0; j < jsonValue.size(); j++)
      {
        const auto& elementValue = jsonValue.at(j);

        if(!elementValue.is_number())
        {
          return MakeErrorResult<ValueType>(-4, fmt::format("DynamicTableFilterParameter '{}' index ({}, {}) with value '{}' is not a number", k_TableDataKey, i, j, jsonValue.dump()));
        }

        auto value = elementValue.get<float64>();
        row.push_back(value);
      }

      table.push_back(row);
    }

    return {std::move(table)};
  }
};

struct AttributeMatrixCreationFilterParameterConverter
{
  using ParameterType = DataGroupCreationParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "AttributeMatrixCreationFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "AttributeMatrixCreationFilterParameter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});

    return {std::move(dataPath)};
  }
};
} // namespace complex::SIMPLConversion
