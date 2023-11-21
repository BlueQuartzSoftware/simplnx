#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Filter/Arguments.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/CalculatorParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/DataTypeParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/EnsembleInfoParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GenerateColorTableParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/MultiPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/ReadCSVFileParameter.hpp"
#include "complex/Parameters/ReadHDF5DatasetParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

// #include "OrientationAnalysis/Parameters/H5EbsdReaderParameter.h"

#include <nlohmann/json.hpp>

#include <string_view>

namespace complex::SIMPLConversion
{
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
template <class ConverterT>
Result<> Convert2Parameters(Arguments& args, const nlohmann::json& json, std::string_view simplKey1, std::string_view simplKey2, const std::string& complexKey)
{
  if(json.contains(simplKey1) && json.contains(simplKey2))
  {
    auto result = ConverterT::convert(json[simplKey1], json[simplKey2]);
    if(result.valid())
    {
      args.insertOrAssign(complexKey, std::make_any<typename ConverterT::ValueType>(std::move(result.value())));
    }

    return ConvertResult(std::move(result));
  }

  return {};
}

//------------------------------------------------------------------------------
template <class ConverterT>
Result<> Convert3Parameters(Arguments& args, const nlohmann::json& json, std::string_view simplKey1, std::string_view simplKey2, std::string_view simplKey3, const std::string& complexKey)
{
  if(json.contains(simplKey1) && json.contains(simplKey2) && json.contains(simplKey3))
  {
    auto result = ConverterT::convert(json[simplKey1], json[simplKey2], json[simplKey3]);
    if(result.valid())
    {
      args.insertOrAssign(complexKey, std::make_any<typename ConverterT::ValueType>(std::move(result.value())));
    }

    return ConvertResult(std::move(result));
  }

  return {};
}

//------------------------------------------------------------------------------
template <class ConverterT>
Result<> ConvertTopParameters(Arguments& args, const nlohmann::json& json, const std::string& complexKey)
{
  {
    auto result = ConverterT::convert(json);
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
inline constexpr StringLiteral k_LinkedDataArrayNameKey = "OutputArrayName";

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
inline Result<FileSystemPathParameter::ValueType> ReadInputFilePath(const nlohmann::json& json, std::string_view parameterName)
{
  using ParameterType = FileSystemPathParameter;

  if(!json.is_string())
  {
    return MakeErrorResult<ParameterType::ValueType>(-1, fmt::format("{} json '{}' is not a string", parameterName, json.dump()));
  }

  auto value = json.get<std::string>();

  return {ParameterType::ValueType(value)};
}

struct NumericTypeParameterConverter
{
  using ParameterType = NumericTypeParameter;
  using ValueType = ParameterType::ValueType;

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
      return {NumericType::int8};
    }
    case 1: {
      return {NumericType::uint8};
    }
    case 2: {
      return {NumericType::int16};
    }
    case 3: {
      return {NumericType::uint16};
    }
    case 4: {
      return {NumericType::int32};
    }
    case 5: {
      return {NumericType::uint32};
    }
    case 6: {
      return {NumericType::int64};
    }
    case 7: {
      return {NumericType::uint64};
    }
    case 8: {
      return {NumericType::float32};
    }
    case 9: {
      return {NumericType::float64};
    }
    case 10: {
      return {NumericType::uint64};
    }
    }

    return MakeErrorResult<ValueType>(-2, fmt::format("Unknown NumericTypeParameter value '{}'", value));
  }
};

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

struct NumericTypeFilterParameterConverter
{
  using ParameterType = NumericTypeParameter;
  using ValueType = ParameterType::ValueType;

  /**
  {
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,
    float32,
    float64
  };
  */

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("NumericTypeParameter json '{}' is not an integer", json.dump()));
    }

    auto value = json.get<int32>();

    switch(value)
    {
    case 0: {
      return {NumericType::int8};
    }
    case 1: {
      return {NumericType::uint8};
    }
    case 2: {
      return {NumericType::int16};
    }
    case 3: {
      return {NumericType::uint16};
    }
    case 4: {
      return {NumericType::int32};
    }
    case 5: {
      return {NumericType::uint32};
    }
    case 6: {
      return {NumericType::int64};
    }
    case 7: {
      return {NumericType::uint64};
    }
    case 8: {
      return {NumericType::float32};
    }
    case 9: {
      return {NumericType::float64};
    }
    case 10: {
      return {NumericType::uint64};
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

template <typename T>
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

using UInt64FilterParameterConverter = IntFilterParameterConverter<uint64>;

template <typename T>
struct StringToIntFilterParameterConverter
{
  using ParameterType = NumberParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("StringToIntFilterParameterConverter json '{}' is not a string", json.dump()));
    }

    auto value = static_cast<T>(std::stoi(json.get<std::string>()));
    return {value};
  }
};

template <class T = float32>
struct FloatFilterParameterConverter
{
  using ParameterType = NumberParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("FloatFilterParameter json '{}' is not a number", json.dump()));
    }

    auto value = json.get<T>();

    return {value};
  }
};

