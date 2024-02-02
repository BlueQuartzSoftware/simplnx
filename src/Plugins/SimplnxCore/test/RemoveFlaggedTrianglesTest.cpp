#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/RemoveFlaggedTrianglesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
namespace
{
fs::path k_ExemplarDataFilePath = fs::path(fmt::format("{}/remove_flagged_triangles_test/remove_flagged_triangles_test.dream3d", nx::core::unit_test::k_TestFilesDir));
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_triangles_test/data_to_generate_test/masked_triangle_geometry.dream3d", nx::core::unit_test::k_TestFilesDir));

static constexpr StringLiteral k_CreatedAMName = "Cell Feature AM";
static constexpr StringLiteral k_NumTrianglesName = "NumTriangles";
static constexpr StringLiteral k_RegionIdsName = "Region Ids";

const DataPath k_TriangleGeomPath({"TriangleGeometry"});
const DataPath k_MaskPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(Constants::k_Mask);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});

const DataPath k_VertexListPath = k_ReducedGeomPath.createChildPath("SharedVertexList");
const DataPath k_TriangleListPath = k_ReducedGeomPath.createChildPath("SharedTriList");
} // namespace
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedTrianglesFilter: Valid Filter Execution", "[SimplnxCore][RemoveFlaggedTrianglesFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, "remove_flagged_triangles_test.tar.gz", "remove_flagged_triangles_test.dream3d");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedTrianglesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_InputGeometry_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(::k_MaskPath));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_OutputGeometry_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(k_ExemplarDataFilePath);

  UnitTest::CompareDataArrays<uint64>(dataStructure.getDataRefAs<IDataArray>(::k_TriangleListPath), exemplarDataStructure.getDataRefAs<IDataArray>(::k_TriangleListPath));
  UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(::k_VertexListPath), exemplarDataStructure.getDataRefAs<IDataArray>(::k_VertexListPath));
}
