#include "UnknownMetadataValue.hpp"

using namespace nx::core;

UnknownMetadataValue::UnknownMetadataValue(const nlohmann::json& json)
: ParentType()
, m_Json(json)
{
}

std::string UnknownMetadataValue::getTypeName() const
{
  return k_TypeName;
}

nlohmann::json UnknownMetadataValue::toJson() const
{
  return m_Json;
}

void UnknownMetadataValue::fromJson(const nlohmann::json& json)
{
  m_Json = json;
}
