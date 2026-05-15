#include "IntMetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
IntMetadataValue::IntMetadataValue(int32 value)
: AbstractMetadataValue<int32>()
, m_Value(value)
{
}

IntMetadataValue::operator int32() const
{
  return m_Value;
}

IntMetadataValue::ValueType IntMetadataValue::getValue() const
{
  return m_Value;
}

void IntMetadataValue::setValue(const ValueType& value)
{
  m_Value = value;
}

bool IntMetadataValue::operator==(const ValueType& rhs) const
{
  return m_Value == rhs;
}

IntMetadataValue::ParentType& IntMetadataValue::operator=(const int32& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string IntMetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

nlohmann::json IntMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey.str()] = k_TypeName;
  json[k_ValueKey.str()] = m_Value;
  return json;
}

void IntMetadataValue::fromJsonImpl(const nlohmann::json& json)
{
  m_Value = json[k_ValueKey.str()].get<int32>();
}
} // namespace nx::core
