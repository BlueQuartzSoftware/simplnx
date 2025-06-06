#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ComputeCoordinateThreshold.hpp"
#include "SimplnxCore/Filters/ComputeCoordinateThresholdFilter.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
constexpr uint64 k_TupleCount = 8;

constexpr StringLiteral k_GeomName = "TestGeom";
const DataPath k_GeomPath({k_GeomName});

constexpr StringLiteral k_MaskName = "mask_array";
const DataPath k_MaskPath({k_MaskName});

void SphereExecuteFilter(DataStructure& dataStructure, bool shouldInvert, const VectorFloat32Parameter::ValueType& sphereInfo)
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Sphere)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_InvertContainer_Key, std::make_any<bool>(shouldInvert));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SphereInfo_Key, std::make_any<VectorFloat32Parameter::ValueType>(sphereInfo));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

void RectangleExecuteFilter(DataStructure& dataStructure, bool shouldInvert, const VectorFloat32Parameter::ValueType& minPoint, const VectorFloat32Parameter::ValueType& maxPoint)
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Rectangle)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_InvertContainer_Key, std::make_any<bool>(shouldInvert));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MinCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(minPoint));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MaxCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(maxPoint));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Image Geom Test - Rectangle", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  VectorFloat32Parameter::ValueType minCoord = {1.0f, 1.0f, 0.0f};
  VectorFloat32Parameter::ValueType maxCoord = {4.0f, 4.0f, 0.0f};

  SECTION("Baseline")
  {
    RectangleExecuteFilter(dataStructure, false, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);
  }

  SECTION("Inverted")
  {
    RectangleExecuteFilter(dataStructure, true, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Image Geom Test - Sphere", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  VectorFloat32Parameter::ValueType sphereInfo = {0.0f, 0.0f, 0.0f, 0.0f};

  SECTION("Baseline")
  {
    SphereExecuteFilter(dataStructure, false, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);
  }

  SECTION("Inverted")
  {
    SphereExecuteFilter(dataStructure, true, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Vertex Geom Test - Rectangle", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* vertexGeom = VertexGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto& vertices = vertexArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  VectorFloat32Parameter::ValueType minCoord = {3.0f, 3.0f, 3.0f};
  VectorFloat32Parameter::ValueType maxCoord = {5.0f, 5.0f, 5.0f};

  SECTION("Baseline")
  {
    RectangleExecuteFilter(dataStructure, false, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 0);
    REQUIRE(mask[2] == 0);
    REQUIRE(mask[3] == 1);
    REQUIRE(mask[4] == 1);
    REQUIRE(mask[5] == 1);
    REQUIRE(mask[6] == 0);
    REQUIRE(mask[7] == 0);
  }

  SECTION("Inverted")
  {
    RectangleExecuteFilter(dataStructure, true, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 1);
    REQUIRE(mask[2] == 1);
    REQUIRE(mask[3] == 0);
    REQUIRE(mask[4] == 0);
    REQUIRE(mask[5] == 0);
    REQUIRE(mask[6] == 1);
    REQUIRE(mask[7] == 1);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Vertex Geom Test - Sphere", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* vertexGeom = VertexGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto& vertices = vertexArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  VectorFloat32Parameter::ValueType sphereInfo = {2.5f, 2.5f, 2.5f, 1.5f};

  SECTION("Baseline")
  {
    SphereExecuteFilter(dataStructure, false, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);
    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 1);
    REQUIRE(mask[2] == 1);
    REQUIRE(mask[3] == 1);
    REQUIRE(mask[4] == 1);
    REQUIRE(mask[5] == 0);
    REQUIRE(mask[6] == 0);
    REQUIRE(mask[7] == 0);
  }

  SECTION("Inverted")
  {
    SphereExecuteFilter(dataStructure, true, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 0);
    REQUIRE(mask[2] == 0);
    REQUIRE(mask[3] == 0);
    REQUIRE(mask[4] == 0);
    REQUIRE(mask[5] == 1);
    REQUIRE(mask[6] == 1);
    REQUIRE(mask[7] == 1);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Edge Geom Test - Rectangle", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* geom = EdgeGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* edgesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "edges", {k_TupleCount}, {2}, geom->getId());
  geom->setEdgeList(*edgesArray);

  auto& vertices = vertexArray->getDataStoreRef();
  auto& edges = edgesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    edges[(i * 2) + 0] = i;
    edges[(i * 2) + 1] = i + 1;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  edges[((k_TupleCount - 1) * 2) + 1] = 0;

  VectorFloat32Parameter::ValueType minCoord = {3.0f, 3.0f, 3.0f};
  VectorFloat32Parameter::ValueType maxCoord = {5.0f, 5.0f, 5.0f};

  SECTION("Baseline")
  {
    RectangleExecuteFilter(dataStructure, false, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 0);
    REQUIRE(mask[2] == 0);
    REQUIRE(mask[3] == 1);
    REQUIRE(mask[4] == 1);
    REQUIRE(mask[5] == 0);
    REQUIRE(mask[6] == 0);
    REQUIRE(mask[7] == 0);
  }

  SECTION("Inverted")
  {
    RectangleExecuteFilter(dataStructure, true, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 1);
    REQUIRE(mask[2] == 1);
    REQUIRE(mask[3] == 0);
    REQUIRE(mask[4] == 0);
    REQUIRE(mask[5] == 1);
    REQUIRE(mask[6] == 1);
    REQUIRE(mask[7] == 1);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Edge Geom Test - Sphere", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* geom = EdgeGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* edgesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "edges", {k_TupleCount}, {2}, geom->getId());
  geom->setEdgeList(*edgesArray);

  auto& vertices = vertexArray->getDataStoreRef();
  auto& edges = edgesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    edges[(i * 2) + 0] = i;
    edges[(i * 2) + 1] = i + 1;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  edges[((k_TupleCount - 1) * 2) + 1] = 0;

  VectorFloat32Parameter::ValueType sphereInfo = {2.5f, 2.5f, 2.5f, 1.0f};

  SECTION("Baseline")
  {
    SphereExecuteFilter(dataStructure, false, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 0);
    REQUIRE(mask[2] == 1);
    REQUIRE(mask[3] == 0);
    REQUIRE(mask[4] == 0);
    REQUIRE(mask[5] == 0);
    REQUIRE(mask[6] == 0);
    REQUIRE(mask[7] == 0);
  }

  SECTION("Inverted")
  {
    SphereExecuteFilter(dataStructure, true, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 1);
    REQUIRE(mask[2] == 0);
    REQUIRE(mask[3] == 1);
    REQUIRE(mask[4] == 1);
    REQUIRE(mask[5] == 1);
    REQUIRE(mask[6] == 1);
    REQUIRE(mask[7] == 1);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Triangle Geom Test - Rectangle", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* geom = TriangleGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geom->getId());
  geom->setFaceList(*facesArray);

  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }

  VectorFloat32Parameter::ValueType minCoord = {-1.0f, -1.0f, -1.0f};
  VectorFloat32Parameter::ValueType maxCoord = {3.5f, 3.5f, 3.5f};

  SECTION("Baseline")
  {
    RectangleExecuteFilter(dataStructure, false, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 0);
  }

  SECTION("Inverted")
  {
    RectangleExecuteFilter(dataStructure, true, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 1);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Triangle Geom Test - Sphere", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* geom = TriangleGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geom->getId());
  geom->setFaceList(*facesArray);

  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }

  VectorFloat32Parameter::ValueType sphereInfo = {1.5f, 1.5f, 1.5f, 2.0f};

  SECTION("Baseline")
  {
    SphereExecuteFilter(dataStructure, false, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 0);
  }

  SECTION("Inverted")
  {
    SphereExecuteFilter(dataStructure, true, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 1);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Quad Geom Test - Rectangle", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* geom = QuadGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {4}, geom->getId());
  geom->setFaceList(*facesArray);

  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }

  VectorFloat32Parameter::ValueType minCoord = {0.0f, 0.0f, 0.0f};
  VectorFloat32Parameter::ValueType maxCoord = {3.0f, 3.0f, 3.0f};

  SECTION("Baseline")
  {
    RectangleExecuteFilter(dataStructure, false, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 0);
  }

  SECTION("Inverted")
  {
    RectangleExecuteFilter(dataStructure, true, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 1);
  }
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Quad Geom Test - Sphere", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  auto* geom = QuadGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {4}, geom->getId());
  geom->setFaceList(*facesArray);

  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }

  VectorFloat32Parameter::ValueType sphereInfo = {5.0f, 5.0f, 5.0f, 2.0f};

  SECTION("Baseline")
  {
    SphereExecuteFilter(dataStructure, false, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 1);
  }

  SECTION("Inverted")
  {
    SphereExecuteFilter(dataStructure, true, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 0);
  }
}
