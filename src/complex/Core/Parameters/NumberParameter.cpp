#include "NumberParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace
{
template <class...>
constexpr std::false_type always_false{};
} // namespace

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
  if constexpr(std::is_same_v<T, i8>)
  {
    constexpr Uuid uuid = *Uuid::FromString("cae73834-68f8-4235-b010-8bea87d8ff7a");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, u8>)
  {
    constexpr Uuid uuid = *Uuid::FromString("6c3efeff-ce8f-47c0-83d1-262f2b2dd6cc");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, i16>)
  {
    constexpr Uuid uuid = *Uuid::FromString("44ae56e8-e6e7-4e4d-8128-dd3dc2c6696e");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, u16>)
  {
    constexpr Uuid uuid = *Uuid::FromString("156a6f46-77e5-41d8-8f5a-65ba1da52f2a");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, i32>)
  {
    constexpr Uuid uuid = *Uuid::FromString("21acff45-a653-45db-a0d1-f43cd344b93a");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, u32>)
  {
    constexpr Uuid uuid = *Uuid::FromString("e9521130-276c-40c7-95d7-0b4cb4f80649");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, i64>)
  {
    constexpr Uuid uuid = *Uuid::FromString("b2039349-bd3a-4dbb-93d2-b4b5c633e697");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, u64>)
  {
    constexpr Uuid uuid = *Uuid::FromString("36d91b23-5500-4ed4-bdf3-d680f54ee5d1");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, f32>)
  {
    constexpr Uuid uuid = *Uuid::FromString("e4452dfe-2f70-4833-819e-0cbbec21289b");
    return uuid;
  }
  else if constexpr(std::is_same_v<T, f64>)
  {
    constexpr Uuid uuid = *Uuid::FromString("f2a18fff-a095-47d7-b436-ede41b5ea21a");
    return uuid;
  }
  else
  {
    static_assert(always_false<T>, "Invalid type");
  }
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
