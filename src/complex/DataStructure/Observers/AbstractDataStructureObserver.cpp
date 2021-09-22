#include "complex/DataStructure/Observers/AbstractDataStructureObserver.hpp"

#include "complex/DataStructure/DataStructure.hpp"

using namespace complex;

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

void AbstractDataStructureObserver::startObservingStructure(DataStructure* ds)
{
  if(m_ObservedStructure != nullptr)
  {
    return;
  }
  else if(ds == nullptr)
  {
    return;
  }

  m_ObservedStructure = ds;
  m_Connection = ds->getSignal().connect([this](DataStructure* ds, const std::shared_ptr<AbstractDataStructureMessage>& msg) { this->onNotify(ds, msg); });
}

void AbstractDataStructureObserver::stopObservingStructure()
{
  m_ObservedStructure = nullptr;
  m_Connection.disconnect();
}
