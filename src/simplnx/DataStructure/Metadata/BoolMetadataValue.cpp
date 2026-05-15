#include "BoolMetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
BoolMetadataValue::BoolMetadataValue(bool value)
: ParentType()
, m_Value(value)
{
}

BoolMetadataValue::operator ValueType() const
{
  return m_Value;
}

BoolMetadataValue::ValueType BoolMetadataValue::getValue() const
{
  return m_Value;
}

void BoolMetadataValue::setValue(const ValueType& value)
{
  m_Value = value;
}

bool BoolMetadataValue::operator==(const ValueType& rhs) const
{
  return m_Value == rhs;
}

BoolMetadataValue::ParentType& BoolMetadataValue::operator=(const ValueType& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string BoolMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

nlohmann::json BoolMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey.str()] = k_TypeName;
  json[k_ValueKey.str()] = m_Value;
  return json;
}

void BoolMetadataValue::fromJsonImpl(const nlohmann::json& json)
{
  m_Value = json[k_ValueKey.str()].get<bool>();
}
} // namespace nx::core
