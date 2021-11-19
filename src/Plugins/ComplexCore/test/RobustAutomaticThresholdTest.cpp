#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RobustAutomaticThreshold.hpp"

using namespace complex;
using namespace ComplexUnitTest::Constants;

TEST_CASE("RobustAutomaticThreshold: Instantiate Filter", "[RobustAutomaticThreshold]")
{
  RobustAutomaticThreshold filter;
  DataStructure dataGraph;
  Arguments args;

  DataPath inputPath;
  DataPath gradientMagnitudePath;
  DataPath createdArrayPath;

  args.insertOrAssign(RobustAutomaticThreshold::k_InputArrayPath, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RobustAutomaticThreshold::k_GradientMagnitudePath, std::make_any<DataPath>(gradientMagnitudePath));
  args.insertOrAssign(RobustAutomaticThreshold::k_ArrayCreationPath, std::make_any<DataPath>(createdArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid() == false);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid() == false);
}

TEST_CASE("RobustAutomaticThreshold: Missing/Empty DataPaths", "[RobustAutomaticThreshold]")
{
  RobustAutomaticThreshold filter;
  DataStructure dataGraph = ComplexUnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  DataPath createdArrayPath({k_SmallIN100, k_EbsdScanData, "Created Array"});

  args.insertOrAssign(RobustAutomaticThreshold::k_InputArrayPath, std::make_any<DataPath>(inputPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid() == false);
  }

  args.insertOrAssign(RobustAutomaticThreshold::k_GradientMagnitudePath, std::make_any<DataPath>(gradientMagnitudePath));
  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid() == false);
  }

  args.insertOrAssign(RobustAutomaticThreshold::k_ArrayCreationPath, std::make_any<DataPath>(createdArrayPath));
  {
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid() == true);
  }
}

TEST_CASE("RobustAutomaticThreshold: Test Algorithm", "[RobustAutomaticThreshold]")
{
  RobustAutomaticThreshold filter;
  DataStructure dataGraph = ComplexUnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});
  DataPath createdArrayPath({k_SmallIN100, k_EbsdScanData, "Created Array"});

  args.insertOrAssign(RobustAutomaticThreshold::k_InputArrayPath, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RobustAutomaticThreshold::k_GradientMagnitudePath, std::make_any<DataPath>(gradientMagnitudePath));
  args.insertOrAssign(RobustAutomaticThreshold::k_ArrayCreationPath, std::make_any<DataPath>(createdArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}
