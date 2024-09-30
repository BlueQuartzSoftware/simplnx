#include "NumberParameter.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
template <class T>
NumberParameter<T>::NumberParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

template <class T>
Uuid NumberParameter<T>::uuid() const
{
  return ParameterTraits<NumberParameter<T>>::uuid;
}

template <class T>
IParameter::AcceptedTypes NumberParameter<T>::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
template <class T>
IParameter::VersionType NumberParameter<T>::getVersion() const
{
  return 1;
}

template <class T>
nlohmann::json NumberParameter<T>::toJsonImpl(const std::any& value) const
{
  auto number = std::any_cast<ValueType>(value);
  nlohmann::json json = number;
  return json;
}

template <class T>
Result<std::any> NumberParameter<T>::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'NumberParameter' JSON Error: ";
  if(!json.is_number())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a number", prefix, name()));
  }
  auto number = json.get<ValueType>();
  return {{number}};
}

template <class T>
IParameter::UniquePointer NumberParameter<T>::clone() const
{
  return std::make_unique<NumberParameter<T>>(name(), humanName(), helpText(), m_DefaultValue);
}

template <class T>
std::any NumberParameter<T>::defaultValue() const
{
  return defaultNumber();
}

template <class T>
typename NumberParameter<T>::ValueType NumberParameter<T>::defaultNumber() const
{
  return m_DefaultValue;
}

template <class T>
Result<> NumberParameter<T>::validate(const std::any& value) const
{
  auto castValue = std::any_cast<ValueType>(value);
  return validateNumber(castValue);
}

template <class T>
Result<> NumberParameter<T>::validateNumber(ValueType value) const
{
  return {};
}

template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<int8>;
template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<uint8>;

template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<int16>;
template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<uint16>;

template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<int32>;
template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<uint32>;

template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<int64>;
template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<uint64>;

template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<float32>;
template class SIMPLNX_TEMPLATE_EXPORT NumberParameter<float64>;

namespace SIMPLConversion
{
template <typename T>
Result<typename IntFilterParameterConverter<T>::ValueType> IntFilterParameterConverter<T>::convert(const nlohmann::json& json)
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

template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT IntFilterParameterConverter<uint64>;

template <typename T>
Result<typename StringToIntFilterParameterConverter<T>::ValueType> StringToIntFilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  if(!json.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("StringToIntFilterParameterConverter json '{}' is not a string", json.dump()));
  }

  auto value = static_cast<T>(std::stoi(json.get<std::string>()));
  return {value};
}

template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT StringToIntFilterParameterConverter<uint64>;

template <class T>
Result<typename FloatFilterParameterConverter<T>::ValueType> FloatFilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  if(!json.is_number())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("FloatFilterParameter json '{}' is not a number", json.dump()));
  }

  auto value = json.get<T>();

  return {value};
}

template struct SIMPLNX_TEMPLATE_EXPORT FloatFilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT FloatFilterParameterConverter<float64>;
} // namespace SIMPLConversion
} // namespace nx::core
