#include "SimplnxCore/Filters/Algorithms/CropEdgeGeometry.hpp"
#include "SimplnxCore/Filters/CropEdgeGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include <simplnx/Parameters/ChoicesParameter.hpp>

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
} // namespace

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Filter Error", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

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
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Ignore Edges", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

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
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Interpolate Outside Vertices", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

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
}

TEST_CASE("SimplnxCore::CropEdgeGeometryFilter - Invalid Params", "[SimplnxCore][CropEdgeGeometryFilter]")
{
  fs::path vertexCoordsPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "VertexCoordinates.csv";
  fs::path edgeConnectivityPath = fs::path(std::string(nx::core::unit_test::k_BuildDir)) / "Data" / "Test_Data" / "EdgeConnectivity.csv";

  CropEdgeGeometryFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

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
}
