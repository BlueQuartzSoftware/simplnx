#include "PluginDir.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filtering/AbstractFilter.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace nx::core;

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
    std::cout << "  " << filter->getName() << "\n";
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
