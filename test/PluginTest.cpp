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

namespace PluginTest
{
namespace UnitTest
{
const FilterHandle k_TestFilterHandle(Uuid::FromString("5502c3f7-37a8-4a86-b003-1c856be02491").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_TestFilter2Handle(Uuid::FromString("ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854e").value());
} // namespace UnitTest
} // namespace PluginTest

TEST_CASE("Test Loading Plugins")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath);

  auto filterList = app.getFilterList();
  auto filterHandles = filterList->getFilterHandles();
  auto plugins = filterList->getLoadedPlugins();

  REQUIRE(plugins.size() == 3);
  REQUIRE(filterHandles.size() >= 2);

  DataStructure ds;
  {

    IFilter::UniquePointer filter = filterList->createFilter(PluginTest::UnitTest::k_TestFilterHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter(PluginTest::UnitTest::k_TestFilter2Handle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
  }
}

void initApplication()
{
  Application* app = new Application();
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
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
    IFilter::UniquePointer filter = filterList->createFilter(PluginTest::UnitTest::k_TestFilterHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter(PluginTest::UnitTest::k_TestFilter2Handle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
  }

  delete Application::Instance();
  REQUIRE(Application::Instance() == nullptr);
}
