#include "VectorParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Common/TypeTraits.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
template <class T>
VectorParameter<T>::VectorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue, const NamesType& names)
: VectorParameterBase(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_Names(names)
{
  if(m_DefaultValue.size() != m_Names.size())
  {
    throw std::runtime_error("VectorParameter: size of names is not equal to required size");
  }

  if(m_DefaultValue.empty())
  {
    throw std::runtime_error("VectorParameter: cannot be size 0");
  }
}

template <class T>
VectorParameter<T>::VectorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: VectorParameter(name, humanName, helpText, defaultValue, NamesType(defaultValue.size()))
{
}

template <class T>
Uuid VectorParameter<T>::uuid() const
{
  return ParameterTraits<VectorParameter<T>>::uuid;
}

template <class T>
IParameter::AcceptedTypes VectorParameter<T>::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
template <class T>
IParameter::VersionType VectorParameter<T>::getVersion() const
{
  return 1;
}

template <class T>
nlohmann::json VectorParameter<T>::toJsonImpl(const std::any& value) const
{
  const auto& vec = GetAnyRef<ValueType>(value);

  auto jsonArray = nlohmann::json::array();
  for(T element : vec)
  {
    jsonArray.push_back(element);
  }

  return jsonArray;
}

template <class T>
Result<std::any> VectorParameter<T>::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'VectorParameter' JSON Error: ";

  if(!json.is_array())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an array", prefix, name()));
  }
  ValueType vec;
  for(usize i = 0; i < json.size(); i++)
  {
    const auto& element = json[i];
    if constexpr(std::is_arithmetic_v<T>)
    {
      if(!element.is_number())
      {
        return MakeErrorResult<std::any>(-3, fmt::format("{}JSON value for array index '{}' is not a number", prefix, i));
      }
    }
    else
    {
      static_assert(dependent_false<T>, "Attempting to convert value for which std::is_arithmetic_v<T>==false. Please check the JSON to ensure the value is numeric.");
    }
    vec.push_back(element.get<T>());
  }
  return {{std::move(vec)}};
}

template <class T>
IParameter::UniquePointer VectorParameter<T>::clone() const
{
  return std::make_unique<VectorParameter<T>>(name(), humanName(), helpText(), m_DefaultValue, m_Names);
}

template <class T>
std::any VectorParameter<T>::defaultValue() const
{
  return defaultVector();
}

template <class T>
const typename VectorParameter<T>::NamesType& VectorParameter<T>::names() const
{
  return m_Names;
}

template <class T>
usize VectorParameter<T>::size() const
{
  return m_DefaultValue.size();
}

template <class T>
const typename VectorParameter<T>::ValueType& VectorParameter<T>::defaultVector() const
{
  return m_DefaultValue;
}

template <class T>
Result<> VectorParameter<T>::validate(const std::any& value) const
{
  const auto& vec = GetAnyRef<ValueType>(value);
  return validateVector(vec);
}

template <class T>
Result<> VectorParameter<T>::validateVector(const ValueType& value) const
{
  usize requiredSize = m_DefaultValue.size();
  usize size = value.size();

  if(size != requiredSize)
  {
    return MakeErrorResult(-1, fmt::format("VectorParameter requires size {}. Received size {}.", requiredSize, size));
  }

  return {};
}

template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<int8>;
template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<uint8>;

template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<int16>;
template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<uint16>;

template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<int32>;
template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<uint32>;

template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<int64>;
template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<uint64>;

template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<float32>;
template class SIMPLNX_TEMPLATE_EXPORT VectorParameter<float64>;

