#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Preserve winding and pruning warnings", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // All wall voxels have positive Feature Ids, so Background-Backed mode cannot prune any faces.
  // The multi-feature junctions also produce a winding-repair warning, verified with the mode off.
  const auto runMesh = [&scope](DataStructure& dataStructure, ChoicesParameter::ValueType mode) {
    return SurfaceMeshingTest::RunMesherRaw<QuickSurfaceMeshFilter>(
        dataStructure, DataPath({"QuickMesh"}), mode, [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); }, true, &scope);
  };
  auto fullDataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
  auto fullResult = runMesh(fullDataStructure, BoundingBoxSkinMode::k_Off);
  SIMPLNX_RESULT_REQUIRE_VALID(fullResult);
  REQUIRE(fullResult.warnings().size() == 1);
  REQUIRE(fullResult.warnings()[0].code == -56730);

  auto backgroundBackedDataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
  auto backgroundBackedResult = runMesh(backgroundBackedDataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
  SIMPLNX_RESULT_REQUIRE_VALID(backgroundBackedResult);
  REQUIRE(backgroundBackedResult.warnings().size() == 2);
  REQUIRE(backgroundBackedResult.warnings()[0].code == -56730);
  REQUIRE(backgroundBackedResult.warnings()[1].code == MeshingUtilities::k_NoFacesPrunedWarning);
  UnitTest::CheckArraysInheritTupleDims(fullDataStructure);
  UnitTest::CheckArraysInheritTupleDims(backgroundBackedDataStructure);
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Feature DataArray tuple bounds", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image Geometry");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({3, 1, 1});
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "Cell Data", {1, 1, 3}, imageGeom->getId());
  REQUIRE(cellAM != nullptr);
  imageGeom->setCellData(cellAM->getId());
  auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Feature Ids", {1, 1, 3}, {1}, cellAM->getId());
  REQUIRE(featureIds != nullptr);
  (*featureIds)[0] = 0;
  (*featureIds)[1] = 1;
  (*featureIds)[2] = 2;
  auto* featureAM = AttributeMatrix::Create(dataStructure, "Feature Data", {2}, imageGeom->getId());
  REQUIRE(featureAM != nullptr);
  auto* featureValues = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Feature Values", {2}, {1}, featureAM->getId());
  REQUIRE(featureValues != nullptr);
  featureValues->fill(7);

  QuickSurfaceMeshFilter filter;
  Arguments args;
  args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "Cell Data", "Feature Ids"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath({"Image Geometry", "Feature Data", "Feature Values"})}));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -62073);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "quick_surface_mesh_test_v3.tar.gz", "quick_surface_mesh_test_v3");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/quick_surface_mesh_test_v3/quick_surface_mesh_test_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

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

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, 2 + selectedCellArrayPaths.size() + selectedFeatureArrayPaths.size());
    std::vector<DataPath> expectedArrayPaths = {vertexGroupDataPath.createChildPath(k_NodeTypeArrayName), faceGroupDataPath.createChildPath(k_Face_Labels)};
    for(const auto& path : selectedCellArrayPaths)
    {
      expectedArrayPaths.push_back(faceGroupDataPath.createChildPath(path.getTargetName()));
    }
    for(const auto& path : selectedFeatureArrayPaths)
    {
      expectedArrayPaths.push_back(faceGroupDataPath.createChildPath(path.getTargetName()));
    }
    UnitTest::RequireAutomaticCreateArrayActions(preflightResult.outputActions, expectedArrayPaths);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63440);
  REQUIRE(vertices->getNumberOfTuples() == 28910);

  // Compare the shared vertex and triangle lists.
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

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "quick_surface_mesh_test_v3.tar.gz", "quick_surface_mesh_test_v3");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/quick_surface_mesh_test_v3/quick_surface_mesh_test_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

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

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

// The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63440);
  REQUIRE(vertices->getNumberOfTuples() == 28910);

  // Compare the shared vertex and triangle lists.
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

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "quick_surface_mesh_test_v3.tar.gz", "quick_surface_mesh_test_v3");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/quick_surface_mesh_test_v3/quick_surface_mesh_test_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

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

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

