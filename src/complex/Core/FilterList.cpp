#include "FilterList.hpp"

#include <memory>
#include <set>

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
//#include "complex/Filtering/AbstractFilter.hpp"
#include "complex/Plugin/PluginLoader.hpp"

using namespace complex;

FilterList::FilterList()
{
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

std::set<complex::FilterHandle> FilterList::getFilterHandles() const
{
  return m_FilterHandles;
}

std::set<complex::AbstractPlugin*> FilterList::getLoadedPlugins() const
{
  std::set<AbstractPlugin*> plugins;
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
