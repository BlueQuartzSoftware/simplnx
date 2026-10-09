#include <catch2/catch.hpp>

#include <array>
#include <memory>
#include <vector>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/SliceTriangleGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const nx::core::DataPath k_InputTriangleGeometryPath = DataPath({"Input Triangle Geometry"});
const nx::core::DataPath k_RegionIdsPath = DataPath({"Input Triangle Geometry", "FaceData", "Part Number"});
const nx::core::DataPath k_ExemplarEdgeGeometryPath = DataPath({"Exemplar Slice Geometry"});

const nx::core::DataPath k_ComputedEdgeGeometryPath = DataPath({"Output Edge Geometry"});
const DataObjectNameParameter::ValueType k_EdgeData("Edge Data");
const DataObjectNameParameter::ValueType k_SliceData("Slice Feature Data");
const DataObjectNameParameter::ValueType k_SliceIds("Slice Ids");
const DataObjectNameParameter::ValueType k_RegionIdsName("Part Number");

void BuildTriangleGeom(DataStructure& dataStructure, const std::vector<float32>& verts, const std::vector<IGeometry::MeshIndexType>& faces)
{
  REQUIRE(verts.size() % 3 == 0);
  REQUIRE(faces.size() % 3 == 0);
  auto* triangleGeom = TriangleGeom::Create(dataStructure, k_InputTriangleGeometryPath.getTargetName());
  REQUIRE(triangleGeom != nullptr);

  auto* faceData = AttributeMatrix::Create(dataStructure, "FaceData", {faces.size() / 3}, triangleGeom->getId());
  REQUIRE(faceData != nullptr);
  triangleGeom->setFaceAttributeMatrix(*faceData);
  auto* vertexData = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, {verts.size() / 3}, triangleGeom->getId());
  REQUIRE(vertexData != nullptr);
  triangleGeom->setVertexAttributeMatrix(*vertexData);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{verts.size() / 3}, std::vector<usize>{3}, 0.0f);
  auto* vertexList = IGeometry::SharedVertexList::Create(dataStructure, "Vertices", std::move(vertexStore), vertexData->getId());
  REQUIRE(vertexList != nullptr);
  for(usize valueIdx = 0; valueIdx < verts.size(); valueIdx++)
  {
    (*vertexList)[valueIdx] = verts[valueIdx];
  }
  triangleGeom->setVertices(*vertexList);

  auto faceStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{faces.size() / 3}, std::vector<usize>{3}, 0);
  auto* faceList = IGeometry::SharedFaceList::Create(dataStructure, "Faces", std::move(faceStore), faceData->getId());
  REQUIRE(faceList != nullptr);
  for(usize valueIdx = 0; valueIdx < faces.size(); valueIdx++)
  {
    (*faceList)[valueIdx] = faces[valueIdx];
  }
  triangleGeom->setFaceList(*faceList);
}

Arguments CreateSliceArguments(const SliceTriangleGeometryFilter& filter)
{
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(SliceTriangleGeometryFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(k_InputTriangleGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(k_ComputedEdgeGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_EdgeData));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceIdArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_SliceIds));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_SliceData));
  return args;
}
} // namespace

