#include <string>

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
const fs::path PluginDir = "test/Plugins";
const FilterHandle testHandle(Uuid::FromString("5502c3f7-37a8-4a86-b003-1c856be02491").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle test2Handle(Uuid::FromString("ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854e").value());

fs::path getPluginDir(const fs::path& appDir)
{
#ifdef __APPLE__
  return appDir.parent_path() / ::PluginDir;
#else
  return appDir / ::PluginDir;
#endif
}
} // namespace

TEST_CASE("Test Loading Plugins")
{
  Application app;
  fs::path pluginPath = getPluginDir(app.getCurrentDir());
  app.loadPlugins(pluginPath);

  auto filterList = app.getFilterList();
  auto filterHandles = filterList->getFilterHandles();
  auto plugins = filterList->getLoadedPlugins();

  REQUIRE(plugins.size() == 2);
  REQUIRE(filterHandles.size() >= 2);

  DataStructure ds;
  {
    IFilter::UniquePointer filter = filterList->createFilter(testHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter(test2Handle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
  }
}

void initApplication()
{
  Application* app = new Application();
  fs::path pluginPath = getPluginDir(app->getCurrentDir());
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
  REQUIRE(plugins.size() == 2);

  // Check filters loaded
  REQUIRE(filterHandles.size() >= 2);

  // Create and execute filters
  DataStructure ds;
  {
    IFilter::UniquePointer filter = filterList->createFilter(testHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
  }
  {
    IFilter::UniquePointer filter2 = filterList->createFilter(test2Handle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
  }

  delete Application::Instance();
  REQUIRE(Application::Instance() == nullptr);
}
