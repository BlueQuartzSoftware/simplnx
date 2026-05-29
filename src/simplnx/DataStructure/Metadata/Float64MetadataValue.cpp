#include "Float64MetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
Float64MetadataValue::Float64MetadataValue(ValueType value)
: ParentType()
, m_Value(value)
{
}

Float64MetadataValue::operator ValueType() const
{
  return m_Value;
}

Float64MetadataValue::ValueType Float64MetadataValue::getValue() const
{
  return m_Value;
}

void Float64MetadataValue::setValue(const ValueType& value)
{
  m_Value = value;
}

bool Float64MetadataValue::operator==(const ValueType& rhs) const
{
  return m_Value == rhs;
}

Float64MetadataValue::ParentType& Float64MetadataValue::operator=(const ValueType& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string Float64MetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

nlohmann::json Float64MetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey.str()] = k_TypeName;
  json[k_ValueKey.str()] = m_Value;
  return json;
}

void Float64MetadataValue::fromJsonImpl(const nlohmann::json& json)
{
  m_Value = json[k_ValueKey.str()].get<float64>();
}
} // namespace nx::core
