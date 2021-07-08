#include "DataStructObserver.hpp"

#include <iostream>
#include <string>

#include "Complex/DataStructure/BaseGroup.hpp"
#include "Complex/DataStructure/DataStructure.hpp"
#include "Complex/DataStructure/Messaging/DataAddedMessage.hpp"
#include "Complex/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "Complex/DataStructure/Messaging/DataRenamedMessage.hpp"
#include "Complex/DataStructure/Messaging/DataReparentedMessage.hpp"

DataStructObserver::DataStructObserver(DataStructure& dataStruct)
: AbstractDataStructureObserver()
, m_DataStructure(dataStruct)
{
  dataStruct.addObserver(this);
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
