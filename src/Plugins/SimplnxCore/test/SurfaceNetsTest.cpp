#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::SurfaceNetsFilter: NO Smoothing", "[SimplnxCore][SurfaceNetsFilter]")
{

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "SurfaceMeshTest.tar.gz", "SurfaceMeshTest");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceMeshTest/SurfaceMeshTest.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath smallIn100Group({nx::core::Constants::k_DataContainer});

  const DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  const DataPath ebsdSanDataPath({k_DataContainer, k_CellData});
  // DataPath triangleParentGroup({k_DataContainer});
  const DataPath triangleGeometryPath({"SurfaceNets Mesh Test"});
  const std::string exemplarGeometryPath("SurfaceNets Mesh");

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    // Create default Parameters for the filter.

    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));

    const DataPath gridGeomDataPath({k_DataContainer});
    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    const MultiArraySelectionParameter::ValueType selectedArrayPaths = {ebsdSanDataPath.createChildPath("BoundaryCells"), ebsdSanDataPath.createChildPath("ConfidenceIndex"),
                                                                        ebsdSanDataPath.createChildPath("IPFColors")};

    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(triangleGeometryPath));

    const DataPath vertexGroupDataPath = triangleGeometryPath.createChildPath(k_VertexDataGroupName);
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));

    const DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));

    const DataPath faceGroupDataPath = triangleGeometryPath.createChildPath(k_FaceDataGroupName);
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));

    const DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    // Check a few things about the generated data.
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    IGeometry::SharedTriList* trianglePtr = triangleGeom.getFaces();
    IGeometry::SharedVertexList* verticesPtr = triangleGeom.getVertices();

    REQUIRE(trianglePtr->getNumberOfTuples() == 63804);
    REQUIRE(verticesPtr->getNumberOfTuples() == 28894);

    // Compare the shift values
    CompareArrays<IGeometry::MeshIndexType>(dataStructure, triangleGeometryPath.createChildPath("SharedTriList"), DataPath({exemplarGeometryPath, "SharedTriList"}));
    CompareArrays<float32>(dataStructure, triangleGeometryPath.createChildPath("SharedVertexList"), DataPath({exemplarGeometryPath, "SharedVertexList"}));
  }

  CompareExemplarToGeneratedData(dataStructure, dataStructure, triangleGeometryPath.createChildPath(k_FaceDataGroupName), exemplarGeometryPath);

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/surface_nets.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: With Smoothing", "[SimplnxCore][SurfaceNetsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "SurfaceMeshTest.tar.gz", "SurfaceMeshTest");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceMeshTest/SurfaceMeshTest.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath smallIn100Group({nx::core::Constants::k_DataContainer});

  const DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  const DataPath ebsdSanDataPath({k_DataContainer, k_CellData});
  // DataPath triangleParentGroup({k_DataContainer});
  const DataPath triangleGeometryPath({"SurfaceNets Mesh Test"});
  const std::string exemplarGeometryPath("SurfaceNets Mesh Smooth");

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    // Create default Parameters for the filter.

    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));

    const DataPath gridGeomDataPath({k_DataContainer});
    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));

    MultiArraySelectionParameter::ValueType const selectedArrayPaths = {ebsdSanDataPath.createChildPath("BoundaryCells"), ebsdSanDataPath.createChildPath("ConfidenceIndex"),
                                                                        ebsdSanDataPath.createChildPath("IPFColors")};

    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(triangleGeometryPath));

    const DataPath vertexGroupDataPath = triangleGeometryPath.createChildPath(k_VertexDataGroupName);
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));

    const DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));

    const DataPath faceGroupDataPath = triangleGeometryPath.createChildPath(k_FaceDataGroupName);
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));

    const DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    // Check a few things about the generated data.
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    IGeometry::SharedTriList* trianglePtr = triangleGeom.getFaces();
    IGeometry::SharedVertexList* verticesPtr = triangleGeom.getVertices();

    REQUIRE(trianglePtr->getNumberOfTuples() == 63804);
    REQUIRE(verticesPtr->getNumberOfTuples() == 28894);

    // Compare the shift values
    CompareArrays<IGeometry::MeshIndexType>(dataStructure, triangleGeometryPath.createChildPath("SharedTriList"), DataPath({exemplarGeometryPath, "SharedTriList"}));
    CompareArrays<float32>(dataStructure, triangleGeometryPath.createChildPath("SharedVertexList"), DataPath({exemplarGeometryPath, "SharedVertexList"}));
  }

  CompareExemplarToGeneratedData(dataStructure, dataStructure, triangleGeometryPath.createChildPath(k_FaceDataGroupName), exemplarGeometryPath);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/surface_nets_smoothing.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
