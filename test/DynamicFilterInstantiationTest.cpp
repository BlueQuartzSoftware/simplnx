#include "complex/Core/Application.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

#include <catch2/catch.hpp>

/**
 * @brief Test every currently compiled filters instantiation.
 */
TEST_CASE("Filter List Instantiation")
{
  std::shared_ptr<complex::UnitTest::make_shared_enabler> app = std::make_shared<complex::UnitTest::make_shared_enabler>();
  app->loadPlugins(complex::unit_test::k_BuildDir.view(), true);
  auto* filterList = complex::Application::Instance()->getFilterList();
  REQUIRE(filterList != nullptr);

  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  complex::DataStructure dataStructure;

  for(const auto& handle : handles)
  {
    auto filter = filterList->createFilter(handle);
    REQUIRE(filter != nullptr);

    SECTION(("Filter List Instantiation::" + filter->className() + ": Instantiation"), ("[complex][" + filter->name() + "]"))
    {
      complex::Arguments args;
      dataStructure.clear();

      auto params = filter->parameters();
      for(const auto& [name, param] : params)
      {
        args.insert(name, param->defaultValue());
      }

      if(filter->defaultTags().empty())
      {
        REQUIRE(!filter->defaultTags().empty());
      }

      auto preflightResult = filter->preflight(dataStructure, args);
      REQUIRE((preflightResult.outputActions.valid() || preflightResult.outputActions.invalid())); // Preflight exists
    }
  }
}
