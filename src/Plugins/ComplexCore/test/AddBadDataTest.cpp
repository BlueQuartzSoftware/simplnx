#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AddBadDataFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using fs = std::filesystem;
using namespace complex;

TEST_CASE("ComplexCore::AddBadDataFilter: Valid Filter Execution", "[ComplexCore][AddBadDataFilter][.][UNIMPLEMENTED][!mayfail]")
{
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/6_6_add_bad_data_test.dream3d",unit_test::k_ComplexTestSourceDir)));
  // Instantiate the filter, a DataStructure object and an Arguments Object
  AddBadDataFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/7_0_add_bad_data_test.dream3d",unit_test::k_ComplexTestSourceDir)));
  std::ofstream  file("/home/nyoung/test.txt");
  dataStructure.exportHeirarchyAsText(file);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(AddBadDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489ULL));
  args.insertOrAssign(AddBadDataFilter::k_PoissonNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_PoissonVolFraction_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryVolFraction_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(AddBadDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::AddBadDataFilter: InValid Filter Execution")
//{
//
// }
