#include "catch2/catch.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Pipeline/Pipeline.hpp"

#include "complex/unit_test/complex_test_dirs.hpp"

#include <nlohmann/json.hpp>

using namespace complex;

TEST_CASE("Save Filters To Json")
{
  Application app;
  app.loadPlugins(unit_test::k_BuildDir.view());
  auto* filterList = app.getFilterList();
  REQUIRE(filterList != nullptr);

  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  for(const auto& handle : handles)
  {
    auto coreFilter = filterList->createFilter(handle);
    REQUIRE(coreFilter != nullptr);

    Arguments args;
    auto params = coreFilter->parameters();
    for(const auto& [name, param] : params)
    {
      args.insert(name, param->defaultValue());
    }

    try
    {
      auto json = coreFilter->toJson(args);
    } catch(...)
    {
      // toJson() threw an exception
      INFO(fmt::format("Filter '{}' did not serialize to json properly!", coreFilter->humanName()))
      REQUIRE(true == false);
    }
  }
}

TEST_CASE("Save Pipeline To Json")
{
  Application app;
  app.loadPlugins(unit_test::k_BuildDir.view());
  auto* filterList = app.getFilterList();
  REQUIRE(filterList != nullptr);

  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

  Pipeline pipeline;
  for(const auto& handle : handles)
  {
    auto coreFilter = filterList->createFilter(handle);
    REQUIRE(coreFilter != nullptr);

    Arguments args;
    auto params = coreFilter->parameters();
    for(const auto& [name, param] : params)
    {
      args.insert(name, param->defaultValue());
    }

    REQUIRE(pipeline.push_back(handle, args));
  }

  try
  {
    auto json = pipeline.toJson();
  } catch(...)
  {
    // toJson() threw an exception
    INFO(fmt::format("The pipeline containing every filter did not serialize to json properly!"))
    REQUIRE(true == false);
  }
}
