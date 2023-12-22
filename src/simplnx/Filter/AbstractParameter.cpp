#include "AbstractParameter.hpp"

namespace nx::core
{
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
} // namespace nx::core
