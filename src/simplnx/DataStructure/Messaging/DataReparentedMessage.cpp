#include "DataReparentedMessage.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"

using namespace nx::core;

DataReparentedMessage::DataReparentedMessage(const DataStructure* dataStructure, DataObject::IdType targetData, DataObject::IdType targetParent, bool parentAdded)
: AbstractDataStructureMessage(dataStructure)
, m_TargetId(targetData)
, m_ParentId(targetParent)
, m_ParentAdded(parentAdded)
{
}

DataReparentedMessage::DataReparentedMessage(const DataReparentedMessage& other)
: AbstractDataStructureMessage(other)
, m_TargetId(other.m_TargetId)
, m_ParentId(other.m_ParentId)
, m_ParentAdded(other.m_ParentAdded)
{
}

DataReparentedMessage::DataReparentedMessage(DataReparentedMessage&& other) noexcept
: AbstractDataStructureMessage(other)
, m_TargetId(std::move(other.m_TargetId))
, m_ParentId(std::move(other.m_ParentId))
, m_ParentAdded(std::move(other.m_ParentAdded))
{
}

DataReparentedMessage::~DataReparentedMessage() = default;

AbstractDataStructureMessage::MessageType DataReparentedMessage::getMsgType() const
{
  return MsgType;
}

DataObject::IdType DataReparentedMessage::getTargetId() const
{
  return m_TargetId;
}

DataObject::IdType DataReparentedMessage::getParentId() const
{
  return m_ParentId;
}

const DataObject* DataReparentedMessage::getTargetData() const
{
  return getDataStructure()->getData(m_TargetId);
}

const DataObject* DataReparentedMessage::getParentData() const
{
  return getDataStructure()->getData(m_ParentId);
}

bool DataReparentedMessage::wasParentAdded() const
{
  return m_ParentAdded;
}

bool DataReparentedMessage::wasParentRemoved() const
{
  return !m_ParentAdded;
}
