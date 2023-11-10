#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <sstream>
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
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());

  auto* filterListPtr = Application::Instance()->getFilterList();
  const auto& filterHandles = filterListPtr->getFilterHandles();
  auto plugins = filterListPtr->getLoadedPlugins();

  if(plugins.size() != COMPLEX_PLUGIN_COUNT)
  {
    std::cout << "Incorrect number of plugins were loaded.\n"
              << "Expected: " << COMPLEX_PLUGIN_COUNT << "\nLoaded: " << plugins.size() << "\nLoaded Plugins are:\n";
    for(auto const& plugin : plugins)
    {
      std::cout << plugin->getName() << "\n";
    }
  }

  REQUIRE(plugins.size() == COMPLEX_PLUGIN_COUNT);
  REQUIRE(filterHandles.size() >= 2);

  DataStructure dataStructure;
  {

    IFilter::UniquePointer filter = filterListPtr->createFilter(k_TestFilterHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(dataStructure, {});
  }
  {
    IFilter::UniquePointer filter2 = filterListPtr->createFilter(k_Test2FilterHandle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(dataStructure, {});
  }
}

TEST_CASE("Test Singleton")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());

  REQUIRE(app != nullptr);

  auto* filterListPtr = app->getFilterList();
  const auto& filterHandles = filterListPtr->getFilterHandles();
  auto plugins = filterListPtr->getLoadedPlugins();

  // Check plugins were loaded
  if(plugins.size() != COMPLEX_PLUGIN_COUNT)
  {
    std::cout << "Incorrect number of plugins were loaded.\n"
              << "Expected: " << COMPLEX_PLUGIN_COUNT << "\nLoaded: " << plugins.size() << "\nLoaded Plugins are:\n";
    for(auto const& plugin : plugins)
    {
      std::cout << plugin->getName() << "\n";
    }
  }
  REQUIRE(plugins.size() == COMPLEX_PLUGIN_COUNT);

  // Check filters loaded
  REQUIRE(filterHandles.size() >= 2);

  // Create and execute filters
  DataStructure dataStructure;
  {
    IFilter::UniquePointer filter = filterListPtr->createFilter(k_TestFilterHandle);
    REQUIRE(filter != nullptr);
    REQUIRE(filter->humanName() == "Test Filter");
    filter->execute(dataStructure, {});
  }
  {
    IFilter::UniquePointer filter2 = filterListPtr->createFilter(k_Test2FilterHandle);
    REQUIRE(filter2 != nullptr);
    REQUIRE(filter2->humanName() == "Test Filter 2");
    filter2->execute(dataStructure, {});
  }

  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);
}

TEST_CASE("Test Filter Help Text")
{
  auto appPtr = Application::GetOrCreateInstance();
  appPtr->loadPlugins(unit_test::k_BuildDir.view());
  REQUIRE(appPtr != nullptr);

  appPtr->loadPlugins(unit_test::k_BuildDir.view());
  auto* filterListPtr = Application::Instance()->getFilterList();
  const auto pluginListPtr = Application::Instance()->getPluginList();

  std::stringstream output;

  // Loop on each Plugin
  for(const auto& plugin : pluginListPtr)
  {
    const std::string plugName = plugin->getName();

    const auto& pluginFilterHandles = plugin->getFilterHandles();

    // Loop on each Filter
    for(const auto& filterHandle : pluginFilterHandles)
    {
      const std::string filterClassName = filterHandle.getClassName();
      IFilter::UniquePointer filter = filterListPtr->createFilter(filterHandle);

      const auto& parameters = filter->parameters();
      // Loop over each Parameter
      for(const auto& parameter : parameters)
      {
        auto const& paramValue = parameter.second;
        if(paramValue->helpText().empty())
        {
          output << plugName << "->" << filter->name() << "->" << paramValue->name() << ": Human Name: '" << paramValue->humanName() << "' The Help Text is empty\n";
        }

        for(const auto& letter : paramValue->name())
        {
          if(::isupper(letter) != 0)
          {
            output << plugName << "->" << filter->name() << "->" << paramValue->name() << ". This parameter key has CAPITAL Letters. All parameter keys should be 'lower_snake_case' style\n";
            break;
          }
        }
      }
    }
  }

  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);

  if(!output.str().empty())
  {
    std::cout << output.str();
  }
  REQUIRE(output.str().empty() == true);
}
