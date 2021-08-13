#include "Application.hpp"

#include <filesystem>
#include <iostream>

#ifdef __linux__
#include <limits.h>
#include <unistd.h>
#elif _WIN32
#include <Windows.h>
#endif

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Plugin/PluginLoader.hpp"

using namespace complex;

#ifdef __linux__
std::filesystem::path findCurrentPath()
{
  char pBuf[256];
  size_t len = sizeof(pBuf);
  int32_t bytes = MIN(readlink("/proc/self/exe", pBuf, len), len - 1);
  if(bytes >= 0)
    pBuf[bytes] = '\0';
  return std::filesystem::path(pBuf);
}
#elif _WIN32
std::filesystem::path findCurrentPath()
{
  char pBuf[256];
  size_t len = sizeof(pBuf);
  int32_t bytes = GetModuleFileName(NULL, pBuf, len);
  return std::filesystem::path(pBuf);
}
#else
std::filesystem::path findCurrentPath()
{
  return std::filesystem::path();
}
}
#endif

Application* Application::s_Instance = nullptr;

Application::Application()
: m_FilterList(new FilterList())
{
  initialize();
}

Application::Application(int argc, char** argv)
: m_FilterList(new FilterList())
{
  initialize();
}

void Application::initialize()
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

void Application::loadPlugins(const std::filesystem::path& pluginDir)
{
  for(const auto& entry : std::filesystem::directory_iterator(pluginDir))
  {
    auto path = std::filesystem::path(entry.path());
    std::string extension = path.extension().string();
    if(extension == ".complex")
    {
      loadPlugin(path.string());
    }
  }
}

// RestServer* Application::startRestServer(int32_t port)
//{
//  return nullptr;
//}

FilterList* Application::getFilterList() const
{
  return m_FilterList.get();
}

JsonPipelineBuilder* Application::getPipelineBuilder() const
{
  return nullptr;
}

void Application::loadPlugin(const std::string& path)
{
  auto pluginLoader = std::make_shared<PluginLoader>(path);
  getFilterList()->addPlugin(pluginLoader);
}
