#include "DataStructObserver.hpp"

#include <iostream>
#include <string>

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Messaging/DataAddedMessage.hpp"
#include "complex/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "complex/DataStructure/Messaging/DataRenamedMessage.hpp"
#include "complex/DataStructure/Messaging/DataReparentedMessage.hpp"

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

const complex::DataStructure& DataStructObserver::getDataStructure() const
{
  return m_DataStructure;
}

size_t DataStructObserver::getDataAddedCount() const
{
  return m_AddedCount;
}

size_t DataStructObserver::getDataRemovedCount() const
{
  return m_RemovedCount;
}

size_t DataStructObserver::getDataRenamedCount() const
{
  return m_RenamedCount;
}

size_t DataStructObserver::getDataReparentedCount() const
{
  return m_ReparentedCount;
}
