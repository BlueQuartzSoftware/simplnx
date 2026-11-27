#include "AbstractPlugin.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <fmt/core.h>

#include <memory>
#include <stdexcept>
#include <utility>

using namespace nx::core;

AbstractPlugin::AbstractPlugin(IdType identifier, std::string name, std::string description, std::string vendor)
: m_Id(identifier)
, m_Name(std::move(name))
, m_Description(std::move(description))
, m_Vendor(std::move(vendor))
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

IFilter::UniquePointerType AbstractPlugin::createFilter(FilterHandle::FilterIdType identifier) const
{
  if(!containsFilterId(identifier))
  {
    return nullptr;
  }

  return m_InitializerMap.at(identifier)();
}

void AbstractPlugin::addFilter(const FilterCreationFuncType& filterFunc)
{
  IFilter::UniquePointerType filter = filterFunc();
  if(filter == nullptr)
  {
    throw std::runtime_error(fmt::format("Failed to instantiate filter in plugin '{}'", getName()));
  }

  Uuid uuid = filter->uuid();

  if(m_InitializerMap.contains(uuid))
  {
    IFilter::UniquePointerType existingFilter = m_InitializerMap[uuid]();
    throw std::runtime_error(
        fmt::format("Attempted to add filter '{}' with uuid '{}' in plugin '{}', but filter '{}' already exists with that uuid", filter->name(), uuid.str(), getName(), existingFilter->name()));
  }

  m_FilterHandles.insert(FilterHandle(*filter, getId()));
  m_InitializerMap[uuid] = filterFunc;
}

AbstractPlugin::IOManagersContainerType AbstractPlugin::getDataIOManagers() const
{
  return m_IOManagers;
}

void AbstractPlugin::addDataIOManager(const IoManagerPointerType& ioManager)
{
  m_IOManagers.push_back(ioManager);
}

void AbstractPlugin::addDefaultValue(std::string keyName, const nlohmann::json& value) const
{
  Preferences* preferencesPtr = Application::GetOrCreateInstance()->getPreferences();
  preferencesPtr->addDefaultValues(getName(), std::move(keyName), value);
}
