#include "AbstractDataStructureMessage.hpp"

#include <utility>

using namespace complex;

AbstractDataStructureMessage::AbstractDataStructureMessage(const DataStructure* ds)
: m_DataStructure(ds)
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
