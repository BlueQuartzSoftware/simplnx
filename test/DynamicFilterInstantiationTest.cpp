#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

/**
 * @brief Test every currently compiled filters instantiation.
 */
TEST_CASE("Filter List Instantiation")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = nx::core::Application::Instance()->getFilterList();
  REQUIRE(filterList != nullptr);

  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  nx::core::DataStructure dataStructure;

  for(const auto& handle : handles)
  {
    auto filter = filterList->createFilter(handle);
    REQUIRE(filter != nullptr);

    UNSCOPED_INFO(fmt::format("Filter List Instantiation::{} Instantiation. [simplnx][{}]", filter->className(), filter->name()));

    nx::core::Arguments args;
    dataStructure.clear();

    auto params = filter->parameters();
    for(const auto& [name, param] : params)
    {
      args.insert(name, param->defaultValue());
    }

    REQUIRE(!filter->defaultTags().empty());

    auto preflightResult = filter->preflight(dataStructure, args);
    REQUIRE((preflightResult.outputActions.valid() || preflightResult.outputActions.invalid())); // Preflight exists
  }
}
