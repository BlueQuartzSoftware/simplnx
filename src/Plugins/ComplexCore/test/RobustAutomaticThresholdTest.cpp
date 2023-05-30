
#include "ComplexCore/Filters/RobustAutomaticThreshold.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::RobustAutomaticThreshold: Missing/Empty DataPaths", "[RobustAutomaticThreshold]")
{
  RobustAutomaticThreshold filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThreshold::k_InputArrayPath, std::make_any<DataPath>(inputPath));

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  args.insertOrAssign(RobustAutomaticThreshold::k_GradientMagnitudePath, std::make_any<DataPath>(gradientMagnitudePath));

  // Preflight the filter and check result
  {
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }
}

TEST_CASE("ComplexCore::RobustAutomaticThreshold: Test Algorithm", "[RobustAutomaticThreshold]")
{
  RobustAutomaticThreshold filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath inputPath({k_SmallIN100, k_EbsdScanData, "Phases"});
  DataPath gradientMagnitudePath({k_SmallIN100, k_EbsdScanData, k_ConfidenceIndex});

  args.insertOrAssign(RobustAutomaticThreshold::k_InputArrayPath, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RobustAutomaticThreshold::k_GradientMagnitudePath, std::make_any<DataPath>(gradientMagnitudePath));
  args.insertOrAssign(RobustAutomaticThreshold::k_ArrayCreationPath, std::make_any<std::string>("Created Array"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
}
