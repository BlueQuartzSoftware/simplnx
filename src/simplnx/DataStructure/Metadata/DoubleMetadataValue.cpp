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

DoubleMetadataValue::ParentType& DoubleMetadataValue::operator=(const ValueType& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string DoubleMetadataValue::getTypeName() const
{
  return k_TypeName;
}

std::string DoubleMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey] = k_TypeName;
  json[k_ValueKey] = m_Value;

  return json;
}

void DoubleMetadataValue::fromJsonImpl(const std::string& jsonStr)
{
  nlohmann::json json(jsonStr);
  m_Value = json[k_ValueKey].get<float64>();
}
} // namespace nx::core
