#include "FilterList.hpp"

#include <memory>
#include <stdexcept>

#include <fmt/core.h>

#include "complex/Core/Application.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Plugin/PluginLoader.hpp"

using namespace complex;

FilterList::FilterList() = default;

FilterList::~FilterList() = default;

std::vector<FilterHandle> FilterList::search(const std::string& text) const
{
  std::vector<FilterHandle> handles;
  for(const auto& handle : getFilterHandles())
  {
    if(handle.getFilterName().find(text) != std::string::npos)
    {
      handles.push_back(handle);
    }
    else if(getPlugin(handle)->getName().find(text) != std::string::npos)
    {
      handles.push_back(handle);
    }
  }
  return handles;
}

IFilter::UniquePointer FilterList::createFilter(const std::string& filterName) const
{
  for(const auto& handle : getFilterHandles())
  {
    if(handle.getFilterName() == filterName)
    {
      return createFilter(handle);
    }
  }
  return {};
}

AbstractPlugin* FilterList::getPluginById(const FilterHandle::PluginIdType& id) const
{
  if(m_PluginMap.find(id) != m_PluginMap.end())
  {
    return m_PluginMap.at(id)->getPlugin();
  }
  return nullptr;
}

IFilter::UniquePointer FilterList::createFilter(const FilterHandle& handle) const
{
  // Core filter
  if(handle.getPluginId() == Uuid{})
  {
    return nullptr;
  }
  // Plugin filter
  const auto& loader = m_PluginMap.at(handle.getPluginId());
  if(!loader->isLoaded())
  {
    return nullptr;
  }
  return loader->getPlugin()->createFilter(handle.getFilterId());
}

IFilter::UniquePointer FilterList::createFilter(const Uuid& uuid) const
{
  if(m_CoreFiltersMap.count(uuid) > 0)
  {
    return createCoreFilter(uuid);
  }

  auto iter = std::find_if(m_PluginMap.cbegin(), m_PluginMap.cend(), [uuid](const decltype(*m_PluginMap.cbegin())& item) { return item.second->getPlugin()->containsFilterId(uuid); });

  if(iter == m_PluginMap.cend())
  {
    return nullptr;
  }

  if(!iter->second->isLoaded())
  {
    return nullptr;
  }

  return iter->second->getPlugin()->createFilter(uuid);
}

AbstractPlugin* FilterList::getPlugin(const FilterHandle& handle) const
{
  for(const auto& iter : m_PluginMap)
  {
    if(iter.second->getPlugin()->getId() == handle.getPluginId())
    {
      return iter.second->getPlugin();
    }
  }
  return nullptr;
}

bool FilterList::addPlugin(const std::shared_ptr<PluginLoader>& loader)
{
  if(!loader->isLoaded())
  {
    return false;
  }
  AbstractPlugin* plugin = loader->getPlugin();
  Uuid pluginUuid = plugin->getId();
  if(m_PluginMap.count(pluginUuid) > 0)
  {
    throw std::runtime_error(fmt::format("Attempted to add plugin \"{}\" with uuid \"{}\", but plugin \"{}\" already exists with that uuid", plugin->getName(), pluginUuid.str(),
                                         m_PluginMap[pluginUuid]->getPlugin()->getName()));
  }
  auto pluginHandles = plugin->getFilterHandles();
  m_FilterHandles.merge(pluginHandles);
  m_PluginMap[pluginUuid] = loader;
  return true;
}

bool FilterList::addPlugin(const std::string& path)
{
  return addPlugin(std::make_shared<PluginLoader>(path));
}

const std::unordered_set<FilterHandle>& FilterList::getFilterHandles() const
{
  return m_FilterHandles;
}

std::unordered_set<AbstractPlugin*> FilterList::getLoadedPlugins() const
{
  std::unordered_set<AbstractPlugin*> plugins;
  for(auto& iter : m_PluginMap)
  {
    if(!iter.second->isLoaded())
    {
      continue;
    }
    plugins.insert(iter.second->getPlugin());
  }
  return plugins;
}
#if 0
void FilterList::addCoreFilter(FilterCreationFunc func)
{
  IFilter::UniquePointer filter = func();
  if(filter == nullptr)
  {
    throw std::runtime_error("Failed to instantiate core filter");
  }
  Uuid uuid = filter->uuid();
  if(m_CoreFiltersMap.count(uuid) > 0)
  {
    IFilter::UniquePointer existingFilter = m_CoreFiltersMap[uuid]();
    throw std::runtime_error(
        fmt::format("Attempted to add core filter \"{}\" with uuid \"{}\", but core filter \"{}\" already exists with that uuid", filter->name(), uuid.str(), existingFilter->name()));
  }
  m_CoreFiltersMap[uuid] = func;
  m_FilterHandles.insert(FilterHandle(filter->humanName(), uuid, {}));
}

IFilter::UniquePointer FilterList::createCoreFilter(const FilterHandle::FilterIdType& filterId) const
{
  if(m_CoreFiltersMap.find(filterId) == m_CoreFiltersMap.end())
  {
    return nullptr;
  }
  return m_CoreFiltersMap.at(filterId)();
}
#endif
