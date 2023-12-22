#include "TooltipGenerator.hpp"

#include <sstream>

using namespace nx::core;

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
  TooltipRowItem spacer;

  std::stringstream ss;

  ss << "<html><head></head>\n";
  ss << "<body>\n";
  ss << "<table cellpadding=\"4\" cellspacing=\"0\" border=\"0\">\n";
  ss << "<tbody>\n";

  for(const auto& row : m_Rows)
  {
    ss << rowToHTML(row);
  }

  ss << rowToHTML(spacer);
  ss << "</tbody></table>\n";
  ss << "</body></html>";
  return ss.str();
}

std::string TooltipGenerator::rowToHTML(const TooltipRowItem& row) const
{
  std::stringstream ss;

  switch(row.getType())
  {
  case TooltipRowItem::Type::Title:
    ss << R"(<tr bgcolor=")" << getRowColorStr() << R"("><th align="center" colspan=2>)" << row.getTitle() << "</th></tr>";
    break;
  case TooltipRowItem::Type::Value:
    ss << R"(<tr bgcolor=")" << getRowColorStr() << R"("><th align="right">)" << row.getTitle() << ":</th><td>" << row.getValue() << "</td></tr>";
    break;
  case TooltipRowItem::Type::Spacer:
    ss << R"(<tr bgcolor=")" << getRowColorStr() << R"("><td></td><td></td></tr>)";
    break;
  }

  return ss.str();
}