// The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63410);
  REQUIRE(vertices->getNumberOfTuples() == 28902);

  // Compare the shared vertex and triangle lists.
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

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "quick_surface_mesh_test_v3.tar.gz", "quick_surface_mesh_test_v3");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/quick_surface_mesh_test_v3/quick_surface_mesh_test_v3.dream3d", nx::core::unit_test::k_TestFilesDir));
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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdCellDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(ebsdCellDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(ebsdFeatureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(ebsdFeatureDataPath.createChildPath(child.second->getName()));
    }

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

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

// The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/QuickSurfaceMeshFilterTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 63410);
  REQUIRE(vertices->getNumberOfTuples() == 28902);

  // Compare the shared vertex and triangle lists.
  CompareArrays<IGeometry::MeshIndexType>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName), exemplarSharedTriPath);
  CompareArrays<float32>(dataStructure, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName), exemplarSharedVertexPath);

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Strict Feature Ids", "[SimplnxCore][QuickSurfaceMeshFilter][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  const bool rectilinear = GENERATE(false, true);
  const bool fixProblemVoxels = GENERATE(false, true);
  CAPTURE(rectilinear, fixProblemVoxels);
  DataStructure dataStructure;
  const DataPath geometryPath({"Strict Geometry"});
  const DataPath cellPath = geometryPath.createChildPath("Cell Data");
  const DataPath featureIdsPath = cellPath.createChildPath("Feature Ids");
  const ShapeType shape{1, 2, 2};
  IGridGeometry* gridGeomPtr = nullptr;
  if(rectilinear)
  {
    gridGeomPtr = RectGridGeom::Create(dataStructure, geometryPath.getTargetName());
  }
  else
  {
    gridGeomPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  }
  REQUIRE(gridGeomPtr != nullptr);
  gridGeomPtr->setDimensions({2, 2, 1});
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cell Data", shape, gridGeomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  gridGeomPtr->setCellData(*cellsPtr);
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{3}, gridGeomPtr->getId()) != nullptr);
  auto* otherPtr = AttributeMatrix::Create(dataStructure, "Other Data", shape, gridGeomPtr->getId());
  REQUIRE(otherPtr != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Feature Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 1), cellsPtr->getId()) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Feature Ids", std::make_shared<DataStore<int32>>(shape, ShapeType{1}, 1), otherPtr->getId()) != nullptr);

  QuickSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(fixProblemVoxels));
  DataPath selectedPath = featureIdsPath;
  int32 expectedCode = 0;
  SECTION("Correct Cell Attribute Matrix")
  {
  }
  SECTION("Feature Ids in another Attribute Matrix with the same tuple shape")
  {
    selectedPath = geometryPath.createChildPath("Other Data").createChildPath("Feature Ids");
    expectedCode = fixProblemVoxels ? -76532 : 0;
  }
  SECTION("Feature Ids through an alias of the Cell Attribute Matrix")
  {
    auto* alternateParentPtr = DataGroup::Create(dataStructure, "Alternate Parent");
    REQUIRE(alternateParentPtr != nullptr);
    REQUIRE(dataStructure.setAdditionalParent(cellsPtr->getId(), alternateParentPtr->getId()));
    selectedPath = DataPath({"Alternate Parent", "Cell Data", "Feature Ids"});
    REQUIRE(gridGeomPtr->getCellDataPath() == cellPath);
  }
  SECTION("Feature Ids with fewer tuples than grid cells")
  {
    selectedPath = DataPath({"Short Feature Ids"});
    REQUIRE(Int32Array::Create(dataStructure, "Short Feature Ids", std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{1}, 1)) != nullptr);
    expectedCode = -76533;
  }
  SECTION("Feature Ids with more tuples than grid cells")
  {
    selectedPath = DataPath({"Long Feature Ids"});
    REQUIRE(Int32Array::Create(dataStructure, "Long Feature Ids", std::make_shared<DataStore<int32>>(ShapeType{5}, ShapeType{1}, 1)) != nullptr);
    expectedCode = -76533;
  }
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(selectedPath));
  const auto result = filter.preflight(dataStructure, args);
  if(expectedCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
    const auto& modificationsRef = result.outputActions.value().modifiedActions;
    if(fixProblemVoxels)
    {
      REQUIRE(modificationsRef.size() == 1);
      CHECK(modificationsRef.front().modifiedPath == featureIdsPath);
      CHECK(modificationsRef.front().modifiedType == DataObjectModification::ModifiedType::Modified);
    }
    else
    {
      CHECK(modificationsRef.empty());
    }
  }
  else
  {
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors().front();
    CHECK(errorRef.code == expectedCode);
    CHECK(errorRef.message.find(selectedPath.toString()) != std::string::npos);
    CHECK(errorRef.message.find(geometryPath.toString()) != std::string::npos);
    if(expectedCode == -76532)
    {
      CHECK(errorRef.message.find(cellPath.toString()) != std::string::npos);
    }
  }
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
      // Successful pipeline loading verifies the MultiDataArraySelectionFilterParameterConverter value.
      CHECK(args.value<DataPath>(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key) == "TestName");
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key) == "TestName");
      CHECK(args.value<std::string>(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Problem voxel correction handles every body diagonal", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  const usize diagonal = GENERATE(0, 1, 2, 3);
  CAPTURE(scenario, diagonal);
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({2, 2, 2});
  geometry->setSpacing({2, 3, 5});
  geometry->setOrigin({10, 20, 30});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", {2, 2, 2}, geometry->getId());
  REQUIRE(cells != nullptr);
  geometry->setCellData(cells->getId());
  const DataPath idsPath({"Image", "Cells", "Ids"});
  auto* ids = Int32Array::Create(dataStructure, "Ids", DataStoreUtilities::CreateDataStore<int32>(dataStructure, idsPath, {2, 2, 2}, {1}), cells->getId());
  REQUIRE(ids != nullptr);
  // Opposite corners in X-fastest order are (0,7), (1,6), (2,5), and (3,4).
  std::array<int32, 8> input = {1, 1, 1, 1, 1, 1, 1, 1};
  input[diagonal] = 2;
  input[7 - diagonal] = 3;
  auto writeResult = ids->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  QuickSurfaceMeshFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(idsPath));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({"Mesh"})));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(true));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
  args.insertOrAssign(QuickSurfaceMeshFilter::k_BoundingBoxSkinMode_Key, std::make_any<ChoicesParameter::ValueType>(BoundingBoxSkinMode::k_Off));
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  std::array<int32, 8> actual{};
  auto readResult = ids->getDataStoreRef().copyIntoBuffer(0, actual);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  auto firstAllowed = input;
  auto secondAllowed = input;
  firstAllowed[diagonal] = 1;
  secondAllowed[7 - diagonal] = 1;
  CHECK((actual == firstAllowed || actual == secondAllowed));
  CHECK(std::count(actual.cbegin(), actual.cend(), 1) == 7);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(DataPath({"Mesh"})));
  // One minority voxel leaves 24 outer faces plus 3 internal faces, each split into two triangles.
  CHECK(dataStructure.getDataRefAs<TriangleGeom>(DataPath({"Mesh"})).getNumberOfFaces() == 54);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
