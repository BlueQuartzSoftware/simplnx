#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "catch.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

//#include "complex/Test/PluginDir.hpp"

using namespace complex;

namespace
{
inline const std::string PluginDir = "test/Plugins";
FilterHandle testHandle("Test Filter", "1", "05cc618b-781f-4ac0-b9ac-43f26ce1854f");
FilterHandle test2Handle("Test2 Filter", "1", "05cc618b-781f-4ac0-b9ac-43f26ce1854e");
} // namespace

TEST_CASE("Test Loading Plugins")
{
  std::cout << "testLoadingPlugins()" << std::endl;

  Application app;
  //app.loadPlugins(PluginDir::Path.string());
  app.loadPlugins(::PluginDir);

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
  if(filter->name() != "Test Filter")
  {
    throw std::runtime_error("");
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(test2Handle);
  if(filter2 == nullptr)
  {
    throw std::runtime_error("");
  }
  if(filter2->name() != "Test Filter 2")
  {
    throw std::runtime_error("");
  }
  filter2->execute(DataStructure(), {});
  std::cout << "\n" << std::endl;
}

#if 0
TEST_CASE("Test Singleton")
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
  if(filter->name() != "Test Filter")
  {
    throw std::runtime_error("");
  }
  filter->execute(DataStructure(), {});

  auto filter2 = filterList->createFilter(test2Handle);
  if(filter2 == nullptr)
  {
    throw std::runtime_error("");
  }
  if(filter2->name() != "Test Filter 2")
  {
    throw std::runtime_error("");
  }
  filter2->execute(DataStructure(), {});

  std::cout << std::endl;
}
#endif
