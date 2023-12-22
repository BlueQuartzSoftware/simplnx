#include <catch2/catch.hpp>

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
const DataPath k_TriangleGeomPath = DataPath({Constants::k_DataContainer});
}

TEST_CASE("SimplnxCore::ReverseTriangleWindingFilter: Valid Filter Execution", "[SimplnxCore][ReverseTriangleWindingFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "reverse_triangle_winding.tar.gz",
                                                              "reverse_triangle_winding");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/reverse_triangle_winding/6_6_exemplar_triangle_winding.dream3d", unit_test::k_TestFilesDir)));

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/reverse_triangle_winding/6_6_reverse_triangle_winding.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ReverseTriangleWindingFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ReverseTriangleWindingFilter::k_TriGeomPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  auto& exemplarTriGeom = exemplarDataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
  auto& generatedTriGeom = exemplarDataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);

  UnitTest::CompareDataArrays<TriangleGeom::MeshIndexType>(exemplarTriGeom.getFacesRef(), generatedTriGeom.getFacesRef());
}