using DoubleFilterParameterConverter = FloatFilterParameterConverter<float64>;

struct BooleanFilterParameterConverter
{
  using ParameterType = BoolParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number_integer() || !json.is_number_unsigned())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("BooleanFilterParameter json '{}' is not a boolean", json.dump()));
    }

    auto value = static_cast<bool>(json.get<uint8>());

    return {std::move(value)};
  }
};

struct InvertedBooleanFilterParameterConverter
{
  using ParameterType = BoolParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number_integer() || !json.is_number_unsigned())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("BooleanFilterParameter json '{}' is not a boolean", json.dump()));
    }

    auto value = !static_cast<bool>(json.get<uint8>());

    return {std::move(value)};
  }
};

struct LinkedBooleanFilterParameterConverter
{
  using ParameterType = BoolParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("LinkedBooleanFilterParameter json '{}' is not an integer", json.dump()));
    }

    auto value = static_cast<bool>(json.get<int32>());

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

template <typename T>
struct NumberToStringFilterParameterConverter
{
  using ParameterType = StringParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_number())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("NumberToStringFilterParameterConverter json '{}' is not a number", json.dump()));
    }

    auto value = json.get<T>();

    return {std::to_string(value)};
  }
};

using DoubleToStringFilterParameterConverter = NumberToStringFilterParameterConverter<float64>;

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

struct LinkedPathCreationFilterParameterConverter
{
  using ParameterType = DataObjectNameParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("LinkedPathCreationFilterParameter json '{}' is not a string", json.dump()));
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

template <typename T>
struct MultiToVec3FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3)
  {
    static const std::string x = "x";
    static const std::string y = "y";
    static const std::string z = "z";

    if(!json1.is_number())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("IntVec3FilterParameter json '{}' is not a number", json1.dump()));
    }
    if(!json2.is_number())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("IntVec3FilterParameter json '{}' is not a number", json2.dump()));
    }
    if(!json3.is_number())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("IntVec3FilterParameter json '{}' is not a number", json3.dump()));
    }

    std::vector<T> value(3);
    value[0] = json1.get<T>();
    value[1] = json2.get<T>();
    value[2] = json3.get<T>();

    return {std::move(value)};
  }
};

using UInt64ToVec3FilterParameterConverter = MultiToVec3FilterParameterConverter<uint64>;
using FloatToVec3FilterParameterConverter = MultiToVec3FilterParameterConverter<float32>;
using DoubleToVec3FilterParameterConverter = MultiToVec3FilterParameterConverter<float64>;

template <typename T>
struct Vec3FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    static const std::string x = "x";
    static const std::string y = "y";
    static const std::string z = "z";

    if(!json.is_object())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("IntVec3FilterParameter json '{}' is not an object", json.dump()));
    }

    if(!json.contains(x))
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("IntVec3FilterParameter json '{}' does not contain an X value", json.dump()));
    }
    if(!json.contains(y))
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("IntVec3FilterParameter json '{}' does not contain a Y value", json.dump()));
    }
    if(!json.contains(z))
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("IntVec3FilterParameter json '{}' does not contain an Z value", json.dump()));
    }

    std::vector<T> value(3);
    value[0] = json[x].get<T>();
    value[1] = json[y].get<T>();
    value[2] = json[z].get<T>();

    return {std::move(value)};
  }
};

using IntVec3FilterParameterConverter = Vec3FilterParameterConverter<int32>;
using UInt32Vec3FilterParameterConverter = Vec3FilterParameterConverter<uint32>;
using UInt64Vec3FilterParameterConverter = Vec3FilterParameterConverter<uint64>;
using FloatVec3FilterParameterConverter = Vec3FilterParameterConverter<float32>;
using DoubleVec3FilterParameterConverter = Vec3FilterParameterConverter<float64>;

template <typename T>
struct Vec4FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    static const std::string x = "x";
    static const std::string y = "y";
    static const std::string z = "z";
    static const std::string w = "w";

    if(!json.is_object())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("Vec4FilterParameter json '{}' is not an object", json.dump()));
    }

    if(!json.contains(x))
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("Vec4FilterParameter json '{}' does not contain an X value", json.dump()));
    }
    if(!json.contains(y))
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("Vec4FilterParameter json '{}' does not contain a Y value", json.dump()));
    }
    if(!json.contains(z))
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("Vec4FilterParameter json '{}' does not contain an Z value", json.dump()));
    }
    if(!json.contains(w))
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("Vec4FilterParameter json '{}' does not contain an W value", json.dump()));
    }

    std::vector<T> value(4);
    value[0] = json[x].get<T>();
    value[1] = json[y].get<T>();
    value[2] = json[z].get<T>();
    value[3] = json[w].get<T>();

    return {std::move(value)};
  }
};

using FloatVec4FilterParameterConverter = Vec4FilterParameterConverter<float32>;

