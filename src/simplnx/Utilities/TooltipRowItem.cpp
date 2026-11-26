#include "TooltipRowItem.hpp"

#include <utility>

using namespace nx::core;

TooltipRowItem::TooltipRowItem()

{
}

TooltipRowItem::TooltipRowItem(std::string title)
: m_Type(Type::Title)
, m_Title(std::move(title))
{
}

TooltipRowItem::TooltipRowItem(std::string title, std::string value)
: m_Type(Type::Value)
, m_Title(std::move(title))
, m_Value(std::move(value))
{
}

TooltipRowItem::TooltipRowItem(const TooltipRowItem& other)
: m_Type(other.m_Type)
, m_Title(other.m_Title)
, m_Value(other.m_Value)
{
}

TooltipRowItem::TooltipRowItem(TooltipRowItem&& other) noexcept
: m_Type(other.m_Type)
, m_Title(std::move(other.m_Title))
, m_Value(std::move(other.m_Value))
{
}

TooltipRowItem::~TooltipRowItem() = default;

TooltipRowItem::Type TooltipRowItem::getType() const
{
  return m_Type;
}

std::string TooltipRowItem::getTitle() const
{
  return m_Title;
}

std::string TooltipRowItem::getValue() const
{
  return m_Value;
}

TooltipRowItem& TooltipRowItem::operator=(const TooltipRowItem& rhs)
{
  m_Type = rhs.m_Type;
  m_Title = rhs.m_Title;
  m_Value = rhs.m_Value;
  return *this;
}

TooltipRowItem& TooltipRowItem::operator=(TooltipRowItem&& rhs) noexcept
{
  m_Type = rhs.m_Type;
  m_Title = std::move(rhs.m_Title);
  m_Value = std::move(rhs.m_Value);
  return *this;
}

bool TooltipRowItem::operator==(const TooltipRowItem& rhs) const
{
  return (m_Type == rhs.m_Type) && (m_Title == rhs.m_Title) && (m_Value == rhs.m_Value);
}

bool TooltipRowItem::operator!=(const TooltipRowItem& rhs) const
{
  return (m_Type != rhs.m_Type) || (m_Title != rhs.m_Title) || (m_Value != rhs.m_Value);
}
