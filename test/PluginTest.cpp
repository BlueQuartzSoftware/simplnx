#include <exception>
#include <iostream>
#include <string>

#include <SIMPL/Core/Application.h>
#include <SIMPL/Core/FilterHandle.h>
#include <SIMPL/DataStructure/DataStructure.h>
#include <SIMPL/Filtering/AbstractFilter.h>
#include <SIMPL/Plugin/AbstractPlugin.h>
#include "Plugin/TestFilter.h"
#include "Plugin/Test2Filter.h"

#include "PluginDir.h"

using namespace SIMPL;

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

  auto filter = filterList->createFilter(filterHandles[1]);
  if(nullptr == filter)
  {
    throw std::exception();
  }
  if(nullptr == dynamic_cast<TestFilter*>(filter))
  {
    throw std::exception();
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(filterHandles[0]);
  if(nullptr == filter2)
  {
    throw std::exception();
  }
  if(nullptr == dynamic_cast<Test2Filter*>(filter2))
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
  auto filter = filterList->createFilter(filterHandles[1]);
  if(nullptr == filter)
  {
    throw std::exception();
  }
  if(nullptr == dynamic_cast<TestFilter*>(filter))
  {
    throw std::exception();
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(filterHandles[0]);
  if(nullptr == filter2)
  {
    throw std::exception();
  }
  if(nullptr == dynamic_cast<Test2Filter*>(filter2))
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