template <typename T>
struct AxisAngleFilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    static const std::string angle = "angle";
    static const std::string h = "h";
    static const std::string k = "k";
    static const std::string l = "l";

    if(!json.is_object())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("AxisAngleFilterParameterConverter json '{}' is not an object", json.dump()));
    }

    if(!json.contains(angle))
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("AxisAngleFilterParameterConverter json '{}' does not contain an X value", json.dump()));
    }
    if(!json.contains(h))
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("AxisAngleFilterParameterConverter json '{}' does not contain a Y value", json.dump()));
    }
    if(!json.contains(k))
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("AxisAngleFilterParameterConverter json '{}' does not contain an Z value", json.dump()));
    }
    if(!json.contains(l))
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("AxisAngleFilterParameterConverter json '{}' does not contain an W value", json.dump()));
    }

    std::vector<T> value(4);
    value[0] = json[angle].get<T>();
    value[1] = json[h].get<T>();
    value[2] = json[k].get<T>();
    value[3] = json[l].get<T>();

    return {std::move(value)};
  }
};

template <typename T>
struct Vec3p1FilterParameterConverter
{
  using ParameterType = VectorParameter<T>;
  using ValueType = typename ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2)
  {
    static const std::string x = "x";
    static const std::string y = "y";
    static const std::string z = "z";

    if(!json1.is_object())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("Vec3+1FilterParameter json '{}' is not an object", json1.dump()));
    }

    if(!json1.contains(x))
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("Vec3+1FilterParameter json '{}' does not contain an X value", json1.dump()));
    }
    if(!json1.contains(y))
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("Vec3+1FilterParameter json '{}' does not contain a Y value", json1.dump()));
    }
    if(!json1.contains(z))
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("Vec3+1FilterParameter json '{}' does not contain an Z value", json1.dump()));
    }

    if(!json2.is_number())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("Vec3+1FilterParameter json '{}' is not a number", json2.dump()));
    }

    std::vector<T> value(4);
    value[0] = json1[x].get<T>();
    value[1] = json1[y].get<T>();
    value[2] = json1[z].get<T>();
    value[3] = json2.get<T>();

    return {std::move(value)};
  }
};

using FloatVec3p1FilterParameterConverter = Vec3p1FilterParameterConverter<float32>;

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

struct DataArrayCreationToAMFilterParameterConverter
{
  using ParameterType = AttributeMatrixSelectionParameter;
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

    DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});

    return {std::move(dataPath)};
  }
};

struct DataArrayCreationToDataObjectNameFilterParameterConverter
{
  using ParameterType = DataObjectNameParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataArrayNameResult = ReadDataArrayName(json, "DataArrayCreationFilterParameter");
    if(dataArrayNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
    }

    return {std::move(dataArrayNameResult.value())};
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

struct StringToDataPathFilterParameterConverter
{
  using ParameterType = DataGroupCreationParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("StringToDataPathFilterParameterConverter json '{}' is not a string", json.dump()));
    }

    DataPath dataPath({json.get<std::string>()});

    return {std::move(dataPath)};
  }
};

struct StringsToDataPathFilterParameterConverter
{
  using ParameterType = DataGroupCreationParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2)
  {
    if(!json1.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("StringsToDataPathFilterParameterConverter json '{}' is not a string", json1.dump()));
    }
    if(!json2.is_string())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("StringsToDataPathFilterParameterConverter json '{}' is not a string", json2.dump()));
    }

    DataPath dataPath({json1.get<std::string>(), json2.get<std::string>()});

    return {std::move(dataPath)};
  }
};

struct AMPathBuilderFilterParameterConverter
{
  using ParameterType = DataGroupCreationParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2)
  {
    std::string dcName;
    std::string amName;

    if(json1.is_string())
    {
      dcName = json1.get<std::string>();
    }
    else
    {
      auto dataContainerNameResult = ReadDataContainerName(json1, "AMPathBuilderFilterParameterConverter");
      if(dataContainerNameResult.invalid())
      {
        return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
      }
      dcName = std::move(dataContainerNameResult.value());
    }

    if(json2.is_string())
    {
      amName = json2.get<std::string>();
    }
    else
    {
      auto attributeMatrixNameResult = ReadAttributeMatrixName(json2, "AMPathBuilderFilterParameterConverter");
      if(attributeMatrixNameResult.invalid())
      {
        return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
      }
      amName = std::move(attributeMatrixNameResult.value());
    }

    DataPath dataPath({dcName, amName});

    return {std::move(dataPath)};
  }
};

struct DAPathBuilderFilterParameterConverter
{
  using ParameterType = DataGroupCreationParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3)
  {
    std::string dcName;
    std::string amName;
    std::string daName;

    if(json1.is_string())
    {
      dcName = json1.get<std::string>();
    }
    else
    {
      auto dataContainerNameResult = ReadDataContainerName(json1, "DAPathBuilderFilterParameterConverter");
      if(dataContainerNameResult.invalid())
      {
        return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
      }
      dcName = std::move(dataContainerNameResult.value());
    }

    if(json2.is_string())
    {
      amName = json2.get<std::string>();
    }
    else
    {
      auto attributeMatrixNameResult = ReadAttributeMatrixName(json2, "DAPathBuilderFilterParameterConverter");
      if(attributeMatrixNameResult.invalid())
      {
        return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
      }
      amName = std::move(attributeMatrixNameResult.value());
    }

    if(json3.is_string())
    {
      daName = json3.get<std::string>();
    }
    else
    {
      auto dataArrayNameResult = ReadDataArrayName(json3, "DAPathBuilderFilterParameterConverter");
      if(dataArrayNameResult.invalid())
      {
        return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
      }
      daName = std::move(dataArrayNameResult.value());
    }

    DataPath dataPath({dcName, amName, daName});

    return {std::move(dataPath)};
  }
};