namespace SIMPLConversion
{
namespace
{
enum ConversionState
{
  Underflow,
  Overflow,
  ZeroBound,
  Clear
};

template <typename ExpectedT, typename TempT>
ConversionState ConversionChecks(TempT temp)
{
  if constexpr(std::is_same_v<ExpectedT, TempT>)
  {
    return Clear;
  }

  constexpr ExpectedT lowerBound = std::numeric_limits<ExpectedT>::lowest();
  constexpr ExpectedT upperBound = std::numeric_limits<ExpectedT>::max();

  // uint8/16/32/64 <- float32/64, int8/16/32/64
  if constexpr(!std::is_floating_point_v<ExpectedT> && std::is_unsigned_v<ExpectedT> && std::is_signed_v<TempT>)
  {
    if(temp < static_cast<TempT>(0))
    {
      return ZeroBound;
    }
  }
  // int8/16/32/64, float32/64 <- int8/16/32/64, float32/64
  else if constexpr(!std::is_unsigned_v<TempT>)
  {
    if(temp < lowerBound)
    {
      return Underflow;
    }
  }
  // int8/16/32/64 <- uint8/16/32/64
  // Can't underflow by definition

  if(temp > upperBound)
  {
    return Overflow;
  }

  return Clear;
}

template <class OutputT>
Result<OutputT> FormatErrorMessage(std::string&& prefix, ConversionState convState)
{
  if(convState == ConversionState::Underflow)
  {
    return MakeErrorResult<OutputT>(-6, prefix + " underflow");
  }
  if(convState == ConversionState::Overflow)
  {
    return MakeErrorResult<OutputT>(-7, prefix + " overflow");
  }

  return {};
}

template <typename T>
Result<std::vector<T>> KeyValueConversion(const nlohmann::json& json, const std::string& key, usize pos, std::vector<T>& outputVec)
{
  using OutputT = std::vector<T>;

  if(!json.contains(key))
  {
    return MakeErrorResult<OutputT>(-2, fmt::format("Vec{}FilterParameter json '{}' does not contain a value for key '{}'", outputVec.size(), json.dump(), key));
  }

  if(!json[key].is_number())
  {
    return MakeErrorResult<OutputT>(-3, fmt::format("Vec{}FilterParameter json '{}' value for key '{}' is not a number", outputVec.size(), json.dump(), key));
  }

  ConversionState convState;

  if(json[key].is_number_integer())
  {
    if(json[key].is_number_unsigned())
    {
      convState = ConversionChecks<T, uint64>(json[key].get<uint64>());
    }
    else
    {
      convState = ConversionChecks<T, int64>(json[key].get<int64>());
    }
  }
  else
  {
    convState = ConversionChecks<T, float64>(json[key].get<float64>());
  }

  switch(convState)
  {
  case ZeroBound: {
    outputVec[pos] = static_cast<T>(0);
    break;
  }
  case Clear: {
    outputVec[pos] = json[key].get<T>();
    break;
  }
  case Underflow: {
    [[fallthrough]];
  }
  case Overflow: {
    return FormatErrorMessage<OutputT>(fmt::format("Vec{}FilterParameter json '{}' cannot convert value at key `{}` to appropriate type without", outputVec.size(), json.dump(), key), convState);
  }
  }

  return {};
}

template <typename T>
Result<std::vector<T>> JsonValueConversion(const nlohmann::json& json, usize pos, std::vector<T>& outputVec)
{
  using OutputT = std::vector<T>;

  if(!json.is_number())
  {
    return MakeErrorResult<OutputT>(-3, fmt::format("Vec{}FilterParameter json '{}' value is not a number", outputVec.size(), json.dump()));
  }

  ConversionState convState;

  if(json.is_number_integer())
  {
    if(json.is_number_unsigned())
    {
      convState = ConversionChecks<T, uint64>(json.get<uint64>());
    }
    else
    {
      convState = ConversionChecks<T, int64>(json.get<int64>());
    }
  }
  else
  {
    convState = ConversionChecks<T, float64>(json.get<float64>());
  }

  switch(convState)
  {
  case ZeroBound: {
    outputVec[pos] = static_cast<T>(0);
    break;
  }
  case Clear: {
    outputVec[pos] = json.get<T>();
    break;
  }
  case Underflow: {
    [[fallthrough]];
  }
  case Overflow: {
    return FormatErrorMessage<OutputT>(fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without", outputVec.size(), json.dump()), convState);
  }
  }

  return {};
}

template <uint8 VecSizeV, typename T>
Result<std::vector<T>> SpacialVectorConvert(const nlohmann::json& json)
{
  using OutputT = std::vector<T>;

  static const std::string x = "x";
  static const std::string y = "y";
  static const std::string z = "z";
  static const std::string w = "w";

  if(!json.is_object())
  {
    return MakeErrorResult<OutputT>(-1, fmt::format("Vec3FilterParameter json '{}' is not an object", json.dump()));
  }

  OutputT value(VecSizeV);

  Result<OutputT> xConv = KeyValueConversion(json, x, 0, value);
  if(xConv.invalid())
  {
    return xConv;
  }

  if constexpr(VecSizeV > 1)
  {
    Result<OutputT> yConv = KeyValueConversion(json, y, 1, value);
    if(yConv.invalid())
    {
      return yConv;
    }
  }
  if constexpr(VecSizeV > 2)
  {
    Result<OutputT> zConv = KeyValueConversion(json, z, 2, value);
    if(zConv.invalid())
    {
      return zConv;
    }
  }
  if constexpr(VecSizeV > 3)
  {
    Result<OutputT> wConv = KeyValueConversion(json, w, 3, value);
    if(wConv.invalid())
    {
      return wConv;
    }
  }

  return {std::move(value)};
}
} // namespace

/*
"Max": 0,
"Min": 0
*/

Result<RangeFilterParameterConverter::ValueType> RangeFilterParameterConverter::convert(const nlohmann::json& json)
{
  constexpr StringLiteral k_MaxKey = "Max";
  constexpr StringLiteral k_MinKey = "Min";

  std::vector<float64> value(2);
  Result<std::vector<float64>> minConv = KeyValueConversion(json, k_MinKey, 0, value);
  if(minConv.invalid())
  {
    return minConv;
  }

  Result<std::vector<float64>> maxConv = KeyValueConversion(json, k_MaxKey, 1, value);
  if(maxConv.invalid())
  {
    return maxConv;
  }

  return {std::move(value)};
}

template <typename T>
Result<typename MultiToVec3FilterParameterConverter<T>::ValueType> MultiToVec3FilterParameterConverter<T>::convert(const nlohmann::json& json1, const nlohmann::json& json2,
                                                                                                                   const nlohmann::json& json3)
{
  using OutputT = std::vector<T>;

  static const std::string x = "x";
  static const std::string y = "y";
  static const std::string z = "z";

  std::vector<T> value(3);
  Result<OutputT> xConv = JsonValueConversion(json1, 0, value);
  if(xConv.invalid())
  {
    return xConv;
  }

  Result<OutputT> yConv = JsonValueConversion(json2, 1, value);
  if(yConv.invalid())
  {
    return yConv;
  }

  Result<OutputT> zConv = JsonValueConversion(json3, 2, value);
  if(zConv.invalid())
  {
    return zConv;
  }

  return {std::move(value)};
}

template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT MultiToVec3FilterParameterConverter<float64>;

template <typename T>
Result<typename Vec2FilterParameterConverter<T>::ValueType> Vec2FilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  return SpacialVectorConvert<2, T>(json);
}

template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec2FilterParameterConverter<float64>;

template <typename T>
Result<typename Vec3FilterParameterConverter<T>::ValueType> Vec3FilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  return SpacialVectorConvert<3, T>(json);
}

