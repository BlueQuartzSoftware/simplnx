#include "AbstractDataStructureMessage.hpp"

#include <utility>

using namespace complex;

AbstractDataStructureMessage::AbstractDataStructureMessage(const DataStructure* dataGraph)
: m_DataStructure(dataGraph)
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
