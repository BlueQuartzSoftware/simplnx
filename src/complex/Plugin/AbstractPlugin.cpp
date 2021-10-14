#include "AbstractPlugin.hpp"

#include <memory>
#include <stdexcept>

#include <fmt/core.h>

#include "complex/Filter/IFilter.hpp"

using namespace complex;

AbstractPlugin::AbstractPlugin(IdType id, const std::string& name, const std::string& description, const std::string& vendor)
: m_Id(id)
, m_Name(name)
, m_Description(description)
, m_Vendor(vendor)
{
}

AbstractPlugin::~AbstractPlugin() = default;

std::string AbstractPlugin::getName() const
{
  return m_Name;
}

std::string AbstractPlugin::getDescription() const
{
  return m_Description;
}

AbstractPlugin::IdType AbstractPlugin::getId() const
{
  return m_Id;
}

std::string AbstractPlugin::getVendor() const
{
  return m_Vendor;
}

bool AbstractPlugin::containsFilterId(FilterHandle::FilterIdType uuid) const
{
  return m_InitializerMap.find(uuid) != m_InitializerMap.end();
}

AbstractPlugin::FilterContainerType AbstractPlugin::getFilterHandles() const
{
  return m_FilterHandles;
}

AbstractPlugin::FilterContainerType::size_type AbstractPlugin::getFilterCount() const
{
  return m_FilterHandles.size();
}

IFilter::UniquePointer AbstractPlugin::createFilter(FilterHandle::FilterIdType id) const
{
  if(!containsFilterId(id))
  {
    return nullptr;
  }

  return m_InitializerMap.at(id)();
}

void AbstractPlugin::addFilter(FilterCreationFunc filterFunc)
{
  IFilter::UniquePointer filter = filterFunc();
  if(filter == nullptr)
  {
    throw std::runtime_error(fmt::format("Failed to instantiate filter in plugin \"{}\"", getName()));
  }

  Uuid uuid = filter->uuid();

  if(m_InitializerMap.count(uuid) > 0)
  {
    IFilter::UniquePointer existingFilter = m_InitializerMap[uuid]();
    throw std::runtime_error(fmt::format("Attempted to add filter \"{}\" with uuid \"{}\" in plugin \"{}\", but filter \"{}\" already exists with that uuid", filter->name(), uuid.str(), getName(),
                                         existingFilter->name()));
  }

  m_FilterHandles.insert(FilterHandle(filter->humanName(), filter->className(), uuid, getId()));
  m_InitializerMap[uuid] = filterFunc;
}
