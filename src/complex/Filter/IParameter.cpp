#include "IParameter.hpp"

namespace complex
{
std::any IParameter::construct(const Arguments& args) const
{
  return args.at(name());
}

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

ValueParameter::Type ValueParameter::type() const
{
  return Type::Value;
}

DataParameter::DataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const std::optional<std::any>& default, Category category)
: AbstractParameter(name, humanName, helpText, default)
, m_Category(category)
{
}

DataParameter::Type DataParameter::type() const
{
  return Type::Data;
}

DataParameter::Category DataParameter::category() const
{
  return m_Category;
}

ConstDataParameter::Mutability ConstDataParameter::mutability() const
{
  return Mutability::Const;
}

MutableDataParameter::Mutability MutableDataParameter::mutability() const
{
  return Mutability::Mutable;
}
} // namespace complex
