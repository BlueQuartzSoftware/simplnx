#include "Application.hpp"

#include <filesystem>

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

using namespace complex;

Application* Application::s_Instance = nullptr;

Application::Application()
: m_FilterList(new FilterList())
{
  s_Instance = this;
}

Application::Application(int argc, char** argv)
: m_FilterList(new FilterList())
{
  s_Instance = this;
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

void Application::loadPlugins(const std::string& pluginDir)
{
  for(const auto& entry : std::filesystem::directory_iterator(pluginDir))
  {
    auto path = std::filesystem::path(entry.path());
    std::string filename = path.filename().string();
    if(filename.find("Plugin.") != std::string::npos)
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
  getFilterList()->addPlugin(path);
}
