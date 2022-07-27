#include "FilterList.hpp"

#include <memory>
#include <stdexcept>

#include <fmt/core.h>

#include "complex/Core/Application.hpp"
#include "complex/Plugin/PluginLoader.hpp"

using namespace complex;

FilterList::FilterList() = default;

FilterList::~FilterList() = default;

FilterList::FilterContainerType::size_type FilterList::size() const
{
  return getFilterHandles().size();
}

FilterList::SearchContainerType FilterList::search(const std::string& text) const
{
  std::vector<FilterHandle> handles;
  for(const auto& handle : getFilterHandles())
  {
    if(handle.getFilterName().find(text) != std::string::npos || getPlugin(handle)->getName().find(text) != std::string::npos)
    {
      handles.push_back(handle);
    }
  }
  return handles;
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
  // Look to make sure the plugin is available. Unordered_Map.at() will throw exceptions if the key is not found.
  if(m_PluginMap.find(handle.getPluginId()) == m_PluginMap.end())
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
  auto iter = std::find_if(m_PluginMap.cbegin(), m_PluginMap.cend(), [uuid](decltype(*m_PluginMap.cbegin())& item) { return item.second->getPlugin()->containsFilterId(uuid); });

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
    throw std::runtime_error(fmt::format("Attempted to add plugin '{}' with uuid '{}', but plugin '{}' already exists with that uuid", plugin->getName(), pluginUuid.str(),
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

const FilterList::FilterContainerType& FilterList::getFilterHandles() const
{
  return m_FilterHandles;
}

std::unordered_set<AbstractPlugin*> FilterList::getLoadedPlugins() const
{
  std::unordered_set<AbstractPlugin*> plugins;
  for(const auto& iter : m_PluginMap)
  {
    if(!iter.second->isLoaded())
    {
      continue;
    }
    plugins.insert(iter.second->getPlugin());
  }
  return plugins;
}
