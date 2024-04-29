#include "SimplnxCore/Filters/FindDifferencesMapFilter.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::FindDifferencesMapFilter: Instantiate Filter", "[FindDifferencesMapFilter]")
{
  FindDifferencesMapFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataPath firstInputPath;
  DataPath secondInputPath;
  DataPath createdArrayPath;

  args.insertOrAssign(FindDifferencesMapFilter::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  args.insertOrAssign(FindDifferencesMapFilter::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));
  args.insertOrAssign(FindDifferencesMapFilter::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(!executeResult.result.valid());
}

TEST_CASE("SimplnxCore::FindDifferencesMapFilter: Test Algorithm", "[FindDifferencesMapFilter]")
{
  FindDifferencesMapFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath firstInputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath secondInputPath({k_SmallIN100, k_EbsdScanData, "FeatureIds"});
  DataPath createdArrayPath({k_SmallIN100, k_EbsdScanData, "Created Array"});

  args.insertOrAssign(FindDifferencesMapFilter::k_FirstInputArrayPath_Key, std::make_any<DataPath>(firstInputPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(FindDifferencesMapFilter::k_SecondInputArrayPath_Key, std::make_any<DataPath>(secondInputPath));
  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(!preflightResult.outputActions.valid());
  }

  args.insertOrAssign(FindDifferencesMapFilter::k_DifferenceMapArrayPath_Key, std::make_any<DataPath>(createdArrayPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());
  }
}
