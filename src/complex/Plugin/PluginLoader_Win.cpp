#include <Windows.h>

#include "complex/Core/FilterHandle.hpp"
#include "complex/Plugin/PluginLoader.hpp"

using namespace complex;

PluginLoader::PluginLoader(const std::string& path)
: m_Path(path)
, m_Plugin(nullptr)
{
  loadPlugin();
}

PluginLoader::~PluginLoader()
{
  unloadPlugin();
}

void PluginLoader::loadPlugin()
{
  m_Handle = LoadLibrary(m_Path.c_str());
  if(m_Handle)
  {
    InitPluginFunc _initPlugin = (InitPluginFunc)GetProcAddress((HINSTANCE)m_Handle, "initPlugin");
    if(_initPlugin)
    {
      m_Plugin = std::shared_ptr<AbstractPlugin>(_initPlugin());
    }
    else
    {
      unloadPlugin();
    }
  }
}

void PluginLoader::unloadPlugin()
{
  if(!m_Handle)
  {
    return;
  }

  m_Plugin = nullptr;
  FreeLibrary((HINSTANCE)m_Handle);
  m_Handle = nullptr;
}

bool PluginLoader::isLoaded() const
{
  return nullptr != getPlugin();
}

AbstractPlugin* PluginLoader::getPlugin() const
{
  return m_Plugin.get();
}
