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
template<uint8 VecSizeV, typename T>
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

  if(!json.contains(x))
  {
    return MakeErrorResult<OutputT>(-2, fmt::format("Vec{}FilterParameter json '{}' does not contain an X value", VecSizeV, json.dump()));
  }

  if constexpr (std::is_floating_point_v<T>)
  {
    value[0] = json[x].get<T>();
  }
  else
  {
    if(json[x].is_number_integer())
    {
      value[0] = json[x].get<T>();
    }
    else
    {
      auto tmp = json[x].get<float64>();
      if(tmp < std::numeric_limits<T>::lowest())
      {
        if constexpr(std::is_unsigned_v<T>)
        {
          value[0] = 0;
        }
        else
        {
          return MakeErrorResult<OutputT>(-6, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without underflow", VecSizeV, json.dump()));
        }
      }
      else if(tmp > std::numeric_limits<T>::max())
      {
        return MakeErrorResult<OutputT>(-7, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without overflow", VecSizeV, json.dump()));
      }
      else
      {
        value[0] = static_cast<T>(tmp);
      }
    }
  }

  if constexpr (VecSizeV > 1)
  {
    if(!json.contains(y))
    {
      return MakeErrorResult<OutputT>(-3, fmt::format("Vec{}FilterParameter json '{}' does not contain an Y value", VecSizeV, json.dump()));
    }

    if constexpr (std::is_floating_point_v<T>)
    {
      value[1] = json[y].get<T>();
    }
    else
    {
      if(json[y].is_number_integer())
      {
        value[1] = json[y].get<T>();
      }
      else
      {
        auto tmp = json[y].get<float64>();
        if(tmp < std::numeric_limits<T>::lowest())
        {
          if constexpr(std::is_unsigned_v<T>)
          {
            value[1] = 0;
          }
          else
          {
            return MakeErrorResult<OutputT>(-6, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without underflow", VecSizeV, json.dump()));
          }
        }
        else if(tmp > std::numeric_limits<T>::max())
        {
          return MakeErrorResult<OutputT>(-7, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without overflow", VecSizeV, json.dump()));
        }
        else
        {
          value[1] = static_cast<T>(tmp);
        }
      }
    }
  }
  if constexpr (VecSizeV > 2)
  {
    if(!json.contains(z))
    {
      return MakeErrorResult<OutputT>(-4, fmt::format("Vec{}FilterParameter json '{}' does not contain an Z value", VecSizeV, json.dump()));
    }

    if constexpr (std::is_floating_point_v<T>)
    {
      value[2] = json[z].get<T>();
    }
    else
    {
      if(json[z].is_number_integer())
      {
        value[2] = json[z].get<T>();
      }
      else
      {
        auto tmp = json[z].get<float64>();
        if(tmp < std::numeric_limits<T>::lowest())
        {
          if constexpr(std::is_unsigned_v<T>)
          {
            value[2] = 0;
          }
          else
          {
            return MakeErrorResult<OutputT>(-6, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without underflow", VecSizeV, json.dump()));
          }
        }
        else if(tmp > std::numeric_limits<T>::max())
        {
          return MakeErrorResult<OutputT>(-7, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without overflow", VecSizeV, json.dump()));
        }
        else
        {
          value[2] = static_cast<T>(tmp);
        }
      }
    }
  }
  if constexpr (VecSizeV > 3)
  {
    if(!json.contains(w))
    {
      return MakeErrorResult<OutputT>(-5, fmt::format("Vec{}FilterParameter json '{}' does not contain an W value", VecSizeV, json.dump()));
    }

    if constexpr (std::is_floating_point_v<T>)
    {
      value[3] = json[w].get<T>();
    }
    else
    {
      if(json[w].is_number_integer())
      {
        value[3] = json[w].get<T>();
      }
      else
      {
        auto tmp = json[w].get<float64>();
        if(tmp < std::numeric_limits<T>::lowest())
        {
          if constexpr(std::is_unsigned_v<T>)
          {
            value[3] = 0;
          }
          else
          {
            return MakeErrorResult<OutputT>(-6, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without underflow", VecSizeV, json.dump()));
          }
        }
        else if(tmp > std::numeric_limits<T>::max())
        {
          return MakeErrorResult<OutputT>(-7, fmt::format("Vec{}FilterParameter json '{}' cannot convert value to appropriate type without overflow", VecSizeV, json.dump()));
        }
        else
        {
          value[3] = static_cast<T>(tmp);
        }
      }
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

template <typename T>
Result<typename MultiToVec3FilterParameterConverter<T>::ValueType> MultiToVec3FilterParameterConverter<T>::convert(const nlohmann::json& json1, const nlohmann::json& json2,
                                                                                                                   const nlohmann::json& json3)
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
  return SpacialVectorConvert<3, T>(json);
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

  result.value().push_back(json2.get<T>());

  return result;
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
