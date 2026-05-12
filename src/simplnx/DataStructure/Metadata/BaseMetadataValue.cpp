#include "BaseMetadataValue.hpp"

#include "nlohmann/json.hpp"

namespace nx::core
{
std::string BaseMetadataValue::toJson() const
{
  nlohmann::json json;
  json[k_ValueTypeKey] = "Error";
  json[k_ValueKey] = "";
  return "";
}

void BaseMetadataValue::fromJson(const std::string& json)
{
  throw std::runtime_error("BaseMetadataValue::fromJson not implemented");
}
} // namespace nx::core
