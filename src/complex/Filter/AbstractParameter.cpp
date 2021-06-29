#include "AbstractParameter.hpp"

namespace complex
{
AbstractParameter::AbstractParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const std::optional<std::any>& defaultValue)
: m_Name(name)
, m_HumanName(humanName)
, m_HelpText(helpText)
, m_DefaultValue(defaultValue)
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

std::optional<std::any> AbstractParameter::defaultValue() const
{
  return m_DefaultValue;
}
} // namespace complex
