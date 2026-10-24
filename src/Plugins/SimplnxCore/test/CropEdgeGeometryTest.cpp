#include "SimplnxCore/Filters/Algorithms/CropEdgeGeometry.hpp"
#include "SimplnxCore/Filters/CropEdgeGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include <simplnx/Parameters/ChoicesParameter.hpp>

#include <algorithm>
#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
inline constexpr StringLiteral k_EdgeGeometry("Edge Geometry");
inline constexpr StringLiteral k_CroppedEdgeGeometry("Cropped Edge Geometry");
inline constexpr StringLiteral k_VerticesName("Vertices");
inline constexpr StringLiteral k_EdgesName("Edges");
inline constexpr StringLiteral k_UInt32ArrayName("UInt32Array");
inline constexpr StringLiteral k_Int32ArrayName("Int32Array");
inline constexpr StringLiteral k_Int64ArrayName("Int64Array");
const DataPath k_UInt32ArrayPath = DataPath({k_EdgeGeometry}).createChildPath(Constants::k_VertexData).createChildPath(k_UInt32ArrayName);
const DataPath k_Int32ArrayPath = DataPath({k_EdgeGeometry}).createChildPath(Constants::k_Edge_Data).createChildPath(k_Int32ArrayName);
const DataPath k_Int64ArrayPath = DataPath({k_EdgeGeometry}).createChildPath(Constants::k_Edge_Data).createChildPath(k_Int64ArrayName);

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  EdgeGeom* edgeGeom = EdgeGeom::Create(dataStructure, k_EdgeGeometry);

  Float32Array* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, k_VerticesName, {4}, {3}, edgeGeom->getId());
  auto& verticesRef = vertices->getDataStoreRef();
  verticesRef[0] = 0;
  verticesRef[1] = 0;
  verticesRef[2] = 0;
  verticesRef[3] = 1;
  verticesRef[4] = 2;
  verticesRef[5] = -2;
  verticesRef[6] = 3;
  verticesRef[7] = 1;
  verticesRef[8] = -2;
  verticesRef[9] = 2;
  verticesRef[10] = -1;
  verticesRef[11] = 0;
  edgeGeom->setVertices(*vertices);

  IGeometry::SharedEdgeList* edges = UnitTest::CreateTestDataArray<uint64>(dataStructure, k_EdgesName, {4}, {2}, edgeGeom->getId());
  auto& edgesRef = edges->getDataStoreRef();
  edgesRef[0] = 0;
  edgesRef[1] = 1;
  edgesRef[2] = 1;
  edgesRef[3] = 2;
  edgesRef[4] = 2;
  edgesRef[5] = 3;
  edgesRef[6] = 3;
  edgesRef[7] = 0;
  edgeGeom->setEdgeList(*edges);

  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, Constants::k_VertexData, {4}, edgeGeom->getId());
  edgeGeom->setVertexAttributeMatrix(*vertexDataPtr);

  auto* edgeDataPtr = AttributeMatrix::Create(dataStructure, Constants::k_Edge_Data, {4}, edgeGeom->getId());
  edgeGeom->setEdgeAttributeMatrix(*edgeDataPtr);

  UInt32Array* uint32Array = UnitTest::CreateTestDataArray<uint32>(dataStructure, "UInt32Array", {4}, {1}, vertexDataPtr->getId());
  auto& uint32ArrayRef = uint32Array->getDataStoreRef();
  uint32ArrayRef[0] = 8;
  uint32ArrayRef[1] = 3;
  uint32ArrayRef[2] = 893;
  uint32ArrayRef[3] = 327;

  Int32Array* int32Array = UnitTest::CreateTestDataArray<int32>(dataStructure, "Int32Array", {4}, {1}, edgeDataPtr->getId());
  auto& int32ArrayRef = int32Array->getDataStoreRef();
  int32ArrayRef[0] = 12;
  int32ArrayRef[1] = 56;
  int32ArrayRef[2] = 2;
  int32ArrayRef[3] = 91;

  Int64Array* int64Array = UnitTest::CreateTestDataArray<int64>(dataStructure, "Int64Array", {4}, {1}, edgeDataPtr->getId());
  auto& int64ArrayRef = int64Array->getDataStoreRef();
  int64ArrayRef[0] = 24;
  int64ArrayRef[1] = 124;
  int64ArrayRef[2] = 352;
  int64ArrayRef[3] = 786;

  return dataStructure;
}
// Two crossing edges share O; the crop variant keeps A-B as the second edge.
DataStructure CreateSharedOutsideVertexData(bool cropCase = false)
{
  DataStructure dataStructure;
  auto* geom = EdgeGeom::Create(dataStructure, k_EdgeGeometry);
  REQUIRE(geom != nullptr);
  const DataPath geomPath({k_EdgeGeometry});
  auto* vertices = Float32Array::Create(dataStructure, k_VerticesName, DataStoreUtilities::CreateDataStore<float32>(dataStructure, geomPath.createChildPath(k_VerticesName), {3}, {3}), geom->getId());
  REQUIRE(vertices != nullptr);
  const std::vector<float32> coordinates{0, 0, 0, 0.5F, 1, -1, 2, 4, 6};
  for(usize i = 0; i < coordinates.size(); ++i)
  {
    (*vertices)[i] = coordinates[i];
  }
  geom->setVertices(*vertices);
  auto* edges = UInt64Array::Create(dataStructure, k_EdgesName, DataStoreUtilities::CreateDataStore<uint64>(dataStructure, geomPath.createChildPath(k_EdgesName), {2}, {2}), geom->getId());
  REQUIRE(edges != nullptr);
  const std::vector<uint64> connectivity = cropCase ? std::vector<uint64>{0, 2, 0, 1} : std::vector<uint64>{0, 2, 1, 2};
  for(usize i = 0; i < connectivity.size(); ++i)
  {
    (*edges)[i] = connectivity[i];
  }
  geom->setEdgeList(*edges);
  auto* vertexData = AttributeMatrix::Create(dataStructure, Constants::k_VertexData, {3}, geom->getId());
  auto* edgeData = AttributeMatrix::Create(dataStructure, Constants::k_Edge_Data, {2}, geom->getId());
  REQUIRE(vertexData != nullptr);
  REQUIRE(edgeData != nullptr);
  geom->setVertexAttributeMatrix(*vertexData);
  geom->setEdgeAttributeMatrix(*edgeData);
  auto* vertexValues = Int32Array::Create(
      dataStructure, "Values", DataStoreUtilities::CreateDataStore<int32>(dataStructure, geomPath.createChildPath(Constants::k_VertexData).createChildPath("Values"), {3}, {1}), vertexData->getId());
  auto* edgeValues = Int32Array::Create(
      dataStructure, "Values", DataStoreUtilities::CreateDataStore<int32>(dataStructure, geomPath.createChildPath(Constants::k_Edge_Data).createChildPath("Values"), {2}, {1}), edgeData->getId());
  REQUIRE(vertexValues != nullptr);
  REQUIRE(edgeValues != nullptr);
  (*vertexValues)[0] = 7;
  (*vertexValues)[1] = 11;
  (*vertexValues)[2] = 13;
  (*edgeValues)[0] = 21;
  (*edgeValues)[1] = 34;
  return dataStructure;
}

