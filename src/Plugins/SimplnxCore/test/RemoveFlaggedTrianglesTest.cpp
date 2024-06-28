#include "SimplnxCore/Filters/RemoveFlaggedTrianglesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
fs::path k_ExemplarDataFilePath = fs::path(fmt::format("{}/remove_flagged_triangles_test_1/remove_flagged_triangles_test.dream3d", nx::core::unit_test::k_TestFilesDir));
fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/remove_flagged_triangles_test_1/data_to_generate_test/masked_triangle_geometry.dream3d", nx::core::unit_test::k_TestFilesDir));

constexpr StringLiteral k_RegionIdsName = "Region Ids";
constexpr StringLiteral k_FaceNormalsName = "Face Normals";
constexpr StringLiteral k_VertexListName = "SharedVertexList";
constexpr StringLiteral k_TriangleListName = "SharedTriList";

const DataPath k_TriangleGeomPath({"TriangleGeometry"});
const DataPath k_MaskPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(Constants::k_Mask);
const DataPath k_RegionIdsPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(k_RegionIdsName);
const DataPath k_FaceNormalsPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(k_FaceNormalsName);
const DataPath k_ReducedGeomPath({"ReducedGeometry"});
const DataPath k_ExemplarReducedGeomPath({"ExemplarReducedGeometry"});
} // namespace

TEST_CASE("SimplnxCore::RemoveFlaggedTrianglesFilter: Test Algorithm", "[SimplnxCore][RemoveFlaggedTrianglesFilter]")
{
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, "remove_flagged_elements_data.tar.gz", "remove_flagged_elements_data");

  // Load DataStructure containing the base geometry and an exemplar cleaned geometry
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);

  {
    // Instantiate the filter and an Arguments Object
    RemoveFlaggedTrianglesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_SelectedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(::k_MaskPath));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(::k_ReducedGeomPath));

    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_TriangleDataHandling_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
    args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_TriangleDataSelectedArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{::k_FaceNormalsPath, ::k_RegionIdsPath}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(::k_RegionIdsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(::k_RegionIdsName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(::k_FaceNormalsName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(Constants::k_Face_Data).createChildPath(::k_FaceNormalsName);

    UnitTest::CompareDataArrays<int32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_VertexListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_VertexListName);

    UnitTest::CompareDataArrays<float32>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }

  {
    DataPath generated = ::k_ReducedGeomPath.createChildPath(::k_TriangleListName);
    DataPath exemplar = ::k_ExemplarReducedGeomPath.createChildPath(::k_TriangleListName);

    UnitTest::CompareDataArrays<uint64>(dataStructure.getDataRefAs<IDataArray>(generated), dataStructure.getDataRefAs<IDataArray>(exemplar));
  }
}
