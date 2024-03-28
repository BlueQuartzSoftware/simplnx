
#include "SimplnxCore/Filters/RobustAutomaticThreshold.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::RobustAutomaticThreshold: Missing/Empty DataPaths", "[RobustAutomaticThreshold]")
{
  RobustAutomaticThreshold filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThreshold::k_InputArrayPath_Key, std::make_any<DataPath>(inputPath));

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThreshold::k_GradientMagnitudePath_Key, std::make_any<DataPath>(gradientMagnitudePath));

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }
}

TEST_CASE("SimplnxCore::RobustAutomaticThreshold: Test Algorithm", "[RobustAutomaticThreshold]")
{
  RobustAutomaticThreshold filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  args.insertOrAssign(RobustAutomaticThreshold::k_InputArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RobustAutomaticThreshold::k_GradientMagnitudePath_Key, std::make_any<DataPath>(gradientMagnitudePath));
  args.insertOrAssign(RobustAutomaticThreshold::k_ArrayCreationName_Key, std::make_any<std::string>("Created Array"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}
