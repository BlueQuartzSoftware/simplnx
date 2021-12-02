#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindDifferencesMap.hpp"

using namespace complex;
using namespace complex::Constants;

TEST_CASE("FindDifferencesMap: Instantiate Filter", "[FindDifferencesMap]")
{
  FindDifferencesMap filter;
  DataStructure dataGraph;
  Arguments args;

  DataPath firstInputPath;
  DataPath secondInputPath;
  DataPath createdArrayPath;

  args.insertOrAssign(FindDifferencesMap::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  args.insertOrAssign(FindDifferencesMap::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));
  args.insertOrAssign(FindDifferencesMap::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(!executeResult.result.valid());
}

TEST_CASE("FindDifferencesMap: Test Algorithm", "[FindDifferencesMap]")
{
  FindDifferencesMap filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath firstInputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath secondInputPath({k_SmallIN100, k_EbsdScanData, "FeatureIds"});
  DataPath createdArrayPath({k_SmallIN100, k_EbsdScanData, "Created Array"});

  args.insertOrAssign(FindDifferencesMap::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(FindDifferencesMap::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));
  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(FindDifferencesMap::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());
  }
}