TEST_CASE("SliceTriangleGeometryFilter: Adjacent triangles share one slice vertex", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  DataStructure dataStructure;
  BuildTriangleGeom(dataStructure, {0.5f, 1.25f, 0.0f, 2.0f, 3.5f, 1.0f, 3.0f, 0.75f, 0.0f, -1.0f, 2.75f, 0.0f}, {0, 1, 2, 1, 0, 3});
  auto* faceData = dataStructure.getDataAs<AttributeMatrix>(k_InputTriangleGeometryPath.createChildPath("FaceData"));
  REQUIRE(faceData != nullptr);
  auto* regionIds = UnitTest::CreateTestDataArray<int32>(dataStructure, k_RegionIdsName, {2}, {1}, faceData->getId());
  REQUIRE(regionIds != nullptr);
  (*regionIds)[0] = 7;
  (*regionIds)[1] = 11;

  const SliceTriangleGeometryFilter filter;
  Arguments args = CreateSliceArguments(filter);
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zstart_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zend_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(true));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_RegionIdArrayPath_Key, std::make_any<DataPath>(k_RegionIdsPath));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(k_ComputedEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  REQUIRE(edgeGeom->getNumberOfVertices() == 3);
  REQUIRE(edgeGeom->getNumberOfEdges() == 2);
  const auto& edges = edgeGeom->getEdgesRef();
  const std::array<IGeometry::MeshIndexType, 4> expectedEdges = {0, 1, 0, 2};
  for(usize valueIdx = 0; valueIdx < expectedEdges.size(); valueIdx++)
  {
    REQUIRE(edges[valueIdx] == expectedEdges[valueIdx]);
  }

  // Each intersection is 0.1 of the distance from a lower vertex to b.
  const std::array<float64, 9> expectedVertices = {0.65, 1.475, 0.1, 2.9, 1.025, 0.1, -0.7, 2.825, 0.1};
  const auto& vertices = edgeGeom->getVerticesRef();
  for(usize valueIdx = 0; valueIdx < expectedVertices.size(); valueIdx++)
  {
    CAPTURE(valueIdx);
    REQUIRE(vertices[valueIdx] == Approx(expectedVertices[valueIdx]).margin(1.0e-5));
  }

  const DataPath edgeDataPath = k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData);
  const auto* sliceIds = dataStructure.getDataAs<Int32Array>(edgeDataPath.createChildPath(k_SliceIds));
  REQUIRE(sliceIds != nullptr);
  REQUIRE(sliceIds->getNumberOfTuples() == 2);
  REQUIRE((*sliceIds)[0] == 0);
  REQUIRE((*sliceIds)[1] == 0);
  const auto* edgeRegionIds = dataStructure.getDataAs<Int32Array>(edgeDataPath.createChildPath(k_RegionIdsName));
  REQUIRE(edgeRegionIds != nullptr);
  REQUIRE(edgeRegionIds->getNumberOfTuples() == 2);
  REQUIRE((*edgeRegionIds)[0] == 7);
  REQUIRE((*edgeRegionIds)[1] == 11);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SliceTriangleGeometryFilter: Full Range includes the top slice", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  DataStructure dataStructure;
  BuildTriangleGeom(dataStructure, {0.25f, 0.5f, 0.0f, 1.5f, 0.75f, 1.0f, 0.5f, 2.0f, 1.0f}, {0, 1, 2});

  const SliceTriangleGeometryFilter filter;
  Arguments args = CreateSliceArguments(filter);
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(false));

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(k_ComputedEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  REQUIRE(edgeGeom->getNumberOfEdges() == 10);
  REQUIRE(edgeGeom->getNumberOfVertices() == 20);
  const auto* sliceIds = dataStructure.getDataAs<Int32Array>(k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_SliceIds));
  REQUIRE(sliceIds != nullptr);
  REQUIRE(sliceIds->getNumberOfTuples() == 10);
  for(usize edgeIdx = 0; edgeIdx < 10; edgeIdx++)
  {
    REQUIRE((*sliceIds)[edgeIdx] == static_cast<int32>(edgeIdx + 1));
  }
  const auto* sliceData = dataStructure.getDataAs<AttributeMatrix>(k_ComputedEdgeGeometryPath.createChildPath(k_SliceData));
  REQUIRE(sliceData != nullptr);
  REQUIRE(sliceData->getNumberOfTuples() == 11);

  const auto& edges = edgeGeom->getEdgesRef();
  const auto& vertices = edgeGeom->getVerticesRef();
  // At z = 0.1k, a + 0.1k(b - a) and a + 0.1k(c - a) give the endpoints.
  for(usize sliceIdx = 1; sliceIdx <= 9; sliceIdx++)
  {
    CAPTURE(sliceIdx);
    const float64 k = static_cast<float64>(sliceIdx);
    const std::array<float64, 3> expectedStart = {0.25 + 0.125 * k, 0.5 + 0.025 * k, 0.1 * k};
    const std::array<float64, 3> expectedEnd = {0.25 + 0.025 * k, 0.5 + 0.15 * k, 0.1 * k};
    const usize startVertex = edges[2 * (sliceIdx - 1)];
    const usize endVertex = edges[2 * (sliceIdx - 1) + 1];
    REQUIRE(startVertex < 20);
    REQUIRE(endVertex < 20);
    for(usize compIdx = 0; compIdx < 3; compIdx++)
    {
      REQUIRE(vertices[3 * startVertex + compIdx] == Approx(expectedStart[compIdx]).margin(1.0e-5));
      REQUIRE(vertices[3 * endVertex + compIdx] == Approx(expectedEnd[compIdx]).margin(1.0e-5));
    }
  }

  const std::array<float32, 3> expectedTopStart = {1.5f, 0.75f, 1.0f};
  const std::array<float32, 3> expectedTopEnd = {0.5f, 2.0f, 1.0f};
  REQUIRE(edges[18] < 20);
  REQUIRE(edges[19] < 20);
  for(usize compIdx = 0; compIdx < 3; compIdx++)
  {
    REQUIRE(vertices[3 * edges[18] + compIdx] == expectedTopStart[compIdx]);
    REQUIRE(vertices[3 * edges[19] + compIdx] == expectedTopEnd[compIdx]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SliceTriangleGeometryFilter: Full Range includes the top slice for a large span", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  DataStructure dataStructure;
  BuildTriangleGeom(dataStructure, {0.25f, 0.5f, 0.0f, 1.5f, 0.75f, 1000.0f, 0.5f, 2.0f, 1000.0f}, {0, 1, 2});

  const SliceTriangleGeometryFilter filter;
  Arguments args = CreateSliceArguments(filter);
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(false));

  // 0.1f is 0.100000001490116, so span/spacing = 9999.99985; a fixed 1e-5 tolerance drops the top slice.
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(k_ComputedEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  // Plane 0 touches only vertex a; planes 1 through 10000 each produce one edge.
  REQUIRE(edgeGeom->getNumberOfEdges() == 10000);
  const auto* sliceData = dataStructure.getDataAs<AttributeMatrix>(k_ComputedEdgeGeometryPath.createChildPath(k_SliceData));
  REQUIRE(sliceData != nullptr);
  REQUIRE(sliceData->getNumberOfTuples() == 10001);

  const usize lastEdgeIdx = edgeGeom->getNumberOfEdges() - 1;
  const auto* sliceIds = dataStructure.getDataAs<Int32Array>(k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_SliceIds));
  REQUIRE(sliceIds != nullptr);
  REQUIRE(sliceIds->getNumberOfTuples() == 10000);
  REQUIRE((*sliceIds)[lastEdgeIdx] == 10000);

  const auto& edges = edgeGeom->getEdgesRef();
  const auto& vertices = edgeGeom->getVerticesRef();
  const usize startVertex = edges[2 * lastEdgeIdx];
  const usize endVertex = edges[2 * lastEdgeIdx + 1];
  REQUIRE(startVertex < edgeGeom->getNumberOfVertices());
  REQUIRE(endVertex < edgeGeom->getNumberOfVertices());
  // The last edge lies exactly along b-c on the top plane at z = 1000.
  const std::array<float32, 3> expectedTopStart = {1.5f, 0.75f, 1000.0f};
  const std::array<float32, 3> expectedTopEnd = {0.5f, 2.0f, 1000.0f};
  for(usize compIdx = 0; compIdx < 3; compIdx++)
  {
    REQUIRE(vertices[3 * startVertex + compIdx] == expectedTopStart[compIdx]);
    REQUIRE(vertices[3 * endVertex + compIdx] == expectedTopEnd[compIdx]);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SliceTriangleGeometryFilter: An empty geometry produces a warning", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  DataStructure dataStructure;
  BuildTriangleGeom(dataStructure, {0.25f, 0.5f, 0.0f, 1.5f, 0.75f, 1000.0f, 0.5f, 2.0f, 1000.0f}, {});

  const SliceTriangleGeometryFilter filter;
  Arguments args = CreateSliceArguments(filter);
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -62105);

  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(k_ComputedEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  REQUIRE(edgeGeom->getNumberOfVertices() == 0);
  REQUIRE(edgeGeom->getNumberOfEdges() == 0);
  const auto* sliceData = dataStructure.getDataAs<AttributeMatrix>(k_ComputedEdgeGeometryPath.createChildPath(k_SliceData));
  REQUIRE(sliceData != nullptr);
  REQUIRE(sliceData->getNumberOfTuples() == 0);
  const auto* sliceIds = dataStructure.getDataAs<Int32Array>(k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_SliceIds));
  REQUIRE(sliceIds != nullptr);
  REQUIRE(sliceIds->getNumberOfTuples() == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SliceTriangleGeometryFilter: Slices a QuickSurfaceMesh output in a pipeline", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  const DataPath imageGeomPath({"Image Geometry"});
  const DataPath cellDataPath = imageGeomPath.createChildPath("Cell Data");
  const DataPath featureIdsPath = cellDataPath.createChildPath("FeatureIds");
  auto* imageGeom = ImageGeom::Create(dataStructure, imageGeomPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({4, 4, 4});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  auto* cellData = AttributeMatrix::Create(dataStructure, cellDataPath.getTargetName(), {4, 4, 4}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, featureIdsPath.getTargetName(), {4, 4, 4}, {1}, cellData->getId());
  REQUIRE(featureIds != nullptr);
  // Split the voxels along x into two features with a vertical interface.
  for(usize voxelIdx = 0; voxelIdx < featureIds->getNumberOfTuples(); voxelIdx++)
  {
    (*featureIds)[voxelIdx] = voxelIdx % 4 < 2 ? 1 : 2;
  }

  QuickSurfaceMeshFilter meshFilter;
  Arguments meshArgs = meshFilter.getDefaultArguments();
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(imageGeomPath));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_InputTriangleGeometryPath));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("VertexData"));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("FaceData"));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>());
  meshArgs.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>());

  const SliceTriangleGeometryFilter sliceFilter;
  Arguments sliceArgs = CreateSliceArguments(sliceFilter);
  sliceArgs.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  sliceArgs.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(0.5f));
  sliceArgs.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(false));

  Pipeline pipeline;
  REQUIRE(pipeline.push_back(std::make_unique<QuickSurfaceMeshFilter>(), meshArgs));
  REQUIRE(pipeline.push_back(std::make_unique<SliceTriangleGeometryFilter>(), sliceArgs));

  // Preflight creates placeholder output geometries; execute starts with the original input.
  DataStructure preflightDataStructure = dataStructure;
  REQUIRE(pipeline.preflight(preflightDataStructure, false));
  const auto* preflightTriangleGeom = preflightDataStructure.getDataAs<TriangleGeom>(k_InputTriangleGeometryPath);
  REQUIRE(preflightTriangleGeom != nullptr);
  REQUIRE(preflightTriangleGeom->getNumberOfFaces() == 0);

  REQUIRE(pipeline.execute(dataStructure, false));
  const auto* sliceNode = dynamic_cast<const PipelineFilter*>(pipeline.at(1));
  REQUIRE(sliceNode != nullptr);
  for(const auto& warning : sliceNode->getWarnings())
  {
    INFO(warning.message);
    REQUIRE(warning.code != -62105);
  }

  const auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(k_InputTriangleGeometryPath);
  REQUIRE(triangleGeom != nullptr);
  REQUIRE(triangleGeom->getNumberOfFaces() > 0);
  const auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(k_ComputedEdgeGeometryPath);
  REQUIRE(edgeGeom != nullptr);
  REQUIRE(edgeGeom->getNumberOfEdges() > 0);
  const auto* sliceData = dataStructure.getDataAs<AttributeMatrix>(k_ComputedEdgeGeometryPath.createChildPath(k_SliceData));
  REQUIRE(sliceData != nullptr);
  REQUIRE(sliceData->getNumberOfTuples() > 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SliceTriangleGeometryFilter: Non-positive Slice Spacing is rejected", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  const float32 spacing = GENERATE(0.0f, -0.5f);
  CAPTURE(spacing);
  DataStructure dataStructure;
  BuildTriangleGeom(dataStructure, {0.25f, 0.5f, 0.0f, 1.5f, 0.75f, 1.0f, 0.5f, 2.0f, 1.0f}, {0, 1, 2});

  const SliceTriangleGeometryFilter filter;
  Arguments args = CreateSliceArguments(filter);
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(spacing));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -62103);
}

