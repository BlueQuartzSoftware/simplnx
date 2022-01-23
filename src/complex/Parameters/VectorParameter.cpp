#include "VectorParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"
#include "complex/Common/TypeTraits.hpp"

namespace complex
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

template <class T>
nlohmann::json VectorParameter<T>::toJson(const std::any& value) const
{
  const auto& vec = GetAnyRef<ValueType>(value);

  auto jsonArray = nlohmann::json::array();
  for(T value : vec)
  {
    jsonArray.push_back(value);
  }

  return jsonArray;
}

template <class T>
Result<std::any> VectorParameter<T>::fromJson(const nlohmann::json& json) const
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
  return {};
}

template class COMPLEX_TEMPLATE_EXPORT VectorParameter<int8>;
template class COMPLEX_TEMPLATE_EXPORT VectorParameter<uint8>;

template class COMPLEX_TEMPLATE_EXPORT VectorParameter<int16>;
template class COMPLEX_TEMPLATE_EXPORT VectorParameter<uint16>;

template class COMPLEX_TEMPLATE_EXPORT VectorParameter<int32>;
template class COMPLEX_TEMPLATE_EXPORT VectorParameter<uint32>;

template class COMPLEX_TEMPLATE_EXPORT VectorParameter<int64>;
template class COMPLEX_TEMPLATE_EXPORT VectorParameter<uint64>;

template class COMPLEX_TEMPLATE_EXPORT VectorParameter<float32>;
template class COMPLEX_TEMPLATE_EXPORT VectorParameter<float64>;
} // namespace complex
