#include <exception>
#include <iostream>
#include <string>

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filtering/AbstractFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

#include "PluginDir.hpp"

using namespace complex;

namespace
{
FilterHandle testHandle("Test Filter", 1, "05cc618b-781f-4ac0-b9ac-43f26ce1854f");
FilterHandle test2Handle("Test2 Filter", 1, "05cc618b-781f-4ac0-b9ac-43f26ce1854e");
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
    throw std::exception();
  }

  auto filter = filterList->createFilter(testHandle);
  if(nullptr == filter)
  {
    throw std::exception();
  }
  if(filter->getName() != "Test Filter")
  {
    throw std::exception();
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(test2Handle);
  if(nullptr == filter2)
  {
    throw std::exception();
  }
  if(filter2->getName() != "Test Filter 2")
  {
    throw std::exception();
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
    throw std::exception();
  }

  // Create and execute filters
  auto filter = filterList->createFilter(testHandle);
  if(nullptr == filter)
  {
    throw std::exception();
  }
  if(filter->getName() != "Test Filter")
  {
    throw std::exception();
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(test2Handle);
  if(nullptr == filter2)
  {
    throw std::exception();
  }
  if(filter2->getName() != "Test Filter 2")
  {
    throw std::exception();
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
