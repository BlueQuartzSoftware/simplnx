#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Filter/IFilter.hpp"

using namespace complex;

TEST_CASE("Create Core Filter")
{
  Application app;
  auto filterList = app.getFilterList();
  REQUIRE(filterList != nullptr);

  // Only core filters should exists since plugins were not loaded
  auto handles = filterList->getFilterHandles();
  REQUIRE(handles.size() > 0);

  for(auto handle : handles)
  {
    auto coreFilter = filterList->createFilter(handle);
    REQUIRE(coreFilter != nullptr);
    delete coreFilter;
  }
}
