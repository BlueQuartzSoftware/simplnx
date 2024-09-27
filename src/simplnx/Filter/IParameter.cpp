#include "IParameter.hpp"

#include "simplnx/Common/StringLiteral.hpp"

#include <nlohmann/json.hpp>

namespace nx::core
{
namespace
{
constexpr StringLiteral k_ValueKey = "value";
constexpr StringLiteral k_VersionKey = "version";
} // namespace

std::any IParameter::construct(const Arguments& args) const
{
  return args.at(name());
}

nlohmann::json IParameter::toJson(const std::any& value) const
{
  nlohmann::json json;

  json[k_ValueKey] = toJsonImpl(value);
  json[k_VersionKey] = getVersion();

  return json;
}

Result<std::any> IParameter::fromJson(const nlohmann::json& json) const
{
  std::vector<Warning> warnings;
  VersionType version = 1;
  if(!json.contains(k_VersionKey))
  {
    warnings.push_back(Warning{-1, fmt::format("Parameter key '{}' does not exist. Assuming version={}", k_VersionKey, version)});
    nlohmann::json versionJson = json[k_VersionKey];
    if(!versionJson.is_number_unsigned())
    {
      return MakeErrorResult<std::any>(-2, fmt::format("Parameter key '{}' is not an unsigned integer", k_VersionKey));
    }

    version = versionJson.get<VersionType>();
  }

  if(!json.contains(k_ValueKey))
  {
    return MakeErrorResult<std::any>(-3, fmt::format("Parameter key '{}' does not exist", k_ValueKey));
  }

  Result<std::any> result = fromJsonImpl(json[k_ValueKey], version);

  result.warnings().insert(result.warnings().begin(), warnings.cbegin(), warnings.cend());

  return result;
}
} // namespace nx::core
