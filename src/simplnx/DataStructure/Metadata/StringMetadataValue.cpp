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

StringMetadataValue::operator std::string() const
{
  return m_Value;
}

StringMetadataValue::ParentType& StringMetadataValue::operator=(const std::string& rhs)
{
  m_Value = rhs;
  return *this;
}

std::string StringMetadataValue::getTypeName() const
{
  return k_TypeName;
}

std::string StringMetadataValue::toJsonImpl() const
{
  nlohmann::json json;
  json[k_ValueTypeKey] = k_TypeName;
  json[k_ValueKey] = m_Value;

  return json;
}

void StringMetadataValue::fromJsonImpl(const std::string& jsonStr)
{
  nlohmann::json json(jsonStr);
  m_Value = json[k_ValueKey].get<std::string>();
}
} // namespace nx::core
