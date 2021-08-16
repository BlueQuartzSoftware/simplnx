#include "NumberParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
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
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_number())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a number", key)}})};
  }
  auto number = jsonValue.get<ValueType>();
  return {number};
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

template class COMPLEX_TEMPLATE_EXPORT NumberParameter<i8>;
template class COMPLEX_TEMPLATE_EXPORT NumberParameter<u8>;

template class COMPLEX_TEMPLATE_EXPORT NumberParameter<i16>;
template class COMPLEX_TEMPLATE_EXPORT NumberParameter<u16>;

template class COMPLEX_TEMPLATE_EXPORT NumberParameter<i32>;
template class COMPLEX_TEMPLATE_EXPORT NumberParameter<u32>;

template class COMPLEX_TEMPLATE_EXPORT NumberParameter<i64>;
template class COMPLEX_TEMPLATE_EXPORT NumberParameter<u64>;

template class COMPLEX_TEMPLATE_EXPORT NumberParameter<f32>;
template class COMPLEX_TEMPLATE_EXPORT NumberParameter<f64>;
} // namespace complex
