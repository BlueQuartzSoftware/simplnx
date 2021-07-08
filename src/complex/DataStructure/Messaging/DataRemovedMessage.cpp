#include "DataRemovedMessage.hpp"

using namespace complex;

DataRemovedMessage::DataRemovedMessage(const DataStructure* ds, DataObject::IdType id, const std::string& name)
: AbstractDataStructureMessage(ds)
, m_Id(id)
, m_Name(name)
{
}

DataRemovedMessage::DataRemovedMessage(const DataRemovedMessage& other)
: AbstractDataStructureMessage(other)
, m_Id(other.m_Id)
, m_Name(other.m_Name)
{
}

DataRemovedMessage::DataRemovedMessage(DataRemovedMessage&& other) noexcept
: AbstractDataStructureMessage(other)
, m_Id(std::move(other.m_Id))
, m_Name(std::move(other.m_Name))
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
