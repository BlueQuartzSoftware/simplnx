#include "StringMetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
StringMetadataValue::StringMetadataValue()
: AbstractMetadataValue<std::string>()
{
}

StringMetadataValue::StringMetadataValue(const std::string& value)
: AbstractMetadataValue<std::string>()
, m_Value(value)
{
}

StringMetadataValue::operator StringMetadataValue::ValueType() const
{
  return m_Value;
}

StringMetadataValue::ValueType StringMetadataValue::getValue() const
{
  return m_Value;
}

void StringMetadataValue::setValue(const ValueType& value)
{
  m_Value = value;
}

bool StringMetadataValue::operator==(const ValueType& rhs) const
{
  return m_Value == rhs;
}

StringMetadataValue::ParentType& StringMetadataValue::operator=(const std::string& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string StringMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

nlohmann::json StringMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey.str()] = k_TypeName;
  json[k_ValueKey.str()] = m_Value;
  return json;
}

void StringMetadataValue::fromJsonImpl(const nlohmann::json& json)
{
  m_Value = json[k_ValueKey.str()].get<std::string>();
}
} // namespace nx::core
