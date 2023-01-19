#include "ComplexCore/Filters/FindDifferencesMap.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::FindDifferencesMap: Instantiate Filter", "[FindDifferencesMap]")
{
  FindDifferencesMap filter;
  DataStructure dataStructure;
  Arguments args;

  DataPath firstInputPath;
  DataPath secondInputPath;
  DataPath createdArrayPath;

  args.insertOrAssign(FindDifferencesMap::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  args.insertOrAssign(FindDifferencesMap::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));
  args.insertOrAssign(FindDifferencesMap::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(!executeResult.result.valid());
}

TEST_CASE("ComplexCore::FindDifferencesMap: Test Algorithm", "[FindDifferencesMap]")
{
  FindDifferencesMap filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath firstInputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath secondInputPath({k_SmallIN100, k_EbsdScanData, "FeatureIds"});
  DataPath createdArrayPath({k_SmallIN100, k_EbsdScanData, "Created Array"});

  args.insertOrAssign(FindDifferencesMap::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(FindDifferencesMap::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));
  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(FindDifferencesMap::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());
  }
}
