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

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Default", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v3.tar.gz", "SurfaceNetsTest_v3");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v3/SurfaceNetsTest_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      // fmt::print("Adding Cell Array: {}\n", child.second->getName());
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      // fmt::print("Adding Feature Array: {}\n", child.second->getName());
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_default.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  AbstractGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  AbstractGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<AbstractDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<AbstractDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<AbstractGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Smoothing", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v3.tar.gz", "SurfaceNetsTest_v3");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v3/SurfaceNetsTest_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets Smoothing"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_Smoothing.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  {
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
    AbstractGeometry::SharedTriList* triangle = triangleGeom.getFaces();
    AbstractGeometry::SharedVertexList* vertices = triangleGeom.getVertices();
    REQUIRE(triangle->getNumberOfTuples() == 668786);
    REQUIRE(vertices->getNumberOfTuples() == 319447);
  }

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<AbstractDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<AbstractDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<AbstractGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Winding", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v3.tar.gz", "SurfaceNetsTest_v3");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v3/SurfaceNetsTest_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets Winding"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_winding.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  AbstractGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  AbstractGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<AbstractDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<AbstractDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<AbstractGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Winding Smoothing", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v3.tar.gz", "SurfaceNetsTest_v3");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v3/SurfaceNetsTest_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets Winding Smoothing"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(AbstractNodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    // Create default Parameters for the filter.

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_winding_smoothing.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Check a few things about the generated data.
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  AbstractGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  AbstractGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<AbstractDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<AbstractDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<AbstractGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