Arguments SharedOutsideVertexArguments(bool cropCase = false)
{
  Arguments args;
  args.insert(CropEdgeGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CropEdgeGeometryFilter::k_SelectedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_EdgeGeometry})));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CreatedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_CroppedEdgeGeometry})));
  args.insertOrAssign(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropEdgeGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float32>>(std::vector<float32>{-1, 0, 0}));
  args.insertOrAssign(CropEdgeGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 0, 0}));
  const auto behavior = cropCase ? CropEdgeGeometry::BoundaryIntersectionBehavior::IgnoreEdge : CropEdgeGeometry::BoundaryIntersectionBehavior::InterpolateOutsideVertex;
  args.insertOrAssign(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(behavior)));
  return args;
}

DataStructure CreateFeatureCropFixture()
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* edgeGeom = EdgeGeom::Create(dataStructure, "Edge");
  REQUIRE(edgeGeom != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(dataStructure, "Vertices", {5}, {3}, edgeGeom->getId());
  REQUIRE(vertices != nullptr);
  vertices->fill(0);
  for(usize i = 0; i < 5; ++i)
  {
    (*vertices)[3 * i] = static_cast<float32>(i);
  }
  edgeGeom->setVertices(*vertices);
  auto* edges = UnitTest::CreateTestDataArray<uint64>(dataStructure, "Edges", {4}, {2}, edgeGeom->getId());
  REQUIRE(edges != nullptr);
  for(usize i = 0; i < 4; ++i)
  {
    (*edges)[2 * i] = i;
    (*edges)[2 * i + 1] = i + 1;
  }
  edgeGeom->setEdgeList(*edges);
  auto* vertexData = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{5}, edgeGeom->getId());
  REQUIRE(vertexData != nullptr);
  edgeGeom->setVertexAttributeMatrix(*vertexData);
  auto* edgeData = AttributeMatrix::Create(dataStructure, "Edge Data", ShapeType{4}, edgeGeom->getId());
  REQUIRE(edgeData != nullptr);
  edgeGeom->setEdgeAttributeMatrix(*edgeData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {4}, {1}, edgeData->getId());
  REQUIRE(featureIds != nullptr);
  for(usize i = 0; i < 4; ++i)
  {
    (*featureIds)[i] = static_cast<int32>(i + 1);
  }

  auto* featureData = AttributeMatrix::Create(dataStructure, "Edge Feature Data", ShapeType{5}, edgeGeom->getId());
  REQUIRE(featureData != nullptr);
  auto* numElements = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureData->getId());
  REQUIRE(numElements != nullptr);
  numElements->fill(1);
  (*numElements)[0] = 0;
  auto* neighbors = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{5}, featureData->getId());
  REQUIRE(neighbors != nullptr);
  neighbors->setLists({{}, {2}, {1, 3}, {2, 4}, {3}});
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{5}, {"Background", "One", "Two", "Three", "Four"}, featureData->getId()) != nullptr);

  auto* ensembleData = AttributeMatrix::Create(dataStructure, "Edge Ensemble Data", ShapeType{2}, edgeGeom->getId());
  REQUIRE(ensembleData != nullptr);
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleData->getId());
  REQUIRE(crystalStructures != nullptr);
  (*crystalStructures)[0] = 999;
  (*crystalStructures)[1] = 1;
  return dataStructure;
}

/**
 * @brief Keeps features 3 and 4, clipping the first retained edge at x = 2.1.
 * @param clear Whether to clear the destination Feature Attribute Matrix.
 * @param renumber Stored renumber setting, effective only when clear is true.
 * @return Arguments that preserve the source and create the Cropped geometry.
 */