template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3FilterParameterConverter<float64>;

template <typename T>
Result<typename Vec4FilterParameterConverter<T>::ValueType> Vec4FilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  return SpacialVectorConvert<4, T>(json);
}

template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec4FilterParameterConverter<float64>;

template <typename T>
Result<typename AxisAngleFilterParameterConverter<T>::ValueType> AxisAngleFilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  using OutputT = std::vector<T>;

  static const std::string angle = "angle";
  static const std::string h = "h";
  static const std::string k = "k";
  static const std::string l = "l";

  if(!json.is_object())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("AxisAngleFilterParameterConverter json '{}' is not an object", json.dump()));
  }

  std::vector<T> value(4);

  Result<OutputT> angleConv = KeyValueConversion(json, angle, 0, value);
  if(angleConv.invalid())
  {
    return angleConv;
  }

  Result<OutputT> hConv = KeyValueConversion(json, h, 1, value);
  if(hConv.invalid())
  {
    return hConv;
  }

  Result<OutputT> kConv = KeyValueConversion(json, k, 2, value);
  if(kConv.invalid())
  {
    return kConv;
  }

  Result<OutputT> lConv = KeyValueConversion(json, l, 3, value);
  if(lConv.invalid())
  {
    return lConv;
  }

  return {std::move(value)};
}

template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT AxisAngleFilterParameterConverter<float64>;

template <typename T>
Result<typename Vec3p1FilterParameterConverter<T>::ValueType> Vec3p1FilterParameterConverter<T>::convert(const nlohmann::json& json1, const nlohmann::json& json2)
{
  Result<Vec3p1FilterParameterConverter<T>::ValueType> result = SpacialVectorConvert<3, T>(json1);
  if(result.invalid())
  {
    return result;
  }

  std::vector<T> outputVec = result.value();
  outputVec.resize(4);
  Result<std::vector<T>> xConv = JsonValueConversion(json2, 3, outputVec);
  if(xConv.invalid())
  {
    return xConv;
  }

  return {outputVec};
}

template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT Vec3p1FilterParameterConverter<float64>;
} // namespace SIMPLConversion
} // namespace nx::core
