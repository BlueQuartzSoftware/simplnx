#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SurfaceMeshingTestUtils.hpp"

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
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Anisotropic spacing places Z correctly", "[SimplnxCore][SurfaceNetsFilter]")
{
  // Regression test for a bug where the Z half-voxel relocation offset used voxelSize[1] (Y
  // spacing) instead of voxelSize[2] (Z spacing). On isotropic data (the exemplar dataset used
  // by the other SurfaceNetsFilter tests above) voxelSize[1] == voxelSize[2], so the bug is
  // invisible there. Y and Z spacing are chosen sharply different here (1.0 vs 4.0) so a
  // Y-for-Z substitution is unmistakable in the resulting mesh's Z bounding box.
  UnitTest::LoadPlugins();

  const FloatVec3 k_Spacing = {0.25F, 1.0F, 4.0F};
  DataStructure dataStructure = SurfaceMeshingTest::CreateCylinderInBox(/*flushWithBottom=*/true, k_Spacing);
  const DataPath k_TriangleGeomPath({"SurfaceNets"});

  SurfaceMeshingTest::MeshResult meshResult = SurfaceMeshingTest::RunMesher<SurfaceNetsFilter>(std::move(dataStructure), k_TriangleGeomPath, BoundingBoxSkinMode::k_Off, [](Arguments& args) {
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
  });

  REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
  const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
  const auto boundingBox = triangleGeom.getBoundingBox();

  // The cylinder is flush with the box's bottom Z wall, and the box's top Z wall is pure
  // background over its full X/Y extent, so the mesh's Z bounding box exactly spans the box:
  // [0, dimZ * spacingZ]. With the bug (Y spacing substituted for Z spacing), every vertex's Z
  // is shifted by 0.5*(spacingZ - spacingY) = 1.5, moving both bounds to [1.5, 49.5].
  const float32 k_ExpectedMinZ = 0.0F;
  const float32 k_ExpectedMaxZ = static_cast<float32>(SurfaceMeshingTest::k_BoxDim) * k_Spacing[2];
  const float32 k_Tolerance = 1.0e-4F;

  REQUIRE_THAT(boundingBox.getMinPoint()[2], Catch::Matchers::WithinAbs(k_ExpectedMinZ, k_Tolerance));
  REQUIRE_THAT(boundingBox.getMaxPoint()[2], Catch::Matchers::WithinAbs(k_ExpectedMaxZ, k_Tolerance));

  UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Default", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
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
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

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
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
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

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
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
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

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
    IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
    IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();
    REQUIRE(triangle->getNumberOfTuples() == 668786);
    REQUIRE(vertices->getNumberOfTuples() == 319447);
  }

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
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

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
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
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

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
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
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

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
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
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

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
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex list and shared triangle list
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