Arguments MakeFeatureCropArgs(bool clear, bool renumber)
{
  Arguments args;
  args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float32>>(std::vector<float32>{2.1F, 0, 0}));
  args.insert(CropEdgeGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float32>>(std::vector<float32>{4, 0, 0}));
  args.insert(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key,
              std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::InterpolateOutsideVertex)));
  args.insert(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_SelectedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({"Edge"})));
  args.insert(CropEdgeGeometryFilter::k_CreatedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({"Cropped"})));
  args.insert(CropEdgeGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(clear));
  args.insert(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(renumber));
  args.insert(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Edge Feature Data"})));
  args.insert(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Edge Data", "FeatureIds"})));
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Filter Error", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  UnitTest::LoadPlugins();
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;
  args.insert(CropEdgeGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));

  const std::vector<float32> k_MinCoords{0.5, 0.5, -0.5};
  const std::vector<float32> k_MaxCoords{1.5, 2, 0.5};

  SECTION("X")
  {
    args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
    args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  }
  SECTION("Y")
  {
    args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(true));
    args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  }
  SECTION("Z")
  {
    args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(true));
  }

  args.insert(CropEdgeGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float32>>(k_MinCoords));
  args.insert(CropEdgeGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float32>>(k_MaxCoords));
  args.insert(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::FilterError)));
  args.insert(CropEdgeGeometryFilter::k_SelectedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_EdgeGeometry})));
  args.insert(CropEdgeGeometryFilter::k_CreatedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_CroppedEdgeGeometry})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors()[0].code == to_underlying(CropEdgeGeometry::ErrorCodes::OutsideVertexError));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Ignore Edges", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  UnitTest::LoadPlugins();
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;
  args.insert(CropEdgeGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));

  const std::vector<float32> k_MinCoords{-0.5, -0.5, -0.5};
  const std::vector<float32> k_MaxCoords{1.5, 2.5, 0.5};

  args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float32>>(k_MinCoords));
  args.insert(CropEdgeGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float32>>(k_MaxCoords));
  args.insert(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::IgnoreEdge)));
  args.insert(CropEdgeGeometryFilter::k_SelectedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_EdgeGeometry})));
  args.insert(CropEdgeGeometryFilter::k_CreatedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_CroppedEdgeGeometry})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(DataPath({k_EdgeGeometry})));
  auto& edgeGeom = dataStructure.getDataRefAs<EdgeGeom>(DataPath({k_EdgeGeometry}));
  Float32Array& vertices = edgeGeom.getVerticesRef();
  UInt64Array& edges = edgeGeom.getEdgesRef();
  REQUIRE(vertices.getNumberOfTuples() == 2);
  REQUIRE(edges.getNumberOfTuples() == 1);
  REQUIRE(vertices[0] == 0);
  REQUIRE(vertices[1] == 0);
  REQUIRE(vertices[2] == 0);
  REQUIRE(vertices[3] == 1);
  REQUIRE(vertices[4] == 2);
  REQUIRE(vertices[5] == -2);
  REQUIRE(edges[0] == 0);
  REQUIRE(edges[1] == 1);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(k_UInt32ArrayPath));
  auto& uint32Array = dataStructure.getDataRefAs<UInt32Array>(k_UInt32ArrayPath);
  REQUIRE(uint32Array.getNumberOfTuples() == 2);
  REQUIRE(uint32Array[0] == 8);
  REQUIRE(uint32Array[1] == 3);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_Int32ArrayPath));
  auto& int32Array = dataStructure.getDataRefAs<Int32Array>(k_Int32ArrayPath);
  REQUIRE(int32Array.getNumberOfTuples() == 1);
  REQUIRE(int32Array[0] == 12);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(k_Int64ArrayPath));
  auto& int64Array = dataStructure.getDataRefAs<Int64Array>(k_Int64ArrayPath);
  REQUIRE(int64Array.getNumberOfTuples() == 1);
  REQUIRE(int64Array[0] == 24);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Interpolate Outside Vertices", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  UnitTest::LoadPlugins();
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;
  args.insert(CropEdgeGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));

  const std::vector<float32> k_MinCoords{-0.5, -0.5, -0.5};
  const std::vector<float32> k_MaxCoords{1.5, 2.5, 0.5};

  args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float32>>(k_MinCoords));
  args.insert(CropEdgeGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float32>>(k_MaxCoords));
  args.insert(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key,
              std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::InterpolateOutsideVertex)));
  args.insert(CropEdgeGeometryFilter::k_SelectedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_EdgeGeometry})));
  args.insert(CropEdgeGeometryFilter::k_CreatedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_CroppedEdgeGeometry})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(DataPath({k_EdgeGeometry})));
  auto& edgeGeom = dataStructure.getDataRefAs<EdgeGeom>(DataPath({k_EdgeGeometry}));
  Float32Array& vertices = edgeGeom.getVerticesRef();
  UInt64Array& edges = edgeGeom.getEdgesRef();
  REQUIRE(vertices.getNumberOfTuples() == 4);
  REQUIRE(edges.getNumberOfTuples() == 3);
  REQUIRE(vertices[0] == 0);
  REQUIRE(vertices[1] == 0);
  REQUIRE(vertices[2] == 0);
  REQUIRE(vertices[3] == 1);
  REQUIRE(vertices[4] == 2);
  REQUIRE(vertices[5] == -2);
  REQUIRE(vertices[6] == 1.5);
  REQUIRE(vertices[7] == 1.75);
  REQUIRE(vertices[8] == -2);
  REQUIRE(vertices[9] == 1);
  REQUIRE(vertices[10] == -0.5);
  REQUIRE(vertices[11] == 0);
  REQUIRE(edges[0] == 0);
  REQUIRE(edges[1] == 1);
  REQUIRE(edges[2] == 1);
  REQUIRE(edges[3] == 2);
  REQUIRE(edges[4] == 3);
  REQUIRE(edges[5] == 0);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(k_UInt32ArrayPath));
  auto& uint32Array = dataStructure.getDataRefAs<UInt32Array>(k_UInt32ArrayPath);
  REQUIRE(uint32Array.getNumberOfTuples() == 4);
  REQUIRE(uint32Array[0] == 8);
  REQUIRE(uint32Array[1] == 3);
  REQUIRE(uint32Array[2] == 893);
  REQUIRE(uint32Array[3] == 327);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_Int32ArrayPath));
  auto& int32Array = dataStructure.getDataRefAs<Int32Array>(k_Int32ArrayPath);
  REQUIRE(int32Array.getNumberOfTuples() == 3);
  REQUIRE(int32Array[0] == 12);
  REQUIRE(int32Array[1] == 56);
  REQUIRE(int32Array[2] == 91);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(k_Int64ArrayPath));
  auto& int64Array = dataStructure.getDataRefAs<Int64Array>(k_Int64ArrayPath);
  REQUIRE(int64Array.getNumberOfTuples() == 3);
  REQUIRE(int64Array[0] == 24);
  REQUIRE(int64Array[1] == 124);
  REQUIRE(int64Array[2] == 786);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Invalid Params", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  UnitTest::LoadPlugins();
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;
  args.insert(CropEdgeGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));

  std::vector<float32> minCoords;
  std::vector<float32> maxCoords;
  int64 errCode;

  SECTION("X Min > X Max")
  {
    minCoords = {2.5, -0.5, -0.5};
    maxCoords = {1.5, 2.5, 0.5};
    errCode = to_underlying(CropEdgeGeometry::ErrorCodes::XMinLargerThanXMax);
    args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
    args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  }
  SECTION("Y Min > Y Max")
  {
    minCoords = {-0.5, 3.5, -0.5};
    maxCoords = {1.5, 2.5, 0.5};
    errCode = to_underlying(CropEdgeGeometry::ErrorCodes::YMinLargerThanYMax);
    args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(true));
    args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  }
  SECTION("Z Min > Z Max")
  {
    minCoords = {-0.5, -0.5, 1.0};
    maxCoords = {1.5, 2.5, 0.5};
    errCode = to_underlying(CropEdgeGeometry::ErrorCodes::ZMinLargerThanZMax);
    args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(true));
  }
  SECTION("No dimensions chosen")
  {
    minCoords = {-0.5, -0.5, -0.5};
    maxCoords = {1.5, 2.5, 0.5};
    errCode = to_underlying(CropEdgeGeometry::ErrorCodes::NoDimensionsChosen);
    args.insert(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
    args.insert(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  }

  args.insert(CropEdgeGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float32>>(minCoords));
  args.insert(CropEdgeGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float32>>(maxCoords));
  args.insert(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insert(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key,
              std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::InterpolateOutsideVertex)));
  args.insert(CropEdgeGeometryFilter::k_SelectedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_EdgeGeometry})));
  args.insert(CropEdgeGeometryFilter::k_CreatedEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({k_CroppedEdgeGeometry})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == errCode);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropEdgeGeometry: Child names containing the geometry name are kept", "[SimplnxCore][CropEdgeGeometry]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  const DataPath srcGeomPath({"Geom Group", "Geom"});
  const DataPath destGeomPath({"Group", "Geom Out"});
  auto* sourceGroupPtr = DataGroup::Create(dataStructure, "Geom Group");
  REQUIRE(sourceGroupPtr != nullptr);
  REQUIRE(DataGroup::Create(dataStructure, "Group") != nullptr);
  auto* geomPtr = EdgeGeom::Create(dataStructure, "Geom", sourceGroupPtr->getId());
  REQUIRE(geomPtr != nullptr);
  auto* verticesPtr = Float32Array::Create(dataStructure, "Vertices",
                                           DataStoreUtilities::CreateDataStore<float32>(dataStructure, geomPtr->getDataPaths().front().createChildPath("Vertices"), {3}, {3}), geomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  verticesPtr->fill(0.0F);
  (*verticesPtr)[3] = 1.0F;
  (*verticesPtr)[6] = 2.0F;
  geomPtr->setVertices(*verticesPtr);
  auto* edgesPtr =
      UInt64Array::Create(dataStructure, "Edges", DataStoreUtilities::CreateDataStore<uint64>(dataStructure, geomPtr->getDataPaths().front().createChildPath("Edges"), {2}, {2}), geomPtr->getId());
  REQUIRE(edgesPtr != nullptr);
  (*edgesPtr)[0] = 0;
  (*edgesPtr)[1] = 1;
  (*edgesPtr)[2] = 1;
  (*edgesPtr)[3] = 2;
  geomPtr->setEdgeList(*edgesPtr);
  auto* vertexAmPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", {3}, geomPtr->getId());
  REQUIRE(vertexAmPtr != nullptr);
  geomPtr->setVertexAttributeMatrix(*vertexAmPtr);
  auto* elementAmPtr = AttributeMatrix::Create(dataStructure, "Edge Data", {2}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setEdgeAttributeMatrix(*elementAmPtr);
  auto* dataPtr =
      Int32Array::Create(dataStructure, "Data", DataStoreUtilities::CreateDataStore<int32>(dataStructure, elementAmPtr->getDataPaths().front().createChildPath("Data"), elementAmPtr->getShape(), {1}),
                         elementAmPtr->getId());
  REQUIRE(dataPtr != nullptr);
  dataPtr->fill(7);
  auto* featureAmPtr = AttributeMatrix::Create(dataStructure, "Geom Feature Data", {2}, geomPtr->getId());
  REQUIRE(featureAmPtr != nullptr);
  auto* valuesPtr = Int32Array::Create(dataStructure, "Geom Values",
                                       DataStoreUtilities::CreateDataStore<int32>(dataStructure, featureAmPtr->getDataPaths().front().createChildPath("Geom Values"), {2}, {1}), featureAmPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(42);
  CropEdgeGeometryFilter filter;
  Arguments args;
  args.insert(CropEdgeGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insert(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CropEdgeGeometryFilter::k_SelectedEdgeGeometryPath_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CreatedEdgeGeometryPath_Key, std::make_any<DataPath>(destGeomPath));
  args.insertOrAssign(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CropXDim_Key, std::make_any<bool>(true));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CropYDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropEdgeGeometryFilter::k_CropZDim_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key,
                      std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::InterpolateOutsideVertex)));
  args.insertOrAssign(CropEdgeGeometryFilter::k_MinCoord_Key, std::make_any<std::vector<float32>>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
  args.insertOrAssign(CropEdgeGeometryFilter::k_MaxCoord_Key, std::make_any<std::vector<float32>>(std::vector<float32>{3.0F, 1.0F, 1.0F}));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");

  SECTION("Declared recursive paths keep child names")
  {
    const CopyDataObjectAction* copyActionPtr = nullptr;
    for(const auto& action : preflightResult.outputActions.value().actions)
    {
      const auto* candidatePtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
      if(candidatePtr != nullptr && candidatePtr->path() == srcGeomPath.createChildPath("Geom Feature Data"))
      {
        copyActionPtr = candidatePtr;
        break;
      }
    }
    REQUIRE(copyActionPtr != nullptr);
    const auto createdPaths = copyActionPtr->getAllCreatedPaths();
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedValuesPath) != createdPaths.end());
    REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), renamedAmPath.createChildPath("Geom Out Values")) == createdPaths.end());
  }
  SECTION("Copied objects keep child names")
  {
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataAs<AttributeMatrix>(copiedAmPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Int32Array>(copiedValuesPath) != nullptr);
    REQUIRE_FALSE(dataStructure.containsData(renamedAmPath));
    REQUIRE(dataStructure.getDataAs<Int32Array>(destGeomPath.createChildPath("Edge Data").createChildPath("Data")) != nullptr);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropEdgeGeometry: Shared outside vertex gets a clip point per edge", "[SimplnxCore][CropEdgeGeometry]")
{
  UnitTest::LoadPlugins();
  auto dataStructure = CreateSharedOutsideVertexData();
  auto args = SharedOutsideVertexArguments();
  CropEdgeGeometryFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath outputPath({k_CroppedEdgeGeometry});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(outputPath));
  const auto& geom = dataStructure.getDataRefAs<EdgeGeom>(outputPath);
  const auto& vertices = geom.getVerticesRef();
  const auto& edges = geom.getEdgesRef();
  REQUIRE(vertices.getNumberOfTuples() == 4);
  const std::vector<float32> expectedVertices{0, 0, 0, 0.5F, 1, -1, 1, 2, 3, 1, 2, 1.333333F};
  for(usize i = 0; i < expectedVertices.size(); ++i)
  {
    CAPTURE(i);
    REQUIRE(vertices[i] == Approx(expectedVertices[i]).margin(0.000001F));
  }
  REQUIRE(edges.getNumberOfTuples() == 2);
  const std::vector<uint64> expectedEdges{0, 2, 1, 3};
  for(usize i = 0; i < expectedEdges.size(); ++i)
  {
    REQUIRE(edges[i] == expectedEdges[i]);
  }
  const auto vertexPath = outputPath.createChildPath(Constants::k_VertexData).createChildPath("Values");
  const auto edgePath = outputPath.createChildPath(Constants::k_Edge_Data).createChildPath("Values");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(vertexPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(edgePath));
  const auto& vertexValues = dataStructure.getDataRefAs<Int32Array>(vertexPath);
  const auto& edgeValues = dataStructure.getDataRefAs<Int32Array>(edgePath);
  REQUIRE(vertexValues.getNumberOfTuples() == 4);
  const std::vector<int32> expectedValues{7, 11, 13, 13};
  for(usize i = 0; i < expectedValues.size(); ++i)
  {
    REQUIRE(vertexValues[i] == expectedValues[i]);
  }
  REQUIRE(edgeValues.getNumberOfTuples() == 2);
  REQUIRE(edgeValues[0] == 21);
  REQUIRE(edgeValues[1] == 34);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropEdgeGeometry: String and NeighborList data are cropped", "[SimplnxCore][CropEdgeGeometry]")
{
  UnitTest::LoadPlugins();
  const bool cropCase = GENERATE(false, true);
  CAPTURE(cropCase);
  auto dataStructure = CreateSharedOutsideVertexData(cropCase);
  const DataPath inputPath({k_EdgeGeometry});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(inputPath));
  auto& geom = dataStructure.getDataRefAs<EdgeGeom>(inputPath);
  auto* strings = StringArray::CreateWithValues(dataStructure, "Strings", {3}, {"a", "b", "o"}, geom.getVertexAttributeMatrixRef().getId());
  auto* neighbors = NeighborList<int32>::Create(dataStructure, "Neighbors", {2}, geom.getEdgeAttributeMatrixRef().getId());
  REQUIRE(strings != nullptr);
  REQUIRE(neighbors != nullptr);
  neighbors->setLists({{1}, {2, 3}});
  auto args = SharedOutsideVertexArguments(cropCase);
  CropEdgeGeometryFilter filter;
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath outputPath({k_CroppedEdgeGeometry});
  const auto stringsPath = outputPath.createChildPath(Constants::k_VertexData).createChildPath("Strings");
  const auto neighborsPath = outputPath.createChildPath(Constants::k_Edge_Data).createChildPath("Neighbors");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(stringsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(neighborsPath));
  const auto& outputStrings = dataStructure.getDataRefAs<StringArray>(stringsPath);
  const auto& outputNeighbors = dataStructure.getDataRefAs<NeighborList<int32>>(neighborsPath);
  const std::vector<std::string> expectedStrings = cropCase ? std::vector<std::string>{"a", "b"} : std::vector<std::string>{"a", "b", "o", "o"};
  const std::vector<std::vector<int32>> expectedNeighbors = cropCase ? std::vector<std::vector<int32>>{{2, 3}} : std::vector<std::vector<int32>>{{1}, {2, 3}};
  REQUIRE(outputStrings.getNumberOfTuples() == expectedStrings.size());
  for(usize i = 0; i < expectedStrings.size(); ++i)
  {
    REQUIRE(outputStrings[i] == expectedStrings[i]);
  }
  REQUIRE(outputNeighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    REQUIRE(outputNeighbors.getList(i) == expectedNeighbors[i]);
  }
  // Output lists are independent copies of the input lists.
  neighbors->updateListEntry(1, 0, 99);
  REQUIRE(outputNeighbors.getList(expectedNeighbors.size() - 1) == expectedNeighbors.back());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON preflight", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  const bool renumber = GENERATE(false, true);
  auto result = filter.preflight(ds, MakeFeatureCropArgs(true, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  const auto it = std::find_if(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; });
  REQUIRE(it != result.outputValues.end());
  CHECK(it->value.find("Edge/Edge Feature Data/NumElements") != std::string::npos);
  CHECK(it->value.find("Edge/Edge Feature Data/NeighborList") != std::string::npos);
  CHECK(it->value.find("Edge/Edge Feature Data/Names") != std::string::npos);
  CHECK(it->value.find("Ensemble") == std::string::npos);
  CHECK(it->value.find(" Its values describe the uncropped geometry and are not copied. Recompute any of the following Data Arrays:\n") != std::string::npos);
  CHECK((it->value.find("\n\nFeature Ids are renumbered.\nThe Attribute Matrix is resized to the number of features remaining after the crop.") != std::string::npos) == renumber);
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Invalidated NeighborLists" || v.name == "Feature Data Not Cleared"; }));
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear OFF preflight", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  auto result = filter.preflight(ds, MakeFeatureCropArgs(false, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
  const auto it = std::find_if(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Feature Data Not Cleared"; });
  REQUIRE(it != result.outputValues.end());
  CHECK(it->value == "Attribute Matrices other than the Edge and Vertex Attribute Matrices are copied to the cropped geometry unchanged. If one of them holds "
                     "Feature data, its values describe the uncropped geometry and are not valid for the cropped geometry.");
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& v) { return v.name == "Cleared Feature Data"; }));
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear OFF ignores stored renumber", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  Arguments args = MakeFeatureCropArgs(false, true);
  args.insertOrAssign(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Edge Data", "DoesNotExist"})));
  args.insertOrAssign(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"DoesNotExist"})));
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  CHECK(result.outputActions.warnings().empty());
  CHECK(std::none_of(result.outputValues.begin(), result.outputValues.end(), [](const auto& value) { return value.value.find("Ensemble") != std::string::npos; }));
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: StrictFeatureIds preflight", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM][StrictFeatureIds]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  const auto* edgeGeom = ds.getDataAs<EdgeGeom>(DataPath({"Edge"}));
  REQUIRE(edgeGeom != nullptr);
  const auto* edgeData = edgeGeom->getEdgeAttributeMatrix();
  REQUIRE(edgeData != nullptr);
  auto* otherData = AttributeMatrix::Create(ds, "Other Edge Data", edgeData->getShape(), edgeGeom->getId());
  REQUIRE(otherData != nullptr);
  auto* misplacedIds = UnitTest::CreateTestDataArray<int32>(ds, "MisplacedFeatureIds", otherData->getShape(), {1}, otherData->getId());
  REQUIRE(misplacedIds != nullptr);
  misplacedIds->fill(1);

  CropEdgeGeometryFilter filter;
  Arguments args = MakeFeatureCropArgs(true, true);
  args.insertOrAssign(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Other Edge Data", "MisplacedFeatureIds"})));

  SECTION("Renumber ON rejects Feature Ids in another Attribute Matrix")
  {
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors().front().code == -67413);
    CHECK(result.outputActions.errors().front().message ==
          "**Edge Feature Ids** DataArray 'Edge/Other Edge Data/MisplacedFeatureIds' must be in the Edge Attribute Matrix 'Edge/Edge Data' of the selected Edge Geometry 'Edge'. "
          "Select the DataArray from that Attribute Matrix.");
  }
  SECTION("Renumber OFF accepts Feature Ids in another Attribute Matrix")
  {
    args.insertOrAssign(CropEdgeGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Renumber ON accepts Feature Ids in the Edge Attribute Matrix")
  {
    args.insertOrAssign(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Edge Data", "FeatureIds"})));
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON Feature AM errors", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  // An Attribute Matrix that is not a child of "Edge".
  REQUIRE(AttributeMatrix::Create(ds, "Elsewhere", std::vector<usize>{5}) != nullptr);
  CropEdgeGeometryFilter filter;

  Arguments args = MakeFeatureCropArgs(true, false);
  args.insertOrAssign(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Elsewhere"})));
  auto notChild = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(notChild.outputActions);
  REQUIRE(notChild.outputActions.errors().size() == 1);
  CHECK(notChild.outputActions.errors().front().code == -67411);

  args.insertOrAssign(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Edge Data"})));
  auto isEdgeAm = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isEdgeAm.outputActions);
  REQUIRE(isEdgeAm.outputActions.errors().size() == 1);
  CHECK(isEdgeAm.outputActions.errors().front().code == -67412);

  args.insertOrAssign(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Vertex Data"})));
  auto isVertexAm = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(isVertexAm.outputActions);
  REQUIRE(isVertexAm.outputActions.errors().size() == 1);
  CHECK(isVertexAm.outputActions.errors().front().code == -67412);

  args.insertOrAssign(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Edge", "Missing"})));
  auto missing = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(missing.outputActions);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON, renumber OFF execute", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  auto result = filter.execute(ds, MakeFeatureCropArgs(true, false));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 0);
  // The kept edges retain Feature Ids 3 and 4, including the clipped edge.
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  // The Ensemble AM is copied unchanged.
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Edge Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Edge Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  // The source geometry is untouched (debugging route).
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Edge", "Edge Feature Data"})));
  CHECK(ds.getDataRefAs<AttributeMatrix>(DataPath({"Edge", "Edge Feature Data"})).getSize() == 3);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON, renumber ON execute", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  auto result = filter.execute(ds, MakeFeatureCropArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{3}); // features 3 and 4 remain, plus row 0
  CHECK(am.getSize() == 0);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Edge Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Edge Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear OFF execute copies Feature AM unchanged", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  const bool renumber = GENERATE(false, true);
  CAPTURE(renumber);
  auto result = filter.execute(ds, MakeFeatureCropArgs(false, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Feature Data", "NumElements"})));
  const auto& numElements = ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Feature Data", "NumElements"}));
  REQUIRE(numElements.getNumberOfTuples() == 5);
  const std::array<int32, 5> expectedNumElements = {0, 1, 1, 1, 1};
  for(usize i = 0; i < expectedNumElements.size(); ++i)
  {
    CHECK(numElements[i] == expectedNumElements[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"}));
  CHECK(am.getShape() == std::vector<usize>{5});
  CHECK(am.getSize() == 3);
  REQUIRE_NOTHROW(ds.getDataRefAs<NeighborList<int32>>(DataPath({"Cropped", "Edge Feature Data", "NeighborList"})));
  const auto& neighbors = ds.getDataRefAs<NeighborList<int32>>(DataPath({"Cropped", "Edge Feature Data", "NeighborList"}));
  const std::vector<std::vector<int32>> expectedNeighbors = {{}, {2}, {1, 3}, {2, 4}, {3}};
  REQUIRE(neighbors.getNumberOfTuples() == expectedNeighbors.size());
  for(usize i = 0; i < expectedNeighbors.size(); ++i)
  {
    CHECK(neighbors.getList(static_cast<int32>(i)) == expectedNeighbors[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<StringArray>(DataPath({"Cropped", "Edge Feature Data", "Names"})));
  const auto& names = ds.getDataRefAs<StringArray>(DataPath({"Cropped", "Edge Feature Data", "Names"}));
  const std::array<std::string, 5> expectedNames = {"Background", "One", "Two", "Three", "Four"};
  REQUIRE(names.getNumberOfTuples() == expectedNames.size());
  for(usize i = 0; i < expectedNames.size(); ++i)
  {
    CHECK(names[i] == expectedNames[i]);
  }
  // The stored renumber=true is ignored.
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {3, 4};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Edge Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Cropped", "Edge Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON in place", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  DataStructure ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  Arguments args = MakeFeatureCropArgs(true, true);
  args.insertOrAssign(CropEdgeGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Edge", "Edge Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Edge", "Edge Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getShape() == std::vector<usize>{3});
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(DataPath({"Edge"})));
  const auto& edgeGeom = ds.getDataRefAs<EdgeGeom>(DataPath({"Edge"}));
  CHECK(edgeGeom.getNumberOfEdges() == 2);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Edge", "Edge Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Edge", "Edge Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  const std::array<int32, 2> expectedIds = {1, 2};
  for(usize i = 0; i < expectedIds.size(); ++i)
  {
    CHECK(ids[i] == expectedIds[i]);
  }
  CHECK(ds.getData(DataPath({"Cropped"})) == nullptr);
  CHECK(ds.getData(DataPath({".Edge"})) == nullptr);
  CHECK(ds.getData(DataPath({".cropped_edge_geometry"})) == nullptr);
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt32Array>(DataPath({"Edge", "Edge Ensemble Data", "CrystalStructures"})));
  const auto& xtal = ds.getDataRefAs<UInt32Array>(DataPath({"Edge", "Edge Ensemble Data", "CrystalStructures"}));
  REQUIRE(xtal.getNumberOfTuples() == 2);
  CHECK(xtal[0] == 999);
  CHECK(xtal[1] == 1);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON Feature Id tuple count", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureCropFixture();
  auto* geom = ds.getDataAs<EdgeGeom>(DataPath({"Edge"}));
  REQUIRE(geom != nullptr);
  auto resizeResult = geom->getEdgeAttributeMatrixRef().resizeTuples({3});
  SIMPLNX_RESULT_REQUIRE_VALID(resizeResult);
  CropEdgeGeometryFilter filter;
  auto result = filter.preflight(ds, MakeFeatureCropArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors().front().code == -67414);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON accepts shared parent paths", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureCropFixture();
  auto* aliases = DataGroup::Create(ds, "Aliases");
  REQUIRE(aliases != nullptr);
  const auto* geom = ds.getDataAs<EdgeGeom>(DataPath({"Edge"}));
  REQUIRE(geom != nullptr);
  const auto* featureAm = ds.getDataAs<AttributeMatrix>(DataPath({"Edge", "Edge Feature Data"}));
  REQUIRE(featureAm != nullptr);
  REQUIRE(ds.setAdditionalParent(geom->getEdgeAttributeMatrix()->getId(), aliases->getId()));
  REQUIRE(ds.setAdditionalParent(featureAm->getId(), aliases->getId()));
  auto args = MakeFeatureCropArgs(true, true);
  args.insertOrAssign(CropEdgeGeometryFilter::k_EdgeFeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"Aliases", "Edge Data", "FeatureIds"})));
  args.insertOrAssign(CropEdgeGeometryFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Aliases", "Edge Feature Data"})));
  CropEdgeGeometryFilter filter;
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"})));
  const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"}));
  CHECK(am.getSize() == 0);
  CHECK(am.getNumberOfTuples() == 3);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  CHECK(ids[0] == 1);
  CHECK(ids[1] == 2);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON validates source Feature Id range", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureCropFixture();
  const int32 invalidId = GENERATE(-1, 5);
  const usize sourceEdge = GENERATE(usize{0}, usize{2});
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Edge", "Edge Data", "FeatureIds"})));
  ds.getDataRefAs<Int32Array>(DataPath({"Edge", "Edge Data", "FeatureIds"}))[sourceEdge] = invalidId;
  CropEdgeGeometryFilter filter;
  auto result = filter.execute(ds, MakeFeatureCropArgs(true, true));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors().front().code == (invalidId < 0 ? -5355 : -5351));
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON keeps Feature Ids with duplicated clip vertices", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureCropFixture();
  auto* geom = ds.getDataAs<EdgeGeom>(DataPath({"Edge"}));
  REQUIRE(geom != nullptr);
  const std::array<float32, 15> coordinates = {0, 4, 6, -1, 0, 0, 3, 0, 0, 3, 1, 0, 0, 0, 0};
  const std::array<uint64, 8> connectivity = {0, 1, 0, 4, 2, 4, 3, 4};
  for(usize i = 0; i < coordinates.size(); ++i)
  {
    geom->getVerticesRef()[i] = coordinates[i];
  }
  for(usize i = 0; i < connectivity.size(); ++i)
  {
    geom->getEdgesRef()[i] = connectivity[i];
  }
  const bool renumber = GENERATE(false, true);
  CropEdgeGeometryFilter filter;
  auto result = filter.execute(ds, MakeFeatureCropArgs(true, renumber));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* cropped = ds.getDataAs<EdgeGeom>(DataPath({"Cropped"}));
  REQUIRE(cropped != nullptr);
  CHECK(cropped->getNumberOfVertices() == 4);
  CHECK(cropped->getNumberOfEdges() == 2);
  CHECK(cropped->getVerticesRef()[6] == Approx(2.1F));
  CHECK(cropped->getVerticesRef()[7] == Approx(0));
  CHECK(cropped->getVerticesRef()[9] == Approx(2.1F));
  CHECK(cropped->getVerticesRef()[10] == Approx(0.7F));
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"})));
  const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"}));
  REQUIRE(ids.getNumberOfTuples() == 2);
  CHECK(ids[0] == (renumber ? 1 : 3));
  CHECK(ids[1] == (renumber ? 2 : 4));
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter: Clear ON with Ignore Edge and Filter Error", "[SimplnxCore][CropEdgeGeometryFilter][ClearFeatureAM]")
{
  UnitTest::LoadPlugins();
  auto ds = CreateFeatureCropFixture();
  CropEdgeGeometryFilter filter;
  // Edge 2 (x = 2 to 3, Feature 3) crosses x = 2.1. Edge 3 (x = 3 to 4, Feature 4) is inside.
  SECTION("Ignore Edge drops the crossing feature")
  {
    const bool renumber = GENERATE(false, true);
    CAPTURE(renumber);
    Arguments args = MakeFeatureCropArgs(true, renumber);
    args.insertOrAssign(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::IgnoreEdge)));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE_NOTHROW(ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"})));
    const auto& am = ds.getDataRefAs<AttributeMatrix>(DataPath({"Cropped", "Edge Feature Data"}));
    CHECK(am.getSize() == 0);
    CHECK(am.getShape() == std::vector<usize>{renumber ? usize{2} : usize{5}});
    REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"})));
    const auto& ids = ds.getDataRefAs<Int32Array>(DataPath({"Cropped", "Edge Data", "FeatureIds"}));
    REQUIRE(ids.getNumberOfTuples() == 1);
    CHECK(ids[0] == (renumber ? 1 : 4));
    UnitTest::CheckArraysInheritTupleDims(ds);
  }
  SECTION("Filter Error fails before renumbering")
  {
    Arguments args = MakeFeatureCropArgs(true, true);
    args.insertOrAssign(CropEdgeGeometryFilter::k_BoundaryIntersectionBehavior_Key,
                        std::make_any<ChoicesParameter::ValueType>(to_underlying(CropEdgeGeometry::BoundaryIntersectionBehavior::FilterError)));
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    CHECK(result.result.errors().front().code == to_underlying(CropEdgeGeometry::ErrorCodes::OutsideVertexError));
  }
}
