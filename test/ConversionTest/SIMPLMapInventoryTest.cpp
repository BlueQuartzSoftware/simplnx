#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <set>
#include <string>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
/**
 * @brief Checks if a backwards compatibility fixture exists for the given filter
 * in any of the plugin simpl_conversion directories.
 */
bool hasBackwardsCompatFixture(const std::string& filterName)
{
  // Check all plugin directories for a 6_5 fixture
  // Includes both in-tree plugins and known external plugins
  for(const auto& pluginDir : nx::core::unit_test::k_PluginSourceDirs)
  {
    fs::path fixturePath = pluginDir / "test" / "simpl_conversion" / "6_5" / (filterName + ".json");
    if(fs::exists(fixturePath))
    {
      return true;
    }
  }
  return false;
}
} // namespace

/**
 * @brief Prints a complete markdown table of all SIMPL-to-SIMPLNX filter mappings.
 *
 * For each plugin, iterates the SIMPLMapType to list every SIMPL UUID,
 * the corresponding SIMPLNX filter name and UUID, and whether a backwards
 * compatibility test fixture exists.
 *
 * Run with: ctest -R "SIMPL Map Inventory" -V
 */
TEST_CASE("SIMPL Map Inventory: Print Complete Table", "[BackwardsCompatibility][Inventory]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  auto plugins = filterList->getLoadedPlugins();

  std::cout << std::endl;
  std::cout << "| Plugin | Filter Name | SIMPL UUID | SIMPLNX UUID | Has Fixture |" << std::endl;
  std::cout << "|--------|-------------|------------|--------------|-------------|" << std::endl;

  usize totalEntries = 0;
  usize uniqueNxFilters = 0;
  usize withFixture = 0;
  usize withoutFixture = 0;
  std::set<Uuid> seenNxUuids;

  for(const auto* plugin : plugins)
  {
    const std::string pluginName = plugin->getName();
    auto simplMap = plugin->getSimplToSimplnxMap();

    for(const auto& [simplUuid, simplData] : simplMap)
    {
      totalEntries++;

      IFilter::UniquePointer filter = filterList->createFilter(simplData.simplnxUuid);
      std::string filterClassName = filter ? filter->className() : "UNKNOWN";
      std::string nxUuid = simplData.simplnxUuid.str();
      std::string simplUuidStr = simplUuid.str();

      bool isFirstOccurrence = !seenNxUuids.contains(simplData.simplnxUuid);
      seenNxUuids.insert(simplData.simplnxUuid);

      bool hasFixture = false;
      if(filter)
      {
        hasFixture = hasBackwardsCompatFixture(filterClassName);
      }

      if(isFirstOccurrence)
      {
        uniqueNxFilters++;
        if(hasFixture)
        {
          withFixture++;
        }
        else
        {
          withoutFixture++;
        }
      }

      std::cout << fmt::format("| {} | {} | {} | {} | {} |", pluginName, filterClassName, simplUuidStr, nxUuid, hasFixture ? "Yes" : "**NO**") << std::endl;
    }
  }

  std::cout << std::endl;
  std::cout << fmt::format("Total SIMPL->SIMPLNX mappings: {} ({} unique NX filters)", totalEntries, uniqueNxFilters) << std::endl;
  std::cout << fmt::format("With backwards compatibility fixture: {}", withFixture) << std::endl;
  std::cout << fmt::format("Missing fixture: {}", withoutFixture) << std::endl;
  std::cout << std::endl;
}

/**
 * @brief Validates that every entry in every plugin's SIMPLMapType has a
 * corresponding backwards compatibility test fixture in simpl_conversion/6_5/.
 *
 * This test will fail if a new SIMPL->SIMPLNX mapping is added to a plugin
 * but no fixture file is created for it, ensuring backwards compatibility
 * test coverage stays complete.
 */
TEST_CASE("SIMPL Map Inventory: Validate Coverage", "[BackwardsCompatibility][Inventory]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  auto plugins = filterList->getLoadedPlugins();

  std::vector<std::string> missingFixtures;

  // Track unique NX filters (multiple SIMPL UUIDs may map to the same NX filter)
  std::set<Uuid> seenNxUuids;

  for(const auto* plugin : plugins)
  {
    auto simplMap = plugin->getSimplToSimplnxMap();

    for(const auto& [simplUuid, simplData] : simplMap)
    {
      // Only check each NX filter once even if it has multiple SIMPL UUIDs
      if(seenNxUuids.contains(simplData.simplnxUuid))
      {
        continue;
      }
      seenNxUuids.insert(simplData.simplnxUuid);

      IFilter::UniquePointer filter = filterList->createFilter(simplData.simplnxUuid);
      if(filter == nullptr)
      {
        continue;
      }

      const std::string filterClassName = filter->className();

      if(!hasBackwardsCompatFixture(filterClassName))
      {
        missingFixtures.emplace_back(fmt::format("{} ({})", filterClassName, plugin->getName()));
      }
    }
  }

  std::string message = fmt::format("{} filters are missing backwards compatibility fixtures:\n", missingFixtures.size());
  for(const auto& name : missingFixtures)
  {
    message += fmt::format("  - {}\n", name);
  }
  INFO(message);
  CHECK(missingFixtures.empty());
}
