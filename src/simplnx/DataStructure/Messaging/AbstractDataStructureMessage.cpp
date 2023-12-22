#include "AbstractDataStructureMessage.hpp"

#include <utility>

using namespace nx::core;

AbstractDataStructureMessage::AbstractDataStructureMessage(const DataStructure* dataStructure)
: m_DataStructure(dataStructure)
{
}

AbstractDataStructureMessage::AbstractDataStructureMessage(const AbstractDataStructureMessage& other)
: m_DataStructure(other.m_DataStructure)
{
}

AbstractDataStructureMessage::AbstractDataStructureMessage(AbstractDataStructureMessage&& other) noexcept
: m_DataStructure(std::move(other.m_DataStructure))
{
}

AbstractDataStructureMessage::~AbstractDataStructureMessage() = default;

const DataStructure* AbstractDataStructureMessage::getDataStructure() const
{
  return m_DataStructure;
}
