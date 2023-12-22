#include "simplnx/DataStructure/Observers/AbstractDataStructureObserver.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

using namespace nx::core;

AbstractDataStructureObserver::AbstractDataStructureObserver()
{
}

AbstractDataStructureObserver::~AbstractDataStructureObserver()
{
  stopObservingStructure();
}

DataStructure* AbstractDataStructureObserver::getObservedStructure() const
{
  return m_ObservedStructure;
}

bool AbstractDataStructureObserver::isObservingStructure() const
{
  return m_ObservedStructure != nullptr;
}

void AbstractDataStructureObserver::startObservingStructure(DataStructure* observedDataStructure)
{
  if(observedDataStructure == nullptr)
  {
    return;
  }
  else if(isObservingStructure())
  {
    stopObservingStructure();
  }

  m_ObservedStructure = observedDataStructure;
  m_Connection = observedDataStructure->getSignal().connect([this](DataStructure* dataStructure, const std::shared_ptr<AbstractDataStructureMessage>& msg) { this->onNotify(dataStructure, msg); });
}

void AbstractDataStructureObserver::stopObservingStructure()
{
  m_ObservedStructure = nullptr;
  m_Connection.disconnect();
}
