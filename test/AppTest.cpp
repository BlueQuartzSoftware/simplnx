#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>

#include "complex/Core/Application.hpp"
#include "complex/Filter/IFilter.hpp"

using namespace complex;

namespace PluginDir
{
inline const std::string InstallPrefixDir = R"(@CMAKE_INSTALL_PREFIX@)";
inline const std::string InstallBinDir("@CMAKE_INSTALL_BINDIR@");

inline const std::filesystem::path Path = std::filesystem::path(InstallPrefixDir) / InstallBinDir;
} // namespace PluginDir

void createApp()
{
  auto app = new Application();
  std::cout << "  Application Created" << std::endl;
  app->loadPlugins(PluginDir::Path.string());
  std::cout << "  Plugins Loaded\n" << std::endl;
}

void testFilterList()
{
  std::cout << "testFilterList()" << std::endl;

  auto filterList = Application::Instance()->getFilterList();
  auto filterHandles = filterList->getFilterHandles();

  // Create filters
  for(auto& filterHandle : filterHandles)
  {
    auto filter = filterList->createFilter(filterHandle);
    if(filter == nullptr)
    {
      throw std::runtime_error("");
    }
    std::cout << "  " << filter->humanName() << "\n";
  }
  std::cout << std::endl;
}

void testDeletingApp()
{
  std::cout << "testDeletingApp()" << std::endl;

  std::cout << "  Deleting Application::Instance()" << std::endl;
  delete Application::Instance();
  if(Application::Instance() != nullptr)
  {
    throw std::runtime_error("");
  }
  std::cout << "  Application::Instance() deleted" << std::endl;

  std::cout << std::endl;
}

int main(int argc, char** argv)
{
  std::cout << "AppTest" << std::endl;

  createApp();
  testFilterList();
  // testPipelineBuilder();
  // testRestServer();
  testDeletingApp();

  return 0;
}
