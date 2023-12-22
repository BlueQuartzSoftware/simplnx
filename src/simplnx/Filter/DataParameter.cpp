#include "DataParameter.hpp"

namespace nx::core
{
DataParameter::DataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, Category category)
: AbstractParameter(name, humanName, helpText)
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
} // namespace nx::core
