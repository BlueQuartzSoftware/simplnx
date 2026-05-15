#include "BaseMetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
std::string BaseMetadataValue::getTypeName() const
{
  return "[Not Implemented]";
}

nlohmann::json BaseMetadataValue::toJson() const
{
  nlohmann::json json;
  json[k_ValueTypeKey] = "Error";
  json[k_ValueKey] = "";
  return json;
}

void BaseMetadataValue::fromJson(const nlohmann::json& json)
{
  throw std::runtime_error("BaseMetadataValue::fromJson not implemented");
}
} // namespace nx::core
