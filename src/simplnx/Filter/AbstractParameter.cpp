#include "AbstractParameter.hpp"

#include <utility>

namespace nx::core
{
AbstractParameter::AbstractParameter(std::string  name, std::string  humanName, std::string  helpText)
: m_Name(std::move(name))
, m_HumanName(std::move(humanName))
, m_HelpText(std::move(helpText))
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
