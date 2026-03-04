#include "AbstractParameter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"

#include <nlohmann/json.hpp>

namespace nx::core
{
namespace
{
constexpr StringLiteral k_ValueKey = "value";
constexpr StringLiteral k_VersionKey = "version";
} // namespace

AbstractParameter::AbstractParameter(const std::string& name, const std::string& humanName, const std::string& helpText)
: m_Name(name)
, m_HumanName(humanName)
, m_HelpText(helpText)
{
  if(m_Name.empty())
  {
    throw std::runtime_error("empty name not allowed");
  }
}

std::string AbstractParameter::name() const
{
  return m_Name;
}

std::string AbstractParameter::humanName() const
{
  return m_HumanName;
}

std::string AbstractParameter::helpText() const
{
  return m_HelpText;
}

std::any AbstractParameter::construct(const Arguments& args, const ExecutionContext& executionContext) const
{
  return args.at(name());
}

nlohmann::json AbstractParameter::toJson(const std::any& value) const
{
  nlohmann::json json;

  json[k_ValueKey] = toJsonImpl(value);
  json[k_VersionKey] = getVersion();

  return json;
}

Result<std::any> AbstractParameter::fromJson(const nlohmann::json& json) const
{
  std::vector<Warning> warnings;
  VersionType version = 1;
  if(json.contains(k_VersionKey))
  {
    nlohmann::json versionJson = json[k_VersionKey];
    if(!versionJson.is_number_unsigned())
    {
      return MakeErrorResult<std::any>(-2, fmt::format("{}: Parameter key '{}' is not an unsigned integer", name(), k_VersionKey));
    }

    version = versionJson.get<VersionType>();
  }
  else
  {
    warnings.push_back(Warning{-1, fmt::format("{}: Parameter key '{}' does not exist. Assuming version={}. To fix this, save the pipeline file.", name(), k_VersionKey, version)});
  }

  nlohmann::json valueJson;
  if(json.contains(k_ValueKey))
  {
    valueJson = json[k_ValueKey];
  }
  else
  {
    warnings.push_back(Warning{-2, fmt::format("{}: Parameter key '{}' does not exist. Assuming json is the current json object", name(), k_ValueKey, version)});
    valueJson = json;
  }

  Result<std::any> result = fromJsonImpl(valueJson, version);

  result.warnings().insert(result.warnings().begin(), warnings.cbegin(), warnings.cend());

  return result;
}
} // namespace nx::core