struct DataContainerNameFilterParameterConverter
{
  using ParameterType = DataObjectNameParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "DataContainerNameFilterParameterConverter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    return {std::move(dataContainerNameResult.value())};
  }
};

struct AttributeMatrixNameFilterParameterConverter
{
  using ParameterType = DataObjectNameParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto nameResult = ReadAttributeMatrixName(json, "AttributeMatrixNameFilterParameterConverter");
    if(nameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(nameResult));
    }

    return {std::move(nameResult.value())};
  }
};

struct DataArrayNameFilterParameterConverter
{
  using ParameterType = DataObjectNameParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto nameResult = ReadDataArrayName(json, "DataArrayNameFilterParameterConverter");
    if(nameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(nameResult));
    }

    return {std::move(nameResult.value())};
  }
};

struct DataContainerSelectionFilterParameterConverter
{
  using ParameterType = DataGroupSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "DataContainerSelectionFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value())});

    return {std::move(dataPath)};
  }
};

struct SingleToMultiDataPathSelectionFilterParameterConverter
{
  using ParameterType = MultiPathSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
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
};

struct MultiDataArraySelectionFilterParameterConverter
{
  using ParameterType = MultiArraySelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("MultiDataArraySelectionParameter json '{}' is not an array", json.dump()));
    }

    std::vector<DataPath> dataPaths;

    for(auto& iter : json)
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
};

struct DataContainerFromMultiSelectionFilterParameterConverter
{
  using ParameterType = DataGroupSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("DataContainerFromMultiSelectionFilterParameterConverter json '{}' is not an array", json.dump()));
    }

    for(auto& iter : json)
    {
      auto dataContainerNameResult = ReadDataContainerName(iter, "DataContainerSelectionFilterParameter");
      if(dataContainerNameResult.invalid())
      {
        return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
      }
      DataPath value({std::move(dataContainerNameResult.value())});

      return {std::move(value)};
    }

    return {DataPath()};
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

struct ArrayToDynamicTableFilterParameterConverter
{
  using ParameterType = DynamicTableParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    const auto& tableDataJson = json;
    if(!tableDataJson.is_array())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("ArrayToDynamicTableFilterParameterConverter '{}' value is not an array", json.dump()));
    }

    DynamicTableInfo::TableDataType table;
    DynamicTableInfo::RowType row;

    for(usize j = 0; j < tableDataJson.size(); j++)
    {
      const auto& elementValue = tableDataJson.at(j);

      if(!elementValue.is_number())
      {
        return MakeErrorResult<ValueType>(-4, fmt::format("ArrayToDynamicTableFilterParameterConverter index ({}, {}) with value '{}' is not a number", 0, j, tableDataJson.dump()));
      }

      auto value = elementValue.get<float64>();
      row.push_back(value);
    }

    table.push_back(row);

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

struct AttributeMatrixSelectionFilterParameterConverter
{
  using ParameterType = AttributeMatrixSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "AttributeMatrixSelectionFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "AttributeMatrixSelectionFilterParameter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});

    return {std::move(dataPath)};
  }
};

struct DataArraySelectionFilterParameterConverter
{
  using ParameterType = ArraySelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
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
};

struct DataArraySelectionToGeometrySelectionFilterParameterConverter
{
  using ParameterType = GeometrySelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "DataArraySelectionFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value())});

    return {std::move(dataPath)};
  }
};

struct InputFileFilterParameterConverter
{
  using ParameterType = FileSystemPathParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto filePathReult = ReadInputFilePath(json, "InputFileFilterParameter");
    if(filePathReult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(filePathReult));
    }

    return {filePathReult.value()};
  }
};

struct OutputFileFilterParameterConverter
{
  using ParameterType = FileSystemPathParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto filePathReult = ReadInputFilePath(json, "OutputFileFilterParameter");
    if(filePathReult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(filePathReult));
    }

    return {filePathReult.value()};
  }
};

