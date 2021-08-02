#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "Test2Plugin.hpp"
#include "TestPlugin.hpp"
#include "Test2Filter.hpp"
#include "TestFilter.hpp"

using namespace complex;

namespace PluginDir
{
inline const std::string InstallPrefixDir = R"(@CMAKE_INSTALL_PREFIX@)";
inline const std::string InstallBinDir("@CMAKE_INSTALL_BINDIR@");

inline const std::filesystem::path Path = std::filesystem::path(InstallPrefixDir) / InstallBinDir;
} // namespace PluginDir

namespace
{
FilterHandle testHandle("Test Filter", complex::Uuid::FromString("4a8fb14c-2d42-45c7-86cd-e7d7bb09967f").value(), TestPlugin::ID);
FilterHandle test2Handle("Test 2 Filter", complex::Uuid::FromString("6060a977-31f5-442c-be83-aa3fc4f8268c").value(), Test2Plugin::ID);
} // namespace

void testLoadingPlugins()
{
  std::cout << "testLoadingPlugins()" << std::endl;

  Application app;
  app.loadPlugins(PluginDir::Path.string());

  auto filterList = app.getFilterList();
  auto filterHandles = filterList->getFilterHandles();

  std::cout << " Loaded Plugins:\n";
  for(auto& plugin : filterList->getLoadedPlugins())
  {
    std::cout << "  " << plugin->getName() << "\n";
    for(auto& handle : plugin->getFilterHandles())
    {
      std::cout << "    " << handle.getFilterName() << "\n";
    }
  }
  std::cout << std::endl;

  if(filterHandles.size() != 2)
  {
    throw std::runtime_error("");
  }

  auto filter = filterList->createFilter(testHandle);
  if(filter == nullptr)
  {
    throw std::runtime_error("");
  }
  if(filter->humanName() != "Test Filter")
  {
    throw std::runtime_error("");
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(test2Handle);
  if(filter2 == nullptr)
  {
    throw std::runtime_error("");
  }
  if(filter2->humanName() != "Test 2 Filter")
  {
    throw std::runtime_error("");
  }
  filter2->execute(DataStructure(), {});
  std::cout << "\n" << std::endl;
}

void testSingleton()
{
  std::cout << "testApplicationSingleton" << std::endl;

  auto filterList = Application::Instance()->getFilterList();
  auto filterHandles = filterList->getFilterHandles();

  std::cout << " Loaded Plugins:\n";
  for(auto& plugin : filterList->getLoadedPlugins())
  {
    std::cout << "  " << plugin->getName() << "\n";
    for(auto& handle : plugin->getFilterHandles())
    {
      std::cout << "    " << handle.getFilterName() << "\n";
    }
  }
  std::cout << std::endl;

  // Check filters loaded
  if(filterHandles.size() != 2)
  {
    throw std::runtime_error("");
  }

  // Create and execute filters
  auto filter = filterList->createFilter(testHandle);
  if(filter == nullptr)
  {
    throw std::runtime_error("");
  }
  if(filter->humanName() != "Test Filter")
  {
    throw std::runtime_error("");
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(test2Handle);
  if(filter2 == nullptr)
  {
    throw std::runtime_error("");
  }
  if(filter2->humanName() != "Test 2 Filter")
  {
    throw std::runtime_error("");
  }
  filter2->execute(DataStructure(), {});

  std::cout << std::endl;
}

int main(int argc, char** argv)
{
  std::cout << "PluginTest" << std::endl;

  testLoadingPlugins();

  Application app;
  app.loadPlugins(PluginDir::Path.string());
  testSingleton();
  return 0;
}
