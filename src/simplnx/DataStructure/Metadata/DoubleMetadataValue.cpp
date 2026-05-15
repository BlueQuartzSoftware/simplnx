#include "DoubleMetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
DoubleMetadataValue::DoubleMetadataValue(ValueType value)
: ParentType()
, m_Value(value)
{
}

DoubleMetadataValue::operator ValueType() const
{
  return m_Value;
}

DoubleMetadataValue::ValueType DoubleMetadataValue::getValue() const
{
  return m_Value;
}

void DoubleMetadataValue::setValue(const ValueType& value)
{
  m_Value = value;
}

bool DoubleMetadataValue::operator==(const ValueType& rhs) const
{
  return m_Value == rhs;
}

DoubleMetadataValue::ParentType& DoubleMetadataValue::operator=(const ValueType& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string DoubleMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

nlohmann::json DoubleMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey.str()] = k_TypeName;
  json[k_ValueKey.str()] = m_Value;
  return json;
}

void DoubleMetadataValue::fromJsonImpl(const nlohmann::json& json)
{
  m_Value = json[k_ValueKey.str()].get<float64>();
}
} // namespace nx::core