struct FileListInfoFilterParameterConverter
{
  using ParameterType = GeneratedFileListParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_EndIndex = "EndIndex";
  static constexpr StringLiteral k_FileExtension = "FileExtension";
  static constexpr StringLiteral k_FilePrefix = "FilePrefix";
  static constexpr StringLiteral k_FileSuffix = "FileSuffix";
  static constexpr StringLiteral k_IncrementIndex = "IncrementIndex";
  static constexpr StringLiteral k_InputPath = "InputPath";
  static constexpr StringLiteral k_Ordering = "Ordering";
  static constexpr StringLiteral k_PaddingDigits = "PaddingDigits";
  static constexpr StringLiteral k_StartIndex = "StartIndex";

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.contains(k_EndIndex))
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_EndIndex));
    }

    if(!json[k_EndIndex].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an integer '{}'", json.dump(), k_EndIndex));
    }

    ValueType value;
    value.endIndex = json[k_EndIndex].get<int32>();

    {
      if(!json.contains(k_FileExtension))
      {
        return MakeErrorResult<ValueType>(-3, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_FileExtension));
      }

      if(!json[k_FileExtension].is_string())
      {
        return MakeErrorResult<ValueType>(-4, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_FileExtension));
      }

      value.fileExtension = json[k_FileExtension].get<std::string>();
    }

    {
      if(!json.contains(k_FilePrefix))
      {
        return MakeErrorResult<ValueType>(-5, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_FilePrefix));
      }

      if(!json[k_FilePrefix].is_string())
      {
        return MakeErrorResult<ValueType>(-6, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_FilePrefix));
      }

      value.filePrefix = json[k_FilePrefix].get<std::string>();
    }

    {
      if(!json.contains(k_FileSuffix))
      {
        return MakeErrorResult<ValueType>(-7, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_FileSuffix));
      }

      if(!json[k_FileSuffix].is_string())
      {
        return MakeErrorResult<ValueType>(-8, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_FileSuffix));
      }

      value.fileSuffix = json[k_FileSuffix].get<std::string>();
    }

    {
      if(!json.contains(k_IncrementIndex))
      {
        return MakeErrorResult<ValueType>(-9, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_IncrementIndex));
      }

      if(!json[k_IncrementIndex].is_number_integer())
      {
        return MakeErrorResult<ValueType>(-10, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an integer '{}'", json.dump(), k_IncrementIndex));
      }

      value.incrementIndex = json[k_IncrementIndex].get<int32>();
    }

    {
      if(!json.contains(k_InputPath))
      {
        return MakeErrorResult<ValueType>(-11, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_InputPath));
      }

      if(!json[k_InputPath].is_string())
      {
        return MakeErrorResult<ValueType>(-12, fmt::format("FileListInfoFilterParameterConverter json '{}' is not a string '{}'", json.dump(), k_InputPath));
      }

      value.inputPath = json[k_InputPath].get<std::string>();
    }

    {
      if(!json.contains(k_Ordering))
      {
        return MakeErrorResult<ValueType>(-13, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_Ordering));
      }

      if(!json[k_Ordering].is_number_unsigned())
      {
        return MakeErrorResult<ValueType>(-14, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an unsigned integer '{}'", json.dump(), k_Ordering));
      }

      value.ordering = static_cast<ParameterType::Ordering>(json[k_Ordering].get<uint8>());
    }

    {
      if(!json.contains(k_PaddingDigits))
      {
        return MakeErrorResult<ValueType>(-15, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_PaddingDigits));
      }

      if(!json[k_PaddingDigits].is_number_unsigned())
      {
        return MakeErrorResult<ValueType>(-16, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an unsigned integer '{}'", json.dump(), k_PaddingDigits));
      }

      value.paddingDigits = json[k_PaddingDigits].get<uint32>();
    }

    {
      if(!json.contains(k_StartIndex))
      {
        return MakeErrorResult<ValueType>(-17, fmt::format("FileListInfoFilterParameterConverter json '{}' does not contain '{}'", json.dump(), k_StartIndex));
      }

      if(!json[k_StartIndex].is_number_integer())
      {
        return MakeErrorResult<ValueType>(-18, fmt::format("FileListInfoFilterParameterConverter json '{}' is not an integer '{}'", json.dump(), k_StartIndex));
      }

      value.startIndex = json[k_StartIndex].get<int32>();
    }

    return {std::move(value)};
  }
};

struct EnsembleInfoFilterParameterConverter
{
  using ParameterType = EnsembleInfoParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_CrystalStructureKey = "CrystalStructure";
  static constexpr StringLiteral k_PhaseNameKey = "PhaseName";
  static constexpr StringLiteral k_PhaseTypeKey = "PhaseType";

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an array", json.dump()));
    }

    ValueType value;

    for(const auto& iter : json)
    {
      if(!iter.is_object())
      {
        return MakeErrorResult<ValueType>(-2, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an object", iter.dump()));
      }

      if(!iter[k_CrystalStructureKey].is_number_integer())
      {
        return MakeErrorResult<ValueType>(-3, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an integer '{}'", iter.dump(), iter[k_CrystalStructureKey].dump()));
      }
      if(!iter[k_PhaseNameKey].is_string())
      {
        return MakeErrorResult<ValueType>(-4, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not a string '{}'", iter.dump(), iter[k_PhaseNameKey].dump()));
      }
      if(!iter[k_PhaseTypeKey].is_number_integer())
      {
        return MakeErrorResult<ValueType>(-5, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an integer '{}'", iter.dump(), iter[k_PhaseTypeKey].dump()));
      }

      ParameterType::RowType row;
      row[0] = ParameterType::k_CrystalStructures[iter[k_CrystalStructureKey].get<int32>()];
      row[1] = ParameterType::k_PhaseTypes[iter[k_PhaseTypeKey].get<int32>()];
      row[2] = iter[k_PhaseNameKey].get<std::string>();

      value.push_back(std::move(row));
    }

    return {std::move(value)};
  }
};

struct GenerateColorTableFilterParameterConverter
{
  using ParameterType = GenerateColorTableParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_SelectedPresetKey = "SelectedPresetName";
  static constexpr StringLiteral k_PresetNameKey = "Name";
  static constexpr StringLiteral k_RGBPointsKey = "RGBPoints";

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2)
  {
    if(!json1.is_string())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("GenerateColorTableFilterParameterConverter json1 '{}' is not a string", json2.dump()));
    }
    if(!json2.is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("GenerateColorTableFilterParameterConverter json2 '{}' is not an array", json1.dump()));
    }

    auto presetName = json1.get<std::string>();
    auto rgbPoints = json2.get<std::vector<float64>>();

    nlohmann::json value = nlohmann::json::object();
    value[k_PresetNameKey] = presetName;
    value[k_RGBPointsKey] = rgbPoints;

    return {std::move(value)};
  }
};

