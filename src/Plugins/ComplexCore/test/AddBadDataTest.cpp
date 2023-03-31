#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AddBadDataFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
 const DataPath k_ImageGeom = DataPath({"DataContainer"});
 const DataPath k_CellDataAM = k_ImageGeom.createChildPath("CellData");
 const DataPath k_EuclideanDistances = k_CellDataAM.createChildPath("GBManhattanDistances");
}

TEST_CASE("ComplexCore::AddBadDataFilter: Valid Filter Execution", "[ComplexCore][AddBadDataFilter]")
{
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/6_6_add_bad_data_test.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  AddBadDataFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/add_bad_data_test/7_0_add_bad_data_test.dream3d",unit_test::k_TestFilesDir)));
  std::ofstream file("/home/nyoung/test.txt");
  std::ofstream file2("/home/nyoung/test2.txt");
  dataStructure.exportHeirarchyAsText(file);
  exemplarDataStructure.exportHeirarchyAsText(file2);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(AddBadDataFilter::k_UseSeed_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_SeedValue_Key, std::make_any<uint64>(5489ULL));
  args.insertOrAssign(AddBadDataFilter::k_PoissonNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_PoissonVolFraction_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryNoise_Key, std::make_any<bool>(true));
  args.insertOrAssign(AddBadDataFilter::k_BoundaryVolFraction_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(AddBadDataFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(k_EuclideanDistances));
  args.insertOrAssign(AddBadDataFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeom));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  UnitTest::WriteTestDataStructure(dataStructure, fs::path("/home/nyoung/bad_data.dream3d"));

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellDataAM, "DataContainer1");
}

// TEST_CASE("ComplexCore::AddBadDataFilter: InValid Filter Execution")
//{
//
// }
