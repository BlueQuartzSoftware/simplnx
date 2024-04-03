#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/LabelTriangleGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
fs::path k_ExemplarDataFilePath = fs::path(fmt::format("{}/label_triangle_geometry_test/label_triangle_geometry_test.dream3d", nx::core::unit_test::k_TestFilesDir));
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/label_triangle_geometry_test/data_to_generate_test/combined_stls.dream3d", nx::core::unit_test::k_TestFilesDir));

static constexpr StringLiteral k_CreatedAMName = "Cell Feature AM";
static constexpr StringLiteral k_NumTrianglesName = "NumTriangles";
static constexpr StringLiteral k_RegionIdsName = "Region Ids";

const DataPath k_TriangleGeomPath({"TriangleGeometry"});
const DataPath k_CreatedRegionIdsPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(k_RegionIdsName);
const DataPath k_CellFeatureAMPath = k_TriangleGeomPath.createChildPath(k_CreatedAMName);
const DataPath k_NumTrianglesPath = k_CellFeatureAMPath.createChildPath(k_NumTrianglesName);
} // namespace

TEST_CASE("SimplnxCore::LabelTriangleGeometryFilter: Valid Filter Execution", "[SimplnxCore][LabelTriangleGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "label_triangle_geometry_test.tar.gz",
                                                              "label_triangle_geometry_test");

  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    LabelTriangleGeometryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(::k_CreatedRegionIdsPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>(::k_CreatedAMName));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<std::string>(::k_NumTrianglesName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // This is in here because the exemplar face attribute matrix is not sized correctly. This will
    // correct that value allowing the test to proceed normally.
    auto& exemplarContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    exemplarContourTriGeom.getVertexAttributeMatrix()->resizeTuples({144});

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(k_ExemplarDataFilePath);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, ::k_CellFeatureAMPath, ::k_TriangleGeomPath.toString());
  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, ::k_TriangleGeomPath.createChildPath(Constants::k_Face_Data), ::k_TriangleGeomPath.toString());
}