struct ReadASCIIWizardDataFilterParameterConverter
{
  using ParameterType = ReadCSVFileParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_InputFilePathKey = "Wizard_InputFilePath";
  static constexpr StringLiteral k_DataHeadersKey = "Wizard_DataHeaders";
  static constexpr StringLiteral k_BeginIndexKey = "Wizard_BeginIndex";
  static constexpr StringLiteral k_NumberOfLinesKey = "Wizard_NumberOfLines";
  static constexpr StringLiteral k_DataTypesKey = "Wizard_DataTypes";
  static constexpr StringLiteral k_DelimitersKey = "Wizard_Delimiters";
  static constexpr StringLiteral k_HeaderLineKey = "Wizard_HeaderLine";
  static constexpr StringLiteral k_HeaderIsCustomKey = "Wizard_HeaderIsCustom";
  static constexpr StringLiteral k_HeaderUsesDefaultsKey = "Wizard_HeaderUseDefaults";
  static constexpr StringLiteral k_ConsecutiveDelimitersKey = "Wizard_ConsecutiveDelimiters";

  static std::vector<DataType> ConvertDataTypeStrings(const std::vector<std::string>& dataTypes)
  {
    std::vector<DataType> output;

    for(usize i = 0; i < dataTypes.size(); i++)
    {
      try
      {
        output.push_back(complex::StringToDataType(dataTypes[i]));
      } catch(const std::exception& e)
      {
      }
    }

    return output;
  }

  static std::vector<char> ConvertToChars(const std::string& string)
  {
    std::vector<char> chars(string.length());
    for(usize i = 0; i < string.size(); i++)
    {
      chars[i] = string[i];
    }
    return chars;
  }

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    std::vector<std::string> dataTypeStrings = json[k_DataTypesKey].get<std::vector<std::string>>();
    for(auto& dataTypeStr : dataTypeStrings)
    {
      if(dataTypeStr == "double")
      {
        dataTypeStr = "float64";
      }
      else if(dataTypeStr == "float")
      {
        dataTypeStr = "float32";
      }
    }

    ValueType value;
    value.inputFilePath = json[k_InputFilePathKey].get<std::string>();
    value.customHeaders = json[k_DataHeadersKey].get<std::vector<std::string>>();
    value.startImportRow = json[k_BeginIndexKey].get<int32>();
    value.dataTypes = ConvertDataTypeStrings(dataTypeStrings);
    value.delimiters = ConvertToChars(json[k_DelimitersKey].get<std::string>());
    value.headersLine = json[k_HeaderLineKey].get<int32>();

    bool headerIsCustom = json[k_HeaderIsCustomKey].get<bool>();
    bool headerUsesDefaults = json[k_HeaderUsesDefaultsKey].get<bool>();

    if(headerIsCustom)
    {
      value.headerMode = ValueType::HeaderMode::CUSTOM;
    }
    else
    {
      value.headerMode = ValueType::HeaderMode::LINE;
    }

    value.consecutiveDelimiters = static_cast<bool>(json[k_ConsecutiveDelimitersKey].get<int32>());

    return {std::move(value)};
  }
};

struct ImportHDF5DatasetFilterParameterConverter
{
  using ParameterType = ReadHDF5DatasetParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_DatasetPathKey = "dataset_path";
  static constexpr StringLiteral k_ComponentDimensionsKey = "component_dimensions";
  static constexpr StringLiteral k_TupleDimensionsKey = "tuple_dimensions";

