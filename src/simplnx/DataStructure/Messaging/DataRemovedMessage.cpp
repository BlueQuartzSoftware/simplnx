#include "DataRemovedMessage.hpp"

using namespace nx::core;

DataRemovedMessage::DataRemovedMessage(const DataStructure* dataStructure, DataObject::IdType identifier, const std::string& name)
: AbstractDataStructureMessage(dataStructure)
, m_Name(name)
, m_Id(identifier)
{
}

DataRemovedMessage::DataRemovedMessage(const DataRemovedMessage& other)
: AbstractDataStructureMessage(other)
, m_Name(other.m_Name)
, m_Id(other.m_Id)
{
}

DataRemovedMessage::DataRemovedMessage(DataRemovedMessage&& other) noexcept
: AbstractDataStructureMessage(other)
, m_Name(std::move(other.m_Name))
, m_Id(std::move(other.m_Id))
{
}

DataRemovedMessage::~DataRemovedMessage() = default;

AbstractDataStructureMessage::MessageType DataRemovedMessage::getMsgType() const
{
  return MsgType;
}

DataObject::IdType DataRemovedMessage::getId() const
{
  return m_Id;
}

std::string DataRemovedMessage::getName() const
{
  return m_Name;
}
