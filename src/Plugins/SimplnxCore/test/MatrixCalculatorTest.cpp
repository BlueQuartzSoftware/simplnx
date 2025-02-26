#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/MatrixCalculatorFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{

} // namespace

TEST_CASE("SimplnxCore::MatrixCalculatorFilter: Valid Filter Execution", "[SimplnxCore][MatrixCalculatorFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "add_bad_data_test.tar.gz", "add_bad_data_test");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/6_6_add_bad_data_test.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  MatrixCalculatorFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/6_6_add_bad_data_baseline.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.


  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

}
