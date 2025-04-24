#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/LabelTriangleGeometryFilter.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/Filters/VerifyTriangleWindingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;

namespace
{
const DataPath k_TriangleGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_FaceDataPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data);
} // namespace

TEST_CASE("SimplnxCore::VerifyTriangleWindingFilter: Valid Filter Execution", "[SimplnxCore][VerifyTriangleWindingFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "STL_Models.tar.gz", "STL_Models");

  DataStructure dataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    auto& triGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    auto& triangles = triGeom.getFacesRef();

    for(usize i = 0; i < triangles.getNumberOfTuples(); i++)
    {
      if(i % 4 == 0)
      {
        IGeometry::MeshIndexType temp = triangles[(i * 3) + 0];
        triangles[(i * 3) + 0] = triangles[(i * 3) + 2];
        triangles[(i * 3) + 2] = temp;
      }
    }
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    LabelTriangleGeometryFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(::k_TriangleGeomPath));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(::k_FaceDataPath.createChildPath(Constants::k_FeatureIds)));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<std::string>("Region Data"));
    args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<std::string>("Num Triangles"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // copy region ids into the surface labels
    auto& faceLabels = dataStructure.getDataRefAs<Int32Array>(k_FaceDataPath.createChildPath(Constants::k_FaceLabels));
    auto& regionIds = dataStructure.getDataRefAs<Int32Array>(k_FaceDataPath.createChildPath(Constants::k_FeatureIds));
    for(usize i = 0; i < faceLabels.getNumberOfTuples(); i++)
    {
      faceLabels[(i * 2) + 0] = 0;
      faceLabels[(i * 2) + 1] = regionIds[i];
    }
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    VerifyTriangleWindingFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(VerifyTriangleWindingFilter::k_SurfaceMeshFaceLabelsPath_Key, std::make_any<DataPath>(k_FaceDataPath.createChildPath(Constants::k_FaceLabels)));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_RepairNormals_Key, std::make_any<bool>(true));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_TriangleNormalsPath_Key, std::make_any<DataPath>(k_FaceDataPath.createChildPath(Constants::k_FaceNormals)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  DataStructure exemplarDataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(exemplarDataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(exemplarDataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const DataPath normalsPath = k_FaceDataPath.createChildPath(Constants::k_FaceNormals);

  UnitTest::CompareDataArrays<float64>(dataStructure.getDataRefAs<Float64Array>(normalsPath), exemplarDataStructure.getDataRefAs<Float64Array>(normalsPath));
}

TEST_CASE("SimplnxCore::VerifyTriangleWindingFilter: Duplicate Vertices Catch", "[SimplnxCore][VerifyTriangleWindingFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "STL_Models.tar.gz", "STL_Models");

  DataStructure dataStructure = {};
  {
    Arguments args;
    ReadStlFileFilter filter;

    std::string inputFile = fmt::format("{}/STL_Models/Example_Triangle_Geometry.stl", unit_test::k_TestFilesDir);

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<fs::path>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleGeomPath));
    args.insertOrAssign(ReadStlFileFilter::k_CreateFaceLabels_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadStlFileFilter::k_FaceLabelsName_Key, std::make_any<std::string>(Constants::k_FaceLabels));
    args.insertOrAssign(ReadStlFileFilter::k_FaceNormalsName_Key, std::make_any<std::string>(Constants::k_FaceNormals));
    args.insertOrAssign(ReadStlFileFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Face_Data));
    args.insertOrAssign(ReadStlFileFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_Vertex_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    auto& triGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    auto& vertexList = triGeom.getVerticesRef();

    constexpr usize currentVert = 3;
    constexpr usize prevVert = 0;

    // replace vert with a duplicate point (doesn't matter that it invalidates faces)
    vertexList[currentVert] = vertexList[prevVert];
    vertexList[currentVert + 1] = vertexList[prevVert + 1];
    vertexList[currentVert + 2] = vertexList[prevVert + 2];
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    VerifyTriangleWindingFilter filter;
    Arguments args;

    const DataPath faceDataPath = k_TriangleGeomPath.createChildPath(Constants::k_Face_Data);

    // Create default Parameters for the filter.
    args.insertOrAssign(VerifyTriangleWindingFilter::k_SurfaceMeshFaceLabelsPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(Constants::k_FaceLabels)));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_RepairNormals_Key, std::make_any<bool>(true));
    args.insertOrAssign(VerifyTriangleWindingFilter::k_TriangleNormalsPath_Key, std::make_any<DataPath>(faceDataPath.createChildPath(Constants::k_FaceNormals)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }
}
