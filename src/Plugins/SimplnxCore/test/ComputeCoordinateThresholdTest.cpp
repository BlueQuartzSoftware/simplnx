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

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Rectangle Preflight Error - Triangle Geom", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
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

  VectorFloat32Parameter::ValueType minCoord = {3.5f, 3.5f, 3.5f};
  VectorFloat32Parameter::ValueType maxCoord = {-1.0f, -1.0f, -1.0f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Rectangle)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MinCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(minCoord));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MaxCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(maxCoord));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Sphere Preflight Error - Triangle Geom", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
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

  VectorFloat32Parameter::ValueType sphereInfo = {1.0f, 1.0f, 1.0f, -1.75f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Sphere)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SphereInfo_Key, std::make_any<VectorFloat32Parameter::ValueType>(sphereInfo));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Rectangle Preflight Bounds Error - Image Geom", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});

#if 0
      0, 0, 0, 0, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 0, 0, 0, 0;
#endif

  VectorFloat32Parameter::ValueType minCoord = {5.5f, 5.5f, 1.5f};
  VectorFloat32Parameter::ValueType maxCoord = {6.5f, 6.5f, 1.5f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Rectangle)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MinCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(minCoord));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MaxCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(maxCoord));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Sphere Preflight Bounds Error - Image Geom", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});

#if 0
      0, 0, 0, 0, 0,
      0, 0, 1, 0, 0,
      0, 1, 1, 1, 0,
      0, 0, 1, 0, 0,
      0, 0, 0, 0, 0;
#endif

  VectorFloat32Parameter::ValueType sphereInfo = {5.5f, 5.5f, 1.5f, 0.5f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Sphere)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SphereInfo_Key, std::make_any<VectorFloat32Parameter::ValueType>(sphereInfo));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Rectangle Runtime Warning - Triangle Geom", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
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

  VectorFloat32Parameter::ValueType minCoord;
  VectorFloat32Parameter::ValueType maxCoord;

  SECTION("Positive Case")
  {
    minCoord = {6.0f, 6.0f, 6.0f};
    maxCoord = {9.0f, 9.0f, 9.0f};
  }

  SECTION("Negative Case")
  {
    minCoord = {-2.0f, -2.0f, -2.0f};
    maxCoord = {-1.0f, -1.0f, -1.0f};
  }

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Rectangle)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MinCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(minCoord));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_MaxCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(maxCoord));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(!executeResult.result.warnings().empty());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Sphere Runtime Warning - Triangle Geom", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
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

  VectorFloat32Parameter::ValueType sphereInfo = {6.0f, 6.0f, 6.0f, 0.75f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCoordinateThresholdFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_ContainerShapeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Sphere)));
  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_SphereInfo_Key, std::make_any<VectorFloat32Parameter::ValueType>(sphereInfo));

  args.insertOrAssign(ComputeCoordinateThresholdFilter::k_CreatedMaskPath_Key, std::make_any<DataPath>(k_MaskPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE(!executeResult.result.warnings().empty());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Image Geom Test - Rectangle", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});

#if 0
      0, 0, 0, 0, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 0, 0, 0, 0;
#endif

  VectorFloat32Parameter::ValueType minCoord = {1.0f, 1.0f, 0.0f};
  VectorFloat32Parameter::ValueType maxCoord = {4.0f, 4.0f, 1.0f};

  SECTION("Baseline")
  {
    RectangleExecuteFilter(dataStructure, false, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 0);
    REQUIRE(mask[2] == 0);
    REQUIRE(mask[3] == 0);
    REQUIRE(mask[4] == 0);

    REQUIRE(mask[5] == 0);
    REQUIRE(mask[6] == 1);
    REQUIRE(mask[7] == 1);
    REQUIRE(mask[8] == 1);
    REQUIRE(mask[9] == 0);

    REQUIRE(mask[10] == 0);
    REQUIRE(mask[11] == 1);
    REQUIRE(mask[12] == 1);
    REQUIRE(mask[13] == 1);
    REQUIRE(mask[14] == 0);

    REQUIRE(mask[15] == 0);
    REQUIRE(mask[16] == 1);
    REQUIRE(mask[17] == 1);
    REQUIRE(mask[18] == 1);
    REQUIRE(mask[19] == 0);

    REQUIRE(mask[20] == 0);
    REQUIRE(mask[21] == 0);
    REQUIRE(mask[22] == 0);
    REQUIRE(mask[23] == 0);
    REQUIRE(mask[24] == 0);
  }

  SECTION("Inverted")
  {
    RectangleExecuteFilter(dataStructure, true, minCoord, maxCoord);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 1);
    REQUIRE(mask[2] == 1);
    REQUIRE(mask[3] == 1);
    REQUIRE(mask[4] == 1);

    REQUIRE(mask[5] == 1);
    REQUIRE(mask[6] == 0);
    REQUIRE(mask[7] == 0);
    REQUIRE(mask[8] == 0);
    REQUIRE(mask[9] == 1);

    REQUIRE(mask[10] == 1);
    REQUIRE(mask[11] == 0);
    REQUIRE(mask[12] == 0);
    REQUIRE(mask[13] == 0);
    REQUIRE(mask[14] == 1);

    REQUIRE(mask[15] == 1);
    REQUIRE(mask[16] == 0);
    REQUIRE(mask[17] == 0);
    REQUIRE(mask[18] == 0);
    REQUIRE(mask[19] == 1);

    REQUIRE(mask[20] == 1);
    REQUIRE(mask[21] == 1);
    REQUIRE(mask[22] == 1);
    REQUIRE(mask[23] == 1);
    REQUIRE(mask[24] == 1);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeCoordinateThresholdFilter: Image Geom Test - Sphere", "[SimplnxCore][ComputeCoordinateThresholdFilter]")
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});

