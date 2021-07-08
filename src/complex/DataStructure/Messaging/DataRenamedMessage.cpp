#include "DataRenamedMessage.hpp"

#include "complex/DataStructure/DataStructure.hpp"

using namespace complex;

DataRenamedMessage::DataRenamedMessage(const DataStructure* ds, DataObject::IdType id, const std::string& oldName, const std::string& newName)
: AbstractDataStructureMessage(ds)
, m_Id(id)
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

DataObject::IdType DataRenamedMessage::getDataId() const
{
  return m_Id;
}

const DataObject* DataRenamedMessage::getData() const
{
  return getDataStructure()->getData(m_Id);
}

std::string DataRenamedMessage::getOldName() const
{
  return m_OldName;
}

std::string DataRenamedMessage::getNewName() const
{
  return m_NewName;
}