  static Result<ValueType> convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3)
  {
    if(!json1.is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("ReadHDF5DatasetParameter json '{}' is not an array", json1.dump()));
    }

    if(!json2.is_string())
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("ImportHDF5DatasetFilterParameter json '{}' is not a string", json2.dump()));
    }

    auto dataContainerNameResult = ReadDataContainerName(json3, "ImportHDF5DatasetFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    auto attributeMatrixNameResult = ReadAttributeMatrixName(json3, "ImportHDF5DatasetFilterParameter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }

    ValueType value;
    value.inputFile = json2.get<std::string>();
    value.parent = DataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});

    for(const auto& iter : json1)
    {
      if(!iter.is_object())
      {
        return MakeErrorResult<ValueType>(-2, fmt::format("ImportHDF5DatasetFilterParameter json '{}' is not an object", iter.dump()));
      }

      ParameterType::DatasetImportInfo info;
      info.dataSetPath = iter[k_DatasetPathKey].get<std::string>();
      info.componentDimensions = iter[k_ComponentDimensionsKey].get<std::string>();
      value.datasets.push_back(std::move(info));
    }

    return {std::move(value)};
  }
};

struct DataArraysToRemoveConverter
{
  using ParameterType = MultiPathSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_DataContainersKey = "Data Containers";
  static constexpr StringLiteral k_AttributeMatricesKey = "Attribute Matricies";
  static constexpr StringLiteral k_DataArraysKey = "Data Arrays";
  static constexpr StringLiteral k_NameKey = "Name";
  static constexpr StringLiteral k_FlagKey = "Flag";

  static Result<std::string> FindName(const nlohmann::json& json)
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

  static bool IsFlagged(const nlohmann::json& json)
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

  static Result<ValueType> convert(const nlohmann::json& json)
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
};

struct CalculatorFilterParameterConverter
{
  using ParameterType = CalculatorParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_InfixEquationKey = "InfixEquation";
  static constexpr StringLiteral k_UnitsKey = "Units";
  static constexpr StringLiteral k_SelectedAttributeMatrixKey = "SelectedAttributeMatrix";

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json.contains(k_InfixEquationKey))
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json '{}' does not contain '{}'", json.dump(), k_InfixEquationKey));
    }
    if(!json.contains(k_UnitsKey))
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json '{}' does not contain '{}'", json.dump(), k_UnitsKey));
    }
    if(!json.contains(k_SelectedAttributeMatrixKey))
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json '{}' does not contain '{}'", json.dump(), k_SelectedAttributeMatrixKey));
    }

    if(!json[k_InfixEquationKey].is_string())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json[InfixEquation] '{}' is not a string", json[k_InfixEquationKey].dump()));
    }
    if(!json[k_UnitsKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("CalculatorParameter json[ScalarType] '{}' is not an integer", json[k_UnitsKey].dump()));
    }
    if(!json[k_SelectedAttributeMatrixKey].is_object())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("CalculatorParameter json[SelectedAttributeMatrix] '{}' is not an object", json[k_SelectedAttributeMatrixKey].dump()));
    }

    const auto& amJson = json[k_SelectedAttributeMatrixKey];
    auto dataContainerNameResult = ReadDataContainerName(amJson, "AttributeMatrixCreationFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    auto attributeMatrixNameResult = ReadAttributeMatrixName(amJson, "AttributeMatrixCreationFilterParameter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});

    ValueType value;
    value.m_Equation = json[k_InfixEquationKey].get<std::string>();
    value.m_Units = static_cast<ParameterType::AngleUnits>(json[k_UnitsKey].get<int32>());
    value.m_SelectedGroup = dataPath;

    return {std::move(value)};
  }
};

#if 0
struct ReadH5EbsdFilterParameterConverter
{
  using ParameterType = H5EbsdReaderParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_InputFileKey = "InputFile";
  static constexpr StringLiteral k_SelectedArrayNamesKey = "SelectedArrayNames";
  static constexpr StringLiteral k_ZStartIndexKey = "ZStartIndex";
  static constexpr StringLiteral k_ZEndIndexKey = "ZEndIndex";
  static constexpr StringLiteral k_RefFrameZDirKey = "RefFrameZDir";
  static constexpr StringLiteral k_UseTransformationsKey = "UseTransformations";

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json[k_InputFileKey].is_string())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("H5EbsdReaderParameterConverter json '{}' is not a string", json[k_InputFileKey].dump()));
    }
    if(!json[k_SelectedArrayNamesKey].is_array())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an array", json[k_SelectedArrayNamesKey].dump()));
    }
    if(!json[k_ZStartIndexKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_ZStartIndexKey].dump()));
    }
    if(!json[k_ZEndIndexKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_ZEndIndexKey].dump()));
    }
    if(!json[k_RefFrameZDirKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_RefFrameZDirKey].dump()));
    }
    if(!json[k_UseTransformationsKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an integer", json[k_UseTransformationsKey].dump()));
    }

    for(const auto& iter : json[k_SelectedArrayNamesKey])
    {
      if(!iter.is_string())
      {
        return MakeErrorResult<ValueType>(-2, fmt::format("H5EbsdReaderParameterConverter array name json '{}' is not a string", iter.dump()));
      }
    }

    ParameterType::ValueType value;
    value.inputFilePath = json[k_InputFileKey].get<std::string>();
    value.startSlice = json[k_ZStartIndexKey].get<int32>();
    value.endSlice = json[k_ZEndIndexKey].get<int32>();
    value.eulerRepresentation = json[k_RefFrameZDirKey].get<int32>();
    value.useRecommendedTransform = static_cast<bool>(json[k_UseTransformationsKey].get<int32>());
    value.hdf5DataPaths = json[k_SelectedArrayNamesKey].get<std::vector<std::string>>();

    return {std::move(value)};
  }
};
#endif