#if 0
      0, 0, 0, 0, 0,
      0, 0, 1, 0, 0,
      0, 1, 1, 1, 0,
      0, 0, 1, 0, 0,
      0, 0, 0, 0, 0;
#endif

  VectorFloat32Parameter::ValueType sphereInfo = {2.5f, 2.5f, 0.5f, 1.66f};

  SECTION("Baseline")
  {
    SphereExecuteFilter(dataStructure, false, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 0);
    REQUIRE(mask[1] == 0);
    REQUIRE(mask[2] == 0);
    REQUIRE(mask[3] == 0);
    REQUIRE(mask[4] == 0);

    REQUIRE(mask[5] == 0);
    REQUIRE(mask[6] == 0);
    REQUIRE(mask[7] == 1);
    REQUIRE(mask[8] == 0);
    REQUIRE(mask[9] == 0);

    REQUIRE(mask[10] == 0);
    REQUIRE(mask[11] == 1);
    REQUIRE(mask[12] == 1);
    REQUIRE(mask[13] == 1);
    REQUIRE(mask[14] == 0);

    REQUIRE(mask[15] == 0);
    REQUIRE(mask[16] == 0);
    REQUIRE(mask[17] == 1);
    REQUIRE(mask[18] == 0);
    REQUIRE(mask[19] == 0);

    REQUIRE(mask[20] == 0);
    REQUIRE(mask[21] == 0);
    REQUIRE(mask[22] == 0);
    REQUIRE(mask[23] == 0);
    REQUIRE(mask[24] == 0);
  }

  SECTION("Inverted")
  {
    SphereExecuteFilter(dataStructure, true, sphereInfo);

    const auto& mask = dataStructure.getDataRefAs<UInt8Array>(k_MaskPath);

    REQUIRE(mask[0] == 1);
    REQUIRE(mask[1] == 1);
    REQUIRE(mask[2] == 1);
    REQUIRE(mask[3] == 1);
    REQUIRE(mask[4] == 1);

    REQUIRE(mask[5] == 1);
    REQUIRE(mask[6] == 1);
    REQUIRE(mask[7] == 0);
    REQUIRE(mask[8] == 1);
    REQUIRE(mask[9] == 1);

    REQUIRE(mask[10] == 1);
    REQUIRE(mask[11] == 0);
    REQUIRE(mask[12] == 0);
    REQUIRE(mask[13] == 0);
    REQUIRE(mask[14] == 1);

    REQUIRE(mask[15] == 1);
    REQUIRE(mask[16] == 1);
    REQUIRE(mask[17] == 0);
    REQUIRE(mask[18] == 1);
    REQUIRE(mask[19] == 1);

    REQUIRE(mask[20] == 1);
    REQUIRE(mask[21] == 1);
    REQUIRE(mask[22] == 1);
    REQUIRE(mask[23] == 1);
    REQUIRE(mask[24] == 1);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  VectorFloat32Parameter::ValueType sphereInfo = {2.5f, 2.5f, 2.5f, 2.6f};

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  VectorFloat32Parameter::ValueType sphereInfo = {1.0f, 1.0f, 1.0f, 1.75f};

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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

  VectorFloat32Parameter::ValueType sphereInfo = {5.5f, 5.5f, 5.5f, 2.6f};

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
