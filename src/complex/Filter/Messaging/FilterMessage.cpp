#include "FilterMessage.hpp"

using namespace complex;

FilterMessage::FilterMessage(Type type, const std::string& msg)
: m_Message(msg)
, m_Type(type)
{
}

FilterMessage::~FilterMessage() = default;

FilterMessage::Type FilterMessage::getType() const
{
  return m_Type;
}

std::string FilterMessage::getMessage() const
{
  return m_Message;
}
