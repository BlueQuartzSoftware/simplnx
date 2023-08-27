#include "ComplexCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;
using namespace complex::Constants;

TEST_CASE("ComplexCore::QuickSurfaceMeshFilter", "[ComplexCore][QuickSurfaceMeshFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "SurfaceMeshTest.tar.gz", "SurfaceMeshTest");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceMeshTest/SurfaceMeshTest.dream3d", complex::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});

  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath ebsdSanDataPath({k_DataContainer, k_CellData});
  // DataPath triangleParentGroup({k_DataContainer});
  DataPath triangleGeometryPath({"QuickSurface Mesh Test"});
  const std::string exemplarGeometryPath("QuickSurface Mesh");

  {
    // DataStructure dataStructure;
    Arguments args;
    QuickSurfaceMeshFilter filter;

    // Create default Parameters for the filter.

    args.insertOrAssign(QuickSurfaceMeshFilter::k_GenerateTripleLines_Key, std::make_any<bool>(false));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));

    DataPath gridGeomDataPath({k_DataContainer});
    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));

    MultiArraySelectionParameter::ValueType selectedArrayPaths = {ebsdSanDataPath.createChildPath("BoundaryCells"), ebsdSanDataPath.createChildPath("ConfidenceIndex"),
                                                                  ebsdSanDataPath.createChildPath("IPFColors")};

    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedArrayPaths));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(triangleGeometryPath));

    DataPath vertexGroupDataPath = triangleGeometryPath.createChildPath(k_VertexDataGroupName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));

    DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));

    DataPath faceGroupDataPath = triangleGeometryPath.createChildPath(k_FaceDataGroupName);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));

    DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    // Check a few things about the generated data.
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
    IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

    REQUIRE(triangle->getNumberOfTuples() == 63804);
    REQUIRE(vertices->getNumberOfTuples() == 28894);

    // Compare the shift values
    CompareArrays<IGeometry::MeshIndexType>(dataStructure, triangleGeometryPath.createChildPath("SharedTriList"), DataPath({exemplarGeometryPath, "SharedTriList"}));
    CompareArrays<float32>(dataStructure, triangleGeometryPath.createChildPath("SharedVertexList"), DataPath({exemplarGeometryPath, "SharedVertexList"}));
  }

  CompareExemplarToGeneratedData(dataStructure, dataStructure, triangleGeometryPath.createChildPath(k_FaceDataGroupName), exemplarGeometryPath);

  // Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
