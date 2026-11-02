#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <algorithm>

using namespace nx::core;

TEST_CASE("ImageProcessing::Plugin loads", "[ImageProcessing]")
{
  UnitTest::LoadPlugins();
  const auto plugins = Application::Instance()->getFilterList()->getLoadedPlugins();
  const bool found = std::any_of(plugins.cbegin(), plugins.cend(), [](const AbstractPlugin* plugin) { return plugin->getName() == "ImageProcessing"; });
  REQUIRE(found);
}
