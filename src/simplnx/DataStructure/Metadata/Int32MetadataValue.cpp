#include "Int32MetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
Int32MetadataValue::Int32MetadataValue(int32 value)
: AbstractMetadataValue<int32>()
, m_Value(value)
{
}

Int32MetadataValue::operator int32() const
{
  return m_Value;
}

Int32MetadataValue::ValueType Int32MetadataValue::getValue() const
{
  return m_Value;
}

void Int32MetadataValue::setValue(const ValueType& value)
{
  m_Value = value;
}

bool Int32MetadataValue::operator==(const ValueType& rhs) const
{
  return m_Value == rhs;
}

Int32MetadataValue::ParentType& Int32MetadataValue::operator=(const int32& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string Int32MetadataValue::getTypeNameImpl() const
{
  return k_TypeName;
}

nlohmann::json Int32MetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey.str()] = k_TypeName;
  json[k_ValueKey.str()] = m_Value;
  return json;
}

void Int32MetadataValue::fromJsonImpl(const nlohmann::json& json)
{
  m_Value = json[k_ValueKey.str()].get<int32>();
}
} // namespace nx::core
