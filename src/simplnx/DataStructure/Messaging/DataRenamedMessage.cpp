#include "DataRenamedMessage.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"

using namespace nx::core;

DataRenamedMessage::DataRenamedMessage(const DataStructure* dataStructure, AbstractDataObject::IdType identifier, const std::string& oldName, const std::string& newName)
: AbstractDataStructureMessage(dataStructure)
, m_Id(identifier)
, m_OldName(oldName)
, m_NewName(newName)
{
}

DataRenamedMessage::DataRenamedMessage(const DataRenamedMessage& other)
: AbstractDataStructureMessage(other)
, m_Id(other.m_Id)
, m_OldName(other.m_OldName)
, m_NewName(other.m_NewName)
{
}

DataRenamedMessage::DataRenamedMessage(DataRenamedMessage&& other) noexcept
: AbstractDataStructureMessage(other)
, m_Id(std::move(other.m_Id))
, m_OldName(std::move(other.m_OldName))
, m_NewName(std::move(other.m_NewName))
{
}

DataRenamedMessage::~DataRenamedMessage() = default;

AbstractDataStructureMessage::MessageType DataRenamedMessage::getMsgType() const
{
  return MsgType;
}

AbstractDataObject::IdType DataRenamedMessage::getDataId() const
{
  return m_Id;
}

const AbstractDataObject* DataRenamedMessage::getData() const
{
  return getDataStructure()->getData(m_Id);
}

std::string DataRenamedMessage::getPreviousName() const
{
  return m_OldName;
}

std::string DataRenamedMessage::getNewName() const
{
  return m_NewName;
}
