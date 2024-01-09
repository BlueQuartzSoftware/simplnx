#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/LabelTriangleGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
// fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_Label_Triangle_Geometry/6_6_Label_Triangle_Geometry.dream3d", nx::core::unit_test::k_TestFilesDir));
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/reverse_triangle_winding/6_6_reverse_triangle_winding.dream3d", nx::core::unit_test::k_TestFilesDir));
static constexpr StringLiteral k_AttributeMatrixName = "Attribute Matrix";
static constexpr StringLiteral k_NumTrianglesName = "Num Triangles Array";

const DataPath k_TriangleGeomPath({Constants::k_DataContainer});
} // namespace

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: Valid Filter Execution", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  // const nx::core::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_Label_Triangle_Geometry.tar.gz",
  //                                                            "6_6_Label_Triangle_Geometry.dream3d");

  // WIP: Upload 6_6_Label_Triangle_Geometry test data
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "reverse_triangle_winding.tar.gz",
                                                              "reverse_triangle_winding.dream3d");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    LabelTriangleGeometryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(DataPath({Constants::k_DataContainer.str(), "Triangles", "Region IDs"})));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>(k_AttributeMatrixName));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<std::string>(k_NumTrianglesName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }
}
