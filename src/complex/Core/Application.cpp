#include "Application.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

#if defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include <fmt/core.h>

#include "complex/Core/Application.hpp"
#include "complex/Filter/FilterList.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Plugin/PluginLoader.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrDataFactoryManager.hpp"

using namespace complex;

namespace
{
std::filesystem::path findCurrentPath()
{
#if defined(__linux__)
  std::vector<char> buffer(PATH_MAX + 1);
  ssize_t bytesWritten = readlink("/proc/self/exe", buffer.data(), buffer.size());
  if(bytesWritten < 0)
  {
    throw std::runtime_error("Failed to get executable path");
  }
  if(bytesWritten >= buffer.size())
  {
    throw std::runtime_error("Failed to get executable path. Path too long for buffer.");
  }
  buffer[bytesWritten] = '\0';
  return std::filesystem::path(buffer.data());
#elif defined(_WIN32)
  std::vector<WCHAR> buffer(MAX_PATH + 1);
  DWORD bytesWritten = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if(bytesWritten == 0)
  {
    throw std::runtime_error("Failed to get executable path");
  }
  if(bytesWritten >= static_cast<DWORD>(buffer.size()))
  {
    throw std::runtime_error("Failed to get executable path. Path too long for buffer.");
  }
  return std::filesystem::path(buffer.data());
#elif defined(__APPLE__)
  std::vector<char> buffer(1024 + 1);
  uint32 size = static_cast<uint32>(buffer.size());
  int result = _NSGetExecutablePath(buffer.data(), &size);
  if(result != 0)
  {
    throw std::runtime_error("Failed to get executable path. Path too long buffer.");
  }
  return std::filesystem::path(buffer.data());
#else
  static_assert(false, "Unsupported platform for findCurrentPath()");
#endif
}
} // namespace

Application* Application::s_Instance = nullptr;

Application::Application()
: m_FilterList(std::make_unique<FilterList>())
, m_DataReader(std::make_unique<H5::DataFactoryManager>())
, m_ZarrFactoryManager(std::make_unique<Zarr::DataFactoryManager>())
{
  assignInstance();
}

Application::Application(int argc, char** argv)
: Application()
{
  assignInstance();
}

void Application::assignInstance()
{
  s_Instance = this;
  m_CurrentPath = findCurrentPath();
}

Application::~Application()
{
  if(Instance() == this)
  {
    s_Instance = nullptr;
  }
}

Application* Application::Instance()
{
  return s_Instance;
}

std::filesystem::path Application::getCurrentPath() const
{
  return m_CurrentPath;
}

std::filesystem::path Application::getCurrentDir() const
{
  return m_CurrentPath.parent_path();
}

void Application::loadPlugins(const std::filesystem::path& pluginDir, bool verbose)
{
  if(verbose)
  {
    fmt::print("Loading Plugins from {}\n", pluginDir.string());
  }
  for(const auto& entry : std::filesystem::directory_iterator(pluginDir))
  {
    std::filesystem::path path = entry.path();
    std::string extension = path.extension().string();
    if(extension == ".complex")
    {
      loadPlugin(path, verbose);
    }
  }
}

FilterList* Application::getFilterList() const
{
  return m_FilterList.get();
}

std::unordered_set<AbstractPlugin*> Application::getPluginList() const
{
  return m_FilterList->getLoadedPlugins();
}

const AbstractPlugin* Application::getPlugin(const Uuid& uuid) const
{
  std::unordered_set<AbstractPlugin*> plugins = m_FilterList->getLoadedPlugins();
  for(const auto* plugin : plugins)
  {
    if(plugin->getId() == uuid)
    {
      return plugin;
    }
  }
  return nullptr;
}

JsonPipelineBuilder* Application::getPipelineBuilder() const
{
  return nullptr;
}

H5::DataFactoryManager* Application::getH5FactoryManager() const
{
  return m_DataReader.get();
}

Zarr::DataFactoryManager* Application::getZarrFactoryManager() const
{
  return m_ZarrFactoryManager.get();
}

void Application::loadPlugin(const std::filesystem::path& path, bool verbose)
{
  if(verbose)
  {
    fmt::print("Loading Plugin: {}\n", path.string());
  }
  auto pluginLoader = std::make_shared<PluginLoader>(path);
  getFilterList()->addPlugin(pluginLoader);

  auto plugin = pluginLoader->getPlugin();
  if((plugin != nullptr) && (m_DataReader != nullptr))
  {
    auto factories = plugin->getDataFactories();
    for(auto factory : factories)
    {
      m_DataReader->addFactory(factory);
    }
  }
}
