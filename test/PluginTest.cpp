#include <string>

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"

using namespace complex;

namespace
{
inline const std::string PluginDir = "/test/Plugins";
FilterHandle testHandle("Test Filter", "1", "05cc618b-781f-4ac0-b9ac-43f26ce1854f");
FilterHandle test2Handle("Test2 Filter", "1", "05cc618b-781f-4ac0-b9ac-43f26ce1854e");
} // namespace

std::string getPluginDir(const std::filesystem::path& appDir)
{
#ifdef __APPLE__
return appDir.parent_path().string() + ::PluginDir;
#else
return appDir.string() + ::PluginDir;
#endif
}

TEST_CASE("Test Loading Plugins")
{
  Application app;
  std::filesystem::path pluginPath = getPluginDir(app.getCurrentDir());
  app.loadPlugins(pluginPath);

  auto filterList = app.getFilterList();
  auto filterHandles = filterList->getFilterHandles();

  REQUIRE(filterHandles.size() == 2);

  DataStructure ds;
  {
    IFilter* filter = filterList->createFilter(testHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
    delete filter;
  }
  {
    IFilter* filter2 = filterList->createFilter(test2Handle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
    delete filter2;
  }
}

void initApplication()
{
  Application* app = new Application();
  std::filesystem::path pluginPath = getPluginDir(app->getCurrentDir());
  app->loadPlugins(pluginPath);
}

TEST_CASE("Test Singleton")
{
  initApplication();
  REQUIRE(Application::Instance() != nullptr);

  auto filterList = Application::Instance()->getFilterList();
  auto filterHandles = filterList->getFilterHandles();

  // Check filters loaded
  REQUIRE(filterHandles.size() == 2);

  // Create and execute filters
  DataStructure ds;
  {
    IFilter* filter = filterList->createFilter(testHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(ds, {});
    delete filter;
  }
  {
    IFilter* filter2 = filterList->createFilter(test2Handle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(ds, {});
    delete filter2;
  }

  delete Application::Instance();
  REQUIRE(Application::Instance() == nullptr);
}
