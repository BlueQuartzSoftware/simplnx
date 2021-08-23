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

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Plugin/PluginLoader.hpp"

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
  uint32_t size = static_cast<uint32_t>(buffer.size());
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