TEST_CASE("SliceTriangleGeometryFilter: Region Ids must match the face count", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  DataStructure dataStructure;
  BuildTriangleGeom(dataStructure, {0.5f, 1.25f, 0.0f, 2.0f, 3.5f, 1.0f, 3.0f, 0.75f, 0.0f, -1.0f, 2.75f, 0.0f}, {0, 1, 2, 1, 0, 3});
  auto* regionIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "Region Ids", {1}, {1});
  REQUIRE(regionIds != nullptr);
  (*regionIds)[0] = 5;

  const SliceTriangleGeometryFilter filter;
  Arguments args = CreateSliceArguments(filter);
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zstart_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zend_Key, std::make_any<float32>(0.5f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(1.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(true));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_RegionIdArrayPath_Key, std::make_any<DataPath>(DataPath({"Region Ids"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -62104);
}

TEST_CASE("SimplnxCore::SliceTriangleGeometryFilter: Valid Filter Execution", "[SimplnxCore][SliceTriangleGeometryFilter]")
{
  /// The test data set was reviewed manually by MAJ and found to be correct in output to the
  /// the best of our abilities. This is needed because DREAM3D-NX did not have
  /// this functionality and so we have nothing to compare against.

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_0_SurfaceMesh_Test_Files_v2.tar.gz", "7_0_SurfaceMesh_Test_Files");
  auto baseDataFilePath = fs::path(fmt::format("{}/7_0_SurfaceMesh_Test_Files/7_0_SurfaceMesh_Test_Files.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  SliceTriangleGeometryFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zstart_Key, std::make_any<float32>(0.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zend_Key, std::make_any<float32>(0.0f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceRange_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(true));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(k_InputTriangleGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_RegionIdArrayPath_Key, std::make_any<DataPath>(k_RegionIdsPath));

  args.insertOrAssign(SliceTriangleGeometryFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(k_ComputedEdgeGeometryPath));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_EdgeData));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceIdArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_SliceIds));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_SliceData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  fs::path testFileOutputPath(fmt::format("{}/slice_triangle_geometry.dream3d", unit_test::k_BinaryTestOutputDir));
  std::cout << "Writing Output file: " << testFileOutputPath << std::endl;
  UnitTest::WriteTestDataStructure(dataStructure, testFileOutputPath);
#endif

  // Compare the exemplar and the computed outputs
  {
    auto exemplarGeom = dataStructure.getDataAs<IGeometry>(k_ExemplarEdgeGeometryPath);
    auto computedGeom = dataStructure.getDataAs<IGeometry>(k_ComputedEdgeGeometryPath);
    REQUIRE(UnitTest::CompareIGeometry(exemplarGeom, computedGeom));
  }
  {
    DataPath exemplarDataArray = k_ExemplarEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_SliceIds);
    DataPath computedDataArray = k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_SliceIds);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  {
    DataPath exemplarDataArray = k_ExemplarEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
    DataPath computedDataArray = k_ComputedEdgeGeometryPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  // Check endpoint geometry beyond the edge count checked by CompareIGeometry.
  {
    const auto* exemplarGeom = dataStructure.getDataAs<EdgeGeom>(k_ExemplarEdgeGeometryPath);
    const auto* computedGeom = dataStructure.getDataAs<EdgeGeom>(k_ComputedEdgeGeometryPath);
    REQUIRE(exemplarGeom != nullptr);
    REQUIRE(computedGeom != nullptr);
    REQUIRE(exemplarGeom->getNumberOfEdges() == computedGeom->getNumberOfEdges());
    REQUIRE(exemplarGeom->getEdges() != nullptr);
    REQUIRE(computedGeom->getEdges() != nullptr);
    REQUIRE(exemplarGeom->getVertices() != nullptr);
    REQUIRE(computedGeom->getVertices() != nullptr);
    const auto& exemplarEdges = exemplarGeom->getEdges()->getDataStoreRef();
    const auto& computedEdges = computedGeom->getEdges()->getDataStoreRef();
    const auto& exemplarVertices = exemplarGeom->getVertices()->getDataStoreRef();
    const auto& computedVertices = computedGeom->getVertices()->getDataStoreRef();
    usize mismatches = 0;
    for(usize edgeIdx = 0; edgeIdx < exemplarGeom->getNumberOfEdges(); edgeIdx++)
    {
      for(usize endpointIdx = 0; endpointIdx < 2; endpointIdx++)
      {
        const usize exemplarVertexIdx = exemplarEdges[2 * edgeIdx + endpointIdx];
        const usize computedVertexIdx = computedEdges[2 * edgeIdx + endpointIdx];
        for(usize compIdx = 0; compIdx < 3; compIdx++)
        {
          const auto exemplarValue = exemplarVertices[3 * exemplarVertexIdx + compIdx];
          const auto computedValue = computedVertices[3 * computedVertexIdx + compIdx];
          if(computedValue != Approx(exemplarValue).margin(1.0e-4))
          {
            if(mismatches < 5)
            {
              UNSCOPED_INFO("Edge " << edgeIdx << ", endpoint " << endpointIdx << ", component " << compIdx << ": exemplar " << exemplarValue << ", computed " << computedValue);
            }
            mismatches++;
          }
        }
      }
    }
    REQUIRE(mismatches == 0);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SliceTriangleGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SliceTriangleGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SliceTriangleGeometryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<SliceTriangleGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(SliceTriangleGeometryFilter::k_SliceRange_Key) == 0);
      CHECK(args.value<float32>(SliceTriangleGeometryFilter::k_Zstart_Key) == Approx(2.5f));
      CHECK(args.value<float32>(SliceTriangleGeometryFilter::k_Zend_Key) == Approx(2.5f));
      CHECK(args.value<float32>(SliceTriangleGeometryFilter::k_SliceResolution_Key) == Approx(2.5f));
      CHECK(args.value<bool>(SliceTriangleGeometryFilter::k_HaveRegionIds_Key) == true);
      CHECK(args.value<DataPath>(SliceTriangleGeometryFilter::k_RegionIdArrayPath_Key) == DataPath({"DataContainer", "CellData", "RegionIds"}));
      CHECK(args.value<DataPath>(SliceTriangleGeometryFilter::k_TriangleGeometryDataPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(SliceTriangleGeometryFilter::k_OutputEdgeGeometryPath_Key) == DataPath({"SliceDataContainer"}));
      CHECK(args.value<std::string>(SliceTriangleGeometryFilter::k_EdgeAttributeMatrixName_Key) == "EdgeData");
      CHECK(args.value<std::string>(SliceTriangleGeometryFilter::k_SliceAttributeMatrixName_Key) == "SliceData");
      CHECK(args.value<std::string>(SliceTriangleGeometryFilter::k_SliceIdArrayName_Key) == "SliceIds");
    }
  }
}
