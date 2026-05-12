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

IntMetadataValue::ParentType& IntMetadataValue::operator=(const int32& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string IntMetadataValue::getTypeName() const
{
  return k_TypeName;
}

std::string IntMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey] = k_TypeName;
  json[k_ValueKey] = m_Value;

  return json;
}

void IntMetadataValue::fromJsonImpl(const std::string& jsonStr)
{
  nlohmann::json json(jsonStr);
  m_Value = json[k_ValueKey].get<int32>();
}
} // namespace nx::core
