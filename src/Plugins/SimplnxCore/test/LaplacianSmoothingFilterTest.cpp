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
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::LaplacianSmoothingFilter", "[SurfaceMeshing][LaplacianSmoothingFilter]")
{
  UnitTest::LoadPlugins();

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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

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
    Int8Array* nodeType = nx::core::UnitTest::CreateTestDataArray<int8_t>(dataStructure, nodeTypeArrayName, tupleShape, compShape, vertexDataGroupId);

    // Assign the `Default Node Type` to all the values
    for(size_t i = 0; i < triangleGeom.getNumberOfVertices(); i++)
    {
      (*nodeType)[i] = nx::core::NodeType::Default;
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
    if(preflightResult.outputActions.invalid())
    {
      for(const auto& error : preflightResult.outputActions.errors())
      {
        std::cout << error.code << ": " << error.message << std::endl;
      }
    }
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/laplacian_smoothing_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::LaplacianSmoothingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][LaplacianSmoothingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "LaplacianSmoothingFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "LaplacianSmoothingFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<LaplacianSmoothingFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
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
  auto* geometry = TriangleGeom::Create(dataStructure, "Mesh");
  REQUIRE(geometry != nullptr);
  const DataPath verticesPath = geometryPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);
  auto* vertices = Float32Array::Create(dataStructure, verticesPath.getTargetName(), DataStoreUtilities::CreateDataStore<float32>(dataStructure, verticesPath, {3}, {3}), geometry->getId());
  const DataPath facesPath = geometryPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  auto* faces = UInt64Array::Create(dataStructure, facesPath.getTargetName(), DataStoreUtilities::CreateDataStore<uint64>(dataStructure, facesPath, {1}, {3}), geometry->getId());
  REQUIRE(vertices != nullptr);
  REQUIRE(faces != nullptr);
  // The triangle has vertices (0,0,0), (2,0,0), (0,3,0) and edges 01,12,20.
  const std::array<float32, 9> coordinates = {0, 0, 0, 2, 0, 0, 0, 3, 0};
  const std::array<uint64, 3> face = {0, 1, 2};
  const std::array<uint64, 6> edgeValues = {0, 1, 1, 2, 2, 0};
  auto vertexResult = vertices->getDataStoreRef().copyFromBuffer(0, coordinates);
  SIMPLNX_RESULT_REQUIRE_VALID(vertexResult);
  auto faceResult = faces->getDataStoreRef().copyFromBuffer(0, face);
  SIMPLNX_RESULT_REQUIRE_VALID(faceResult);
  geometry->setVertices(*vertices);
  geometry->setFaceList(*faces);
  auto* vertexData = AttributeMatrix::Create(dataStructure, "Vertex Data", {3}, geometry->getId());
  REQUIRE(vertexData != nullptr);
  geometry->setVertexDataId(vertexData->getId());
  const DataPath nodePath({"Mesh", "Vertex Data", "Node Types"});
  auto* nodes = Int8Array::Create(dataStructure, "Node Types", DataStoreUtilities::CreateDataStore<int8>(dataStructure, nodePath, {3}, {1}), vertexData->getId());
  REQUIRE(nodes != nullptr);
  nodes->fill(NodeType::Default);
  auto* edgeData = AttributeMatrix::Create(dataStructure, "CustomEdges", {3}, geometry->getId());
  REQUIRE(edgeData != nullptr);
  geometry->setEdgeDataId(edgeData->getId());
  const DataPath markerPath({"Mesh", "CustomEdges", "Marker"});
  auto* marker = Int32Array::Create(dataStructure, "Marker", DataStoreUtilities::CreateDataStore<int32>(dataStructure, markerPath, {3}, {1}), edgeData->getId());
  REQUIRE(marker != nullptr);
  const std::array<int32, 3> markers = {11, 22, 33};
  auto markerResult = marker->getDataStoreRef().copyFromBuffer(0, markers);
  SIMPLNX_RESULT_REQUIRE_VALID(markerResult);
  const auto edgeDataId = edgeData->getId();
  const auto markerId = marker->getId();
  const DataPath edgesPath = geometryPath.createChildPath(INodeGeometry1D::k_SharedEdgeListName);
  std::optional<DataObject::IdType> edgeId;
  if(existingEdges)
  {
    auto* edges = UInt64Array::Create(dataStructure, edgesPath.getTargetName(), DataStoreUtilities::CreateDataStore<uint64>(dataStructure, edgesPath, {3}, {2}), geometry->getId());
    REQUIRE(edges != nullptr);
    auto edgeResult = edges->getDataStoreRef().copyFromBuffer(0, edgeValues);
    SIMPLNX_RESULT_REQUIRE_VALID(edgeResult);
    geometry->setEdgeList(*edges);
    edgeId = edges->getId();
  }
  LaplacianSmoothingInputValues inputValues{};
  inputValues.pTriangleGeometryDataPath = geometryPath;
  inputValues.pSurfaceMeshNodeTypeArrayPath = nodePath;
  inputValues.pIterationSteps = cancelled ? 1 : 0;
  const std::atomic_bool shouldCancel = cancelled;
  IFilter::MessageHandler messageHandler;
  auto result = LaplacianSmoothing(dataStructure, &inputValues, shouldCancel, messageHandler)();
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(geometry->getEdgeListId() == edgeId);
  CHECK(geometry->getEdgeAttributeMatrixId() == edgeDataId);
  CHECK(geometry->getEdgeAttributeMatrix() == edgeData);
  CHECK(dataStructure.getId(DataPath({"Mesh", "CustomEdges"})) == edgeDataId);
  CHECK(dataStructure.getId(markerPath) == markerId);
  std::array<int32, 3> actualMarkers{};
  auto markerRead = marker->getDataStoreRef().copyIntoBuffer(0, actualMarkers);
  SIMPLNX_RESULT_REQUIRE_VALID(markerRead);
  CHECK(actualMarkers == markers);
  std::array<float32, 9> actualVertices{};
  auto vertexRead = vertices->getDataStoreRef().copyIntoBuffer(0, actualVertices);
  SIMPLNX_RESULT_REQUIRE_VALID(vertexRead);
  CHECK(actualVertices == coordinates);
  if(existingEdges)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64Array>(edgesPath));
    CHECK(dataStructure.getId(edgesPath) == edgeId);
    std::array<uint64, 6> actualEdges{};
    auto edgeRead = dataStructure.getDataRefAs<UInt64Array>(edgesPath).getDataStoreRef().copyIntoBuffer(0, actualEdges);
    SIMPLNX_RESULT_REQUIRE_VALID(edgeRead);
    CHECK(actualEdges == edgeValues);
  }
  else
  {
    CHECK(dataStructure.getData(edgesPath) == nullptr);
    CHECK(geometry->getEdges() == nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
