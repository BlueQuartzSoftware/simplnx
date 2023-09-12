#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace complex;

namespace
{
constexpr Uuid k_TestOnePluginId = *Uuid::FromString("01ff618b-781f-4ac0-b9ac-43f26ce1854f");
constexpr Uuid k_TestFilterId = *Uuid::FromString("5502c3f7-37a8-4a86-b003-1c856be02491");
const FilterHandle k_TestFilterHandle(k_TestFilterId, k_TestOnePluginId);

constexpr Uuid k_TestTwoPluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f33ce1854e");
constexpr Uuid k_Test2FilterId = *Uuid::FromString("ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04");
const FilterHandle k_Test2FilterHandle(k_Test2FilterId, k_TestTwoPluginId);

} // namespace

TEST_CASE("Test Loading Plugins")
{
  Application app;
  app.loadPlugins(unit_test::k_BuildDir.view());

  auto* filterList = Application::Instance()->getFilterList();
  const auto& filterHandles = filterList->getFilterHandles();
  auto plugins = filterList->getLoadedPlugins();

  REQUIRE(plugins.size() == COMPLEX_PLUGIN_COUNT);
  REQUIRE(filterHandles.size() >= 2);

  DataStructure dataStructure;
  {

    IFilter::UniquePointer filter = filterList->createFilter(k_TestFilterHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(dataStructure, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter(k_Test2FilterHandle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(dataStructure, {});
  }
}

TEST_CASE("Test Singleton")
{
  Application* app = new Application();
  app->loadPlugins(unit_test::k_BuildDir.view());

  REQUIRE(app != nullptr);

  auto* filterList = app->getFilterList();
  const auto& filterHandles = filterList->getFilterHandles();
  auto plugins = filterList->getLoadedPlugins();

  // Check plugins were loaded
  REQUIRE(plugins.size() == COMPLEX_PLUGIN_COUNT);

  // Check filters loaded
  REQUIRE(filterHandles.size() >= 2);

  // Create and execute filters
  DataStructure dataStructure;
  {
    IFilter::UniquePointer filter = filterList->createFilter(k_TestFilterHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(dataStructure, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter(k_Test2FilterHandle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(dataStructure, {});
  }

  delete Application::Instance();
  REQUIRE(Application::Instance() == nullptr);
}
