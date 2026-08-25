#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/QuickSurfaceMeshTest_v2/QuickSurfaceMeshTest_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath ebsdCellDataPath({k_DataContainer, k_CellData});
  DataPath ebsdFeatureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed QuickMesh"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar QuickMesh"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    QuickSurfaceMeshFilter filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63440);
  REQUIRE(vertices->getNumberOfTuples() == 28910);

  // Compare the shared vertex list and shared triangle list
  CompareArrays<IGeometry::MeshIndexType>(dataStructure, exemplarSharedTriPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Winding", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/QuickSurfaceMeshTest_v2/QuickSurfaceMeshTest_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath ebsdCellDataPath({k_DataContainer, k_CellData});
  DataPath ebsdFeatureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed QuickMesh"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar QuickMesh Winding"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    QuickSurfaceMeshFilter filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63440);
  REQUIRE(vertices->getNumberOfTuples() == 28910);

  // Compare the shared vertex list and shared triangle list
  CompareArrays<IGeometry::MeshIndexType>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName), exemplarSharedTriPath);
  CompareArrays<float32>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName), exemplarSharedVertexPath);

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Problem Voxels", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/QuickSurfaceMeshTest_v2/QuickSurfaceMeshTest_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath ebsdCellDataPath({k_DataContainer, k_CellData});
  DataPath ebsdFeatureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed QuickMesh"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar QuickMesh Problem Voxels"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    QuickSurfaceMeshFilter filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(true));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63440);
  REQUIRE(vertices->getNumberOfTuples() == 28910);

  // Compare the shared vertex list and shared triangle list
  CompareArrays<IGeometry::MeshIndexType>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName), exemplarSharedTriPath);
  CompareArrays<float32>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName), exemplarSharedVertexPath);

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Winding and Problem Voxels", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "QuickSurfaceMeshTest_v2.tar.gz", "QuickSurfaceMeshTest_v2");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/QuickSurfaceMeshTest_v2/QuickSurfaceMeshTest_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath ebsdCellDataPath({k_DataContainer, k_CellData});
  DataPath ebsdFeatureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed QuickMesh"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar QuickMesh Problem Voxel Winding"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    QuickSurfaceMeshFilter filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(true));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63440);
  REQUIRE(vertices->getNumberOfTuples() == 28910);

  // Compare the shared vertex list and shared triangle list
  CompareArrays<IGeometry::MeshIndexType>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName), exemplarSharedTriPath);
  CompareArrays<float32>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName), exemplarSharedVertexPath);

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: SIMPL Backwards Compatibility", "[SimplnxCore][QuickSurfaceMeshFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "QuickSurfaceMeshFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "QuickSurfaceMeshFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<QuickSurfaceMeshFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<bool>(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key) == true);
      }
      CHECK(args.value<DataPath>(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key) == "TestName");
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key) == "TestName");
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key) == "TestName");
    }
  }
}
