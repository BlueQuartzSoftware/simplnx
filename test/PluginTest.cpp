#include <string>

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

#include "complex/unit_test/complex_test_dirs.h"

using namespace complex;
namespace fs = std::filesystem;

TEST_CASE("Test Loading Plugins")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/",complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir );
  app.loadPlugins(pluginPath);

  auto filterList = app.getFilterList();
  auto filterHandles = filterList->getFilterHandles();
  auto plugins = filterList->getLoadedPlugins();

  REQUIRE(plugins.size() == 3);
  REQUIRE(filterHandles.size() >= 2);

  DataStructure ds;
  {
    IFilter::UniquePointer filter = filterList->createFilter("TestFilter");
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter("Test2Filter");
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
  }
}

void initApplication()
{
  Application* app = new Application();
  fs::path pluginPath = fmt::format("{}/{}/",complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir );
  app->loadPlugins(pluginPath);
}

TEST_CASE("Test Singleton")
{
  initApplication();
  REQUIRE(Application::Instance() != nullptr);

  auto filterList = Application::Instance()->getFilterList();
  auto filterHandles = filterList->getFilterHandles();
  auto plugins = filterList->getLoadedPlugins();

  // Check plugins were loaded
  REQUIRE(plugins.size() == 3);

  // Check filters loaded
  REQUIRE(filterHandles.size() >= 2);

  // Create and execute filters
  DataStructure ds;
  {
    IFilter::UniquePointer filter = filterList->createFilter("TestFilter");
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter("Test2Filter");
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
  }

  delete Application::Instance();
  REQUIRE(Application::Instance() == nullptr);
}
