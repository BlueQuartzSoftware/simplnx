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

template <class T>
nlohmann::json NumberParameter<T>::toJson(const std::any& value) const
{
  auto number = std::any_cast<ValueType>(value);
  nlohmann::json json = number;
  return json;
}

template <class T>
Result<std::any> NumberParameter<T>::fromJson(const nlohmann::json& json) const
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
} // namespace nx::core