struct DataContainerReaderFilterParameterConverter
{
  using ParameterType = Dream3dImportParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_InputFileKey = "InputFile";
  static constexpr StringLiteral k_InputFileDataContainerArrayProxyKey = "InputFileDataContainerArrayProxy";
  static constexpr StringLiteral k_DataContainerProxyKey = "Data Containers";
  static constexpr StringLiteral k_AttributeMatrixProxyKey = "Attribute Matricies";
  static constexpr StringLiteral k_DataArrayProxyKey = "Data Arrays";
  static constexpr StringLiteral k_DataObjectNameKey = "Name";

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    if(!json[k_InputFileKey].is_string())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("H5EbsdReaderParameterConverter json '{}' is not a string", json[k_InputFileKey].dump()));
    }
    if(!json[k_InputFileDataContainerArrayProxyKey].is_object())
    {
      return MakeErrorResult<ValueType>(-1, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an object", json[k_InputFileDataContainerArrayProxyKey].dump()));
    }

    std::string inputFilePath = json[k_InputFileKey].get<std::string>();
    std::vector<complex::DataPath> dataPaths;

    auto dcaProxyJson = json[k_InputFileDataContainerArrayProxyKey];
    if(!dcaProxyJson[k_DataContainerProxyKey].is_array())
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("H5EbsdReaderParameterConverter json '{}' is not an array", dcaProxyJson[k_DataContainerProxyKey].dump()));
    }

    for(const auto& dcIter : dcaProxyJson[k_DataContainerProxyKey])
    {
      std::string dcName = dcIter[k_DataObjectNameKey].get<std::string>();
      DataPath dcPath({dcName});
      dataPaths.push_back(dcPath);

      for(const auto& amIter : dcIter[k_AttributeMatrixProxyKey])
      {
        std::string amName = amIter[k_DataObjectNameKey].get<std::string>();
        DataPath amPath = dcPath.createChildPath(amName);
        dataPaths.push_back(amPath);

        for(const auto& daIter : amIter[k_DataArrayProxyKey])
        {
          std::string daName = daIter[k_DataObjectNameKey].get<std::string>();
          DataPath daPath = amPath.createChildPath(daName);
          dataPaths.push_back(daPath);
        }
      }
    }

    ParameterType::ValueType value;
    value.FilePath = std::filesystem::path(inputFilePath);
    value.DataPaths = dataPaths;

    return {std::move(value)};
  }
};

struct ComparisonSelectionFilterParameterConverter
{
  using ParameterType = ArrayThresholdsParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_DataArrayNameKey = "Attribute Array Name";
  static constexpr StringLiteral k_AttributeMatrixNameKey = "Attribute Matrix Name";
  static constexpr StringLiteral k_DataContainerNameKey = "Data Container Name";
  static constexpr StringLiteral k_ComparisonValueKey = "Comparison Value";
  static constexpr StringLiteral k_ComparisonOperatorKey = "Comparison Operator";

  static Result<ValueType> convert(const nlohmann::json& json)
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
};

struct ComparisonSelectionAdvancedFilterParameterConverter
{
  using ParameterType = ArrayThresholdsParameter;
  using ValueType = ParameterType::ValueType;

  static constexpr StringLiteral k_DataArrayNameKey = "Attribute Array Name";
  static constexpr StringLiteral k_AttributeMatrixNameKey = "Attribute Matrix Name";
  static constexpr StringLiteral k_DataContainerNameKey = "Data Container Name";
  static constexpr StringLiteral k_ComparisonValueKey = "Comparison Value";
  static constexpr StringLiteral k_ComparisonOperatorKey = "Comparison Operator";
  static constexpr StringLiteral k_ThresholdValuesKey = "Comparison Values";
  static constexpr StringLiteral k_UnionOperatorKey = "Union Operator";
  static constexpr StringLiteral k_ThresholdsKey = "Thresholds";
  static constexpr StringLiteral k_InvertedKey = "Invert Comparison";

  static bool isArrayThreshold(const nlohmann::json& json)
  {
    return !json.contains(k_ThresholdValuesKey);
  }

  static ArrayThreshold::ComparisonType invertComparison(ArrayThreshold::ComparisonType comparison)
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

  static std::shared_ptr<ArrayThreshold> convertArrayThreshold(const DataPath& amPath, const nlohmann::json& json)
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

  static std::vector<std::shared_ptr<ArrayThreshold>> flattenSetThreshold(const std::shared_ptr<ArrayThresholdSet>& thresholdSet, bool parentInverted = false)
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

  static std::shared_ptr<ArrayThresholdSet> convertSetThreshold(const DataPath& amPath, const nlohmann::json& json)
  {
    ParameterType::ValueType::CollectionType thresholdSet;

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

  static Result<ValueType> convert(const nlohmann::json& json)
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
};
} // namespace complex::SIMPLConversion
