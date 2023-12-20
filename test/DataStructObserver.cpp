#include "DataStructObserver.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Messaging/DataAddedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataRenamedMessage.hpp"
#include "simplnx/DataStructure/Messaging/DataReparentedMessage.hpp"

using namespace nx::core;

DataStructObserver::DataStructObserver(DataStructure& dataStruct)
: AbstractDataStructureObserver()
, m_DataStructure(dataStruct)
{
  startObservingStructure(&dataStruct);
}

DataStructObserver::~DataStructObserver() = default;

void DataStructObserver::onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& msg)
{
  switch(msg->getMsgType())
  {
  case DataAddedMessage::MsgType:
    m_AddedCount++;
    break;
  case DataRemovedMessage::MsgType:
    m_RemovedCount++;
    break;
  case DataRenamedMessage::MsgType:
    m_RenamedCount++;
    break;
  case DataReparentedMessage::MsgType:
    m_ReparentedCount++;
    break;
  }
}

const nx::core::DataStructure& DataStructObserver::getDataStructure() const
{
  return m_DataStructure;
}

usize DataStructObserver::getDataAddedCount() const
{
  return m_AddedCount;
}

usize DataStructObserver::getDataRemovedCount() const
{
  return m_RemovedCount;
}

usize DataStructObserver::getDataRenamedCount() const
{
  return m_RenamedCount;
}

usize DataStructObserver::getDataReparentedCount() const
{
  return m_ReparentedCount;
}
