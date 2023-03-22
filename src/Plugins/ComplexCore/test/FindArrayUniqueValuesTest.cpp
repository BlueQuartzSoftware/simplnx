#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindArrayUniqueValuesFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::FindArrayUniqueValuesTest: Instantiate Filter", "[ComplexCore][FindArrayStatisticsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindArrayUniqueValuesFilter filter;
  DataStructure dataStructure;
  Arguments args;

  DataPath inputArrayPath;
  DataPath destPath;
  DataPath maskArrayPath;
  DataPath featureIdsArrayPath;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());
}
