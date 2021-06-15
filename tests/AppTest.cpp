#include <exception>
#include <iostream>
#include <string>

#include "Complex/Core/Application.h"
#include "Complex/Filtering/AbstractFilter.h"

#include "PluginDir.h"

using namespace Complex;

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
    if(nullptr == filter)
    {
      throw std::exception();
    }
    std::cout << "  " << filter->getName() << "\n";
  }
  std::cout << std::endl;
}

int main(int argc, char** argv)
{
  std::cout << "AppTest" << std::endl;

  createApp();
  testFilterList();
  // testPipelineBuilder();
  // testRestServer();

  return 0;
}
