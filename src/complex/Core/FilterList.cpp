#include "FilterList.hpp"

#include <memory>
#include <set>

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/ImportCoreFilters.hpp"
#include "complex/Plugin/PluginLoader.hpp"

using namespace complex;

FilterList::FilterList()
{
  registerCoreFilters();
}

FilterList::~FilterList() = default;

std::vector<complex::FilterHandle> FilterList::search(const std::string& text) const
{
  std::vector<complex::FilterHandle> handles;
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

AbstractPlugin* FilterList::getPluginById(const FilterHandle::PluginIdType& id) const
{
  if(m_PluginMap.find(id) != m_PluginMap.end())
  {
    return m_PluginMap.at(id)->getPlugin();
  }
  return nullptr;
}

complex::IFilter* FilterList::createFilter(const complex::FilterHandle& handle) const
{
  // Core filter
  if(handle.getPluginId() == Uuid{})
  {
    return createCoreFilter(handle.getFilterId());
  }
  // Plugin filter
  const auto& loader = m_PluginMap.at(handle.getPluginId());
  if(!loader->isLoaded())
  {
    return nullptr;
  }
  return loader->getPlugin()->createFilter(handle.getFilterId());
}

complex::AbstractPlugin* FilterList::getPlugin(const complex::FilterHandle& handle) const
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
  auto pluginHandles = loader->getPlugin()->getFilterHandles();
  m_FilterHandles.merge(pluginHandles);
  m_PluginMap[loader->getPlugin()->getId()] = loader;
  return true;
}

bool FilterList::addPlugin(const std::string& path)
{
  return addPlugin(std::make_shared<PluginLoader>(path));
}

std::unordered_set<complex::FilterHandle> FilterList::getFilterHandles() const
{
  return m_FilterHandles;
}

std::unordered_set<complex::AbstractPlugin*> FilterList::getLoadedPlugins() const
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

void FilterList::addCoreFilter(FilterCreationFunc func)
{
  IFilter* filter = func();
  m_CoreFiltersMap[filter->uuid()] = func;
  m_FilterHandles.insert(FilterHandle(filter->name(), filter->uuid(), {}));
  delete filter;
}

IFilter* FilterList::createCoreFilter(const FilterHandle::FilterIdType& filterId) const
{
  if(m_CoreFiltersMap.find(filterId) == m_CoreFiltersMap.end())
  {
    return nullptr;
  }
  return m_CoreFiltersMap.at(filterId)();
}
