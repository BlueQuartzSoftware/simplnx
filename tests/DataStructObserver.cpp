#include "DataStructObserver.h"

#include <iostream>
#include <string>

#include "Complex/DataStructure/BaseGroup.h"
#include "Complex/DataStructure/DataStructure.h"
#include "Complex/DataStructure/Messaging/DataAddedMessage.h"
#include "Complex/DataStructure/Messaging/DataRemovedMessage.h"
#include "Complex/DataStructure/Messaging/DataRenamedMessage.h"
#include "Complex/DataStructure/Messaging/DataReparentedMessage.h"

DataStructObserver::DataStructObserver(DataStructure& dataStruct)
: AbstractDataStructureObserver()
, m_DataStructure(dataStruct)
{
}

DataStructObserver::~DataStructObserver() = default;

void DataStructObserver::onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& msg)
{
  switch(msg->getMsgType())
  {
  case DataAddedMessage::MsgType:
    handleDataMsg(target, dynamic_cast<DataAddedMessage*>(msg.get()));
    break;
  case DataRemovedMessage::MsgType:
    handleDataMsg(target, dynamic_cast<DataRemovedMessage*>(msg.get()));
    break;
  case DataRenamedMessage::MsgType:
    handleDataMsg(target, dynamic_cast<DataRenamedMessage*>(msg.get()));
    break;
  case DataReparentedMessage::MsgType:
    handleDataMsg(target, dynamic_cast<DataReparentedMessage*>(msg.get()));
    break;
  }
}

const Complex::DataStructure& DataStructObserver::getDataStructure() const
{
  return m_DataStructure;
}

void DataStructObserver::handleDataMsg(DataStructure* ds, DataAddedMessage* msg)
{
  std::cout << " Data Created...\n";
  for(auto& path : msg->getDataPaths())
  {
    std::cout << "  Added: " << path.toString() << "\n";
  }
  std::cout << "\n";
}

void DataStructObserver::handleDataMsg(DataStructure* ds, DataRemovedMessage* msg)
{
  std::cout << " Data Deleted...\n";
  std::cout << "  Removed: " << msg->getName() << "\n\n";
}

void DataStructObserver::handleDataMsg(DataStructure* ds, DataRenamedMessage* msg)
{
  std::cout << " Data Renamed...\n";
  std::cout << "  " << msg->getOldName() << " -> " << msg->getNewName() << "\n";
  for(auto& path : msg->getData()->getDataPaths())
  {
    std::cout << "  Updated: " << path.toString() << "\n";
  }
  std::cout << "\n";
}

void DataStructObserver::handleDataMsg(DataStructure* ds, DataReparentedMessage* msg)
{
  std::string addedStr = "Added " + msg->getTargetData()->getName() + " to " + msg->getParentData()->getName();
  std::string removedStr = "Removed " + msg->getTargetData()->getName() + " from " + msg->getParentData()->getName();

  std::cout << " Data Reparented...\n";
  std::cout << "  " << (msg->wasParentAdded() ? addedStr : removedStr) << "\n";
  for(auto& path : msg->getTargetData()->getDataPaths())
  {
    std::cout << "  -" << path.toString() << "\n";
  }
  std::cout << "\n";
}

void DataStructObserver::printDataContainer(BaseGroup* target, const std::string& prefix) const
{
  std::cout << prefix << target->getName() << "\n";
  std::string newPrefix = prefix + " | ";
  for(auto& data : *target)
  {
    printData(data.second.get(), newPrefix);
  }
}

void DataStructObserver::printDataObject(DataObject* data, const std::string& prefix) const
{
  std::cout << prefix << data->getName() << "\n";
}

void DataStructObserver::printData(DataObject* data, const std::string& prefix) const
{
  if(auto container = dynamic_cast<BaseGroup*>(data); container != nullptr)
  {
    printDataContainer(container, prefix);
  }
  else
  {
    printDataObject(data, prefix);
  }
}

void DataStructObserver::printDataStructure() const
{
  std::cout << " --------------\n";
  for(auto& data : getDataStructure())
  {
    printData(data.second.get());
  }

  std::cout << std::endl;
}
