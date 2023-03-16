#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AddBadDataFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("ComplexCore::AddBadDataFilter: Valid Filter Execution", "[ComplexCore][AddBadDataFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  AddBadDataFilter filter;
  DataStructure ds = UnitTest::LoadDataStructure(std::filesystem::path("/home/nyoung/Apps/DREAM3DNX-Dev/DREAM3D-Build/DREAM3DNX-Debug-Linux-x64/Bin/Data/Output/Reconstruction/SmallIN100_Final.dream3d"));
  std::ofstream  file("/home/nyoung/test.txt");
  ds.exportHeirarchyAsText(file);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(AddBadDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_SeedValue_Key, std::make_any<uint64>(0ULL));
  args.insertOrAssign(AddBadDataFilter::k_PoissonNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_PoissonVolFraction_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryVolFraction_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(AddBadDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::AddBadDataFilter: InValid Filter Execution")
//{
//
// }
