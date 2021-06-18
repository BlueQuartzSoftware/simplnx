#include "TooltipGenerator.hpp"

using namespace complex;

TooltipGenerator::TooltipGenerator()
{
}

TooltipGenerator::TooltipGenerator(const TooltipGenerator& other)
: m_Rows(other.m_Rows)
{
}

TooltipGenerator::TooltipGenerator(TooltipGenerator&& other) noexcept
: m_Rows(std::move(other.m_Rows))
{
}

TooltipGenerator::~TooltipGenerator() = default;

void TooltipGenerator::addTitle(const std::string& title)
{
  m_Rows.emplace_back(title);
}

void TooltipGenerator::addValue(const std::string& name, const std::string& value)
{
  m_Rows.emplace_back(name, value);
}

void TooltipGenerator::addSpacer()
{
  m_Rows.emplace_back();
}

void TooltipGenerator::append(const TooltipGenerator& other)
{
  m_Rows.insert(m_Rows.end(), other.m_Rows.begin(), other.m_Rows.end());
}

void TooltipGenerator::clear()
{
  m_Rows.clear();
}

std::string TooltipGenerator::getRowColorStr() const
{
  return m_ColorStr;
}

void TooltipGenerator::setRowColorStr(const std::string& colorStr)
{
  m_ColorStr = colorStr;
}

std::string TooltipGenerator::generateHTML() const
{
  throw std::exception();
}

std::string TooltipGenerator::rowToHTML(const TooltipRowItem& row)
{
  throw std::exception();
}
