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

BoolMetadataValue::ParentType& BoolMetadataValue::operator=(const ValueType& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string BoolMetadataValue::getTypeName() const
{
  return k_TypeName;
}

std::string BoolMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey] = "bool";
  json[k_ValueKey] = m_Value;

  return json;
}

void BoolMetadataValue::fromJsonImpl(const std::string& jsonStr)
{
  nlohmann::json json(jsonStr);
  m_Value = json[k_ValueKey].get<bool>();
}
} // namespace nx::core
