#include "SimplnxCore/Filters/LaplacianSmoothingFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/LaplacianSmoothing.hpp"
#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::constants;

TEST_CASE("SimplnxCore::LaplacianSmoothingFilter", "[SurfaceMeshing][LaplacianSmoothingFilter]")
{
  unit_test::LoadPlugins();

  std::string triangleGeometryName = "[Triangle Geometry]";
  std::string triangleFaceDataGroupName = INodeGeometry2D::k_FaceAttributeMatrixName;
  std::string normalsDataArrayName = "FaceNormals";
  std::string triangleVertexDataGroupName = INodeGeometry0D::k_VertexAttributeMatrixName;
  std::string nodeTypeArrayName = "Node Type";

  DataStructure dataStructure;

  {
    Arguments args;
    ReadStlFileFilter filter;

    DataPath triangleGeomDataPath({triangleGeometryName});
    DataPath triangleFaceDataGroupDataPath({triangleGeometryName, triangleFaceDataGroupName});
    DataPath normalsDataPath({triangleGeometryName, triangleFaceDataGroupName, normalsDataArrayName});

    std::string inputFile = fmt::format("{}/ASTMD638_specimen.stl", unit_test::k_SimplnxTestDataSourceDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 92);
    REQUIRE(triangleGeom.getNumberOfVertices() == 48);
  }

  {
    DataPath triangleGeometryPath({triangleGeometryName});

    DataObject::IdType triangleGeometryId = dataStructure.getId(triangleGeometryPath).value();
    DataPath vertexDataGroupPath = triangleGeometryPath.createChildPath(triangleVertexDataGroupName);
    DataObject::IdType vertexDataGroupId = dataStructure.getId(vertexDataGroupPath).value();

    // Instantiate the filter, a DataStructure object and an Arguments Object
    LaplacianSmoothingFilter filter;
    Arguments args;

    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryPath);
    std::vector<size_t> tupleShape = {triangleGeom.getNumberOfVertices()};
    std::vector<size_t> compShape = {1};
    // Insert a Node Type array into the DataStructure so the filter works.
    Int8ArrayType* nodeTypePtr = nx::core::unit_test::CreateTestDataArray<int8_t>(dataStructure, nodeTypeArrayName, tupleShape, compShape, vertexDataGroupId);

    // Assign the `Default Node Type` to all the values
    for(size_t i = 0; i < triangleGeom.getNumberOfVertices(); i++)
    {
      (*nodeTypePtr)[i] = nx::core::node_type::k_Default;
    }

    DataPath nodeTypeArrayPath = vertexDataGroupPath.createChildPath(nodeTypeArrayName);

    // Create default Parameters for the filter.
    args.insertOrAssign(LaplacianSmoothingFilter::k_IterationSteps_Key, std::make_any<int32>(5));
    args.insertOrAssign(LaplacianSmoothingFilter::k_Lambda_Key, std::make_any<float32>(0.15F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_UseTaubinSmoothing_Key, std::make_any<bool>(true));
    args.insertOrAssign(LaplacianSmoothingFilter::k_MuFactor_Key, std::make_any<float32>(.1F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_TripleLineLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_QuadPointLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfacePointLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceTripleLineLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceQuadPointLambda_Key, std::make_any<float32>(.25F));
    args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceMeshNodeTypeArrayPath_Key, std::make_any<DataPath>(nodeTypeArrayPath));
    args.insertOrAssign(LaplacianSmoothingFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(triangleGeometryPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    if(preflightResult.output_actions.invalid())
    {
      for(const auto& error : preflightResult.output_actions.errors())
      {
        std::cout << error.code << ": " << error.message << '\n';
      }
    }
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/laplacian_smoothing_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::LaplacianSmoothingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][LaplacianSmoothingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "LaplacianSmoothingFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "LaplacianSmoothingFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<LaplacianSmoothingFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<int32>(LaplacianSmoothingFilter::k_IterationSteps_Key) == 5);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_Lambda_Key) == 2.5f);
      CHECK(args.value<bool>(LaplacianSmoothingFilter::k_UseTaubinSmoothing_Key) == true);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_MuFactor_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_TripleLineLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_QuadPointLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_SurfacePointLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_SurfaceTripleLineLambda_Key) == 2.5f);
      CHECK(args.value<float32>(LaplacianSmoothingFilter::k_SurfaceQuadPointLambda_Key) == 2.5f);
      CHECK(args.value<DataPath>(LaplacianSmoothingFilter::k_TriangleGeometryDataPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(LaplacianSmoothingFilter::k_SurfaceMeshNodeTypeArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(LaplacianSmoothingFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

TEST_CASE("SimplnxCore::LaplacianSmoothingFilter: Smoothing preserves input edge topology and attributes", "[SimplnxCore][LaplacianSmoothingFilter]")
{
  const bool existingEdges = GENERATE(false, true);
  const bool cancelled = GENERATE(false, true);
  CAPTURE(existingEdges, cancelled);
  DataStructure dataStructure;
  const DataPath geometryPath({"Mesh"});
  auto* geometryPtr = TriangleGeom::Create(dataStructure, "Mesh");
  REQUIRE(geometryPtr != nullptr);
  const DataPath verticesPath = geometryPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);
  auto* verticesPtr =
      Float32ArrayType::Create(dataStructure, verticesPath.getTargetName(), data_store_utilities::CreateDataStore<float32>(dataStructure, verticesPath, {3}, {3}), geometryPtr->getId());
  const DataPath facesPath = geometryPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  auto* facesPtr = UInt64ArrayType::Create(dataStructure, facesPath.getTargetName(), data_store_utilities::CreateDataStore<uint64>(dataStructure, facesPath, {1}, {3}), geometryPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  // The triangle has vertices (0,0,0), (2,0,0), (0,3,0) and edges 01,12,20.
  const std::array<float32, 9> coordinates = {0, 0, 0, 2, 0, 0, 0, 3, 0};
  const std::array<uint64, 3> face = {0, 1, 2};
  const std::array<uint64, 6> edgeValues = {0, 1, 1, 2, 2, 0};
  auto vertexResult = verticesPtr->getDataStoreRef().copyFromBuffer(0, coordinates);
  SIMPLNX_RESULT_REQUIRE_VALID(vertexResult);
  auto faceResult = facesPtr->getDataStoreRef().copyFromBuffer(0, face);
  SIMPLNX_RESULT_REQUIRE_VALID(faceResult);
  geometryPtr->setVertices(*verticesPtr);
  geometryPtr->setFaceList(*facesPtr);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", {3}, geometryPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  geometryPtr->setVertexDataId(vertexDataPtr->getId());
  const DataPath nodePath({"Mesh", "Vertex Data", "Node Types"});
  auto* nodesPtr = Int8ArrayType::Create(dataStructure, "Node Types", data_store_utilities::CreateDataStore<int8>(dataStructure, nodePath, {3}, {1}), vertexDataPtr->getId());
  REQUIRE(nodesPtr != nullptr);
  nodesPtr->fill(node_type::k_Default);
  auto* edgeDataPtr = AttributeMatrix::Create(dataStructure, "CustomEdges", {3}, geometryPtr->getId());
  REQUIRE(edgeDataPtr != nullptr);
  geometryPtr->setEdgeDataId(edgeDataPtr->getId());
  const DataPath markerPath({"Mesh", "CustomEdges", "Marker"});
  auto* markerPtr = Int32ArrayType::Create(dataStructure, "Marker", data_store_utilities::CreateDataStore<int32>(dataStructure, markerPath, {3}, {1}), edgeDataPtr->getId());
  REQUIRE(markerPtr != nullptr);
  const std::array<int32, 3> markers = {11, 22, 33};
  auto markerResult = markerPtr->getDataStoreRef().copyFromBuffer(0, markers);
  SIMPLNX_RESULT_REQUIRE_VALID(markerResult);
  const auto edgeDataId = edgeDataPtr->getId();
  const auto markerId = markerPtr->getId();
  const DataPath edgesPath = geometryPath.createChildPath(INodeGeometry1D::k_SharedEdgeListName);
  std::optional<DataObject::IdType> edgeId;
  if(existingEdges)
  {
    auto* edgesPtr = UInt64ArrayType::Create(dataStructure, edgesPath.getTargetName(), data_store_utilities::CreateDataStore<uint64>(dataStructure, edgesPath, {3}, {2}), geometryPtr->getId());
    REQUIRE(edgesPtr != nullptr);
    auto edgeResult = edgesPtr->getDataStoreRef().copyFromBuffer(0, edgeValues);
    SIMPLNX_RESULT_REQUIRE_VALID(edgeResult);
    geometryPtr->setEdgeList(*edgesPtr);
    edgeId = edgesPtr->getId();
  }
  LaplacianSmoothingInputValues inputValues{};
  inputValues.p_triangle_geometry_data_path = geometryPath;
  inputValues.p_surface_mesh_node_type_array_path = nodePath;
  inputValues.p_iteration_steps = cancelled ? 1 : 0;
  const std::atomic_bool shouldCancel = cancelled;
  IFilter::MessageHandler messageHandler;
  auto result = LaplacianSmoothing(dataStructure, &inputValues, shouldCancel, messageHandler)();
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(geometryPtr->getEdgeListId() == edgeId);
  CHECK(geometryPtr->getEdgeAttributeMatrixId() == edgeDataId);
  CHECK(geometryPtr->getEdgeAttributeMatrix() == edgeDataPtr);
  CHECK(dataStructure.getId(DataPath({"Mesh", "CustomEdges"})) == edgeDataId);
  CHECK(dataStructure.getId(markerPath) == markerId);
  std::array<int32, 3> actualMarkers{};
  auto markerRead = markerPtr->getDataStoreRef().copyIntoBuffer(0, actualMarkers);
  SIMPLNX_RESULT_REQUIRE_VALID(markerRead);
  CHECK(actualMarkers == markers);
  std::array<float32, 9> actualVertices{};
  auto vertexRead = verticesPtr->getDataStoreRef().copyIntoBuffer(0, actualVertices);
  SIMPLNX_RESULT_REQUIRE_VALID(vertexRead);
  CHECK(actualVertices == coordinates);
  if(existingEdges)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64ArrayType>(edgesPath));
    CHECK(dataStructure.getId(edgesPath) == edgeId);
    std::array<uint64, 6> actualEdges{};
    auto edgeRead = dataStructure.getDataRefAs<UInt64ArrayType>(edgesPath).getDataStoreRef().copyIntoBuffer(0, actualEdges);
    SIMPLNX_RESULT_REQUIRE_VALID(edgeRead);
    CHECK(actualEdges == edgeValues);
  }
  else
  {
    CHECK(dataStructure.getData(edgesPath) == nullptr);
    CHECK(geometryPtr->getEdges() == nullptr);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
