#include "FilterList.h"

#include <exception>
#include <memory>
#include <set>

#include "Complex/Core/FilterHandle.h"
//#include "Complex/Filtering/AbstractFilter.h"
#include "Complex/Plugin/PluginLoader.h"

using namespace Complex;

FilterList::FilterList()
{
}

FilterList::~FilterList() = default;

std::vector<Complex::FilterHandle> FilterList::search(const std::string& text) const
{
  throw std::exception();
}

Complex::AbstractFilter* FilterList::createFilter(const Complex::FilterHandle& handle) const
{
  auto& loader = m_PluginMap.at(handle.getPluginId());
  if(!loader->isLoaded())
  {
    return nullptr;
  }
  return loader->getPlugin()->createFilter(handle.getFilterId());
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

std::set<Complex::FilterHandle> FilterList::getFilterHandles() const
{
  return m_FilterHandles;
}

std::set<Complex::AbstractPlugin*> FilterList::getLoadedPlugins() const
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
