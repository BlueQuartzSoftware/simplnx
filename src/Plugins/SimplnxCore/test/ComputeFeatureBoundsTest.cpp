#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureBounds.hpp"
#include "SimplnxCore/Filters/ComputeFeatureBoundsFilter.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

namespace
{
const DataPath k_FeatureIdsPath({"feature_ids"});

constexpr uint64 k_TupleCount = 8;
constexpr StringLiteral k_VertexAttributeMatrixName = "VertexData";
const DataPath k_VertexGeomPath{std::vector<std::string>{"VertexGeom"}};
const DataPath k_VertexDataPath = k_VertexGeomPath.createChildPath(k_VertexAttributeMatrixName);
const DataPath k_CroppedGeomPath{std::vector<std::string>{"Cropped VertexGeom"}};
const std::vector<DataPath> targetDataArrays{k_VertexDataPath.createChildPath("DataArray")};
} // namespace

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Image Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{

  DataStructure dataStructure;

  const std::string k_GeomName = "EdgeGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  const std::string k_CellAMName = "Cell Data";
  const DataPath k_CellAMPath = k_GeomPath.createChildPath(k_CellAMName);
  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_CellAMName, dims, imageGeom->getId());

  Int32Array* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "feature_ids", dims, std::vector<usize>{1}, cellAm->getId());
  featureIds->fill(0);
  (*featureIds)[6] = 1;
  (*featureIds)[7] = 1;
  (*featureIds)[8] = 1;
  (*featureIds)[11] = 1;
  (*featureIds)[12] = 1;
  (*featureIds)[13] = 1;
  (*featureIds)[16] = 1;
  (*featureIds)[17] = 1;
  (*featureIds)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAm = AttributeMatrix::Create(dataStructure, k_FeatureAMName, dims, imageGeom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<float32, 6> expectedValues = std::array<float32, 6>{1.0f, 1.0f, 0.0f, 4.0f, 4.0f, 1.0f};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("unified"));
  // Start from 1 because feature 0 is junk
  for(usize j = 0; j < 6; j++)
  {
    REQUIRE(unified[6 + j] == expectedValues[j]);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Image Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{

  DataStructure dataStructure;

  const std::string k_GeomName = "EdgeGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  const std::string k_CellAMName = "Cell Data";
  const DataPath k_CellAMPath = k_GeomPath.createChildPath(k_CellAMName);
  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_CellAMName, dims, imageGeom->getId());

  Int32Array* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "feature_ids", dims, std::vector<usize>{1}, cellAm->getId());
  featureIds->fill(0);
  (*featureIds)[6] = 1;
  (*featureIds)[7] = 1;
  (*featureIds)[8] = 1;
  (*featureIds)[11] = 1;
  (*featureIds)[12] = 1;
  (*featureIds)[13] = 1;
  (*featureIds)[16] = 1;
  (*featureIds)[17] = 1;
  (*featureIds)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAm = AttributeMatrix::Create(dataStructure, k_FeatureAMName, dims, imageGeom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<float32, 6> expectedValues = std::array<float32, 6>{1.0f, 1.0f, 0.0f, 4.0f, 4.0f, 1.0f};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("max"));
  for(usize j = 0; j < 6; j++)
  {
    if(j < 3)
    {
      REQUIRE(min[3 + j] == expectedValues[j]);
    }
    else
    {
      REQUIRE(max[3 + j - 3] == expectedValues[j]);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Vertex Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, k_VertexAttributeMatrixName, {k_TupleCount}, vertexGeom->getId());
  vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, vertexAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = (i % 2) + 1;
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  AttributeMatrix::Create(dataStructure, "feature_data", {2 + 1}, vertexGeom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", k_VertexAttributeMatrixName, "feature_ids"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", "feature_data"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 6.0f, 6.0f, 6.0f}, std::array<float32, 6>{1.0f, 1.0f, 1.0f, 7.0f, 7.0f, 7.0f}};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(DataPath({"VertexGeom", "feature_data", "unified"}));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < unified.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      REQUIRE(unified[(i * 6) + j] == expectedValues[i - 1][j]);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Vertex Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertexArray);

  auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, k_VertexAttributeMatrixName, {k_TupleCount}, vertexGeom->getId());
  vertexGeom->setVertexAttributeMatrix(*vertexAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, vertexAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = (i % 2) + 1;
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  AttributeMatrix::Create(dataStructure, "feature_data", {2 + 1}, vertexGeom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", k_VertexAttributeMatrixName, "feature_ids"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", "feature_data"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 6.0f, 6.0f, 6.0f}, std::array<float32, 6>{1.0f, 1.0f, 1.0f, 7.0f, 7.0f, 7.0f}};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(DataPath({"VertexGeom", "feature_data", "min"}));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(DataPath({"VertexGeom", "feature_data", "max"}));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < min.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      if(j < 3)
      {
        REQUIRE(min[(i * 3) + j] == expectedValues[i - 1][j]);
      }
      else
      {
        REQUIRE(max[(i * 3) + j - 3] == expectedValues[i - 1][j]);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Edge Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "EdgeGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = EdgeGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* edgesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "edges", {k_TupleCount}, {2}, geom->getId());
  geom->setEdgeList(*edgesArray);

  const std::string k_EdgeAMName = "Edge Data";
  const DataPath k_EdgeAMPath = k_GeomPath.createChildPath(k_EdgeAMName);
  auto* edgeAttributeMatrix = AttributeMatrix::Create(dataStructure, k_EdgeAMName, {k_TupleCount}, geom->getId());
  geom->setEdgeAttributeMatrix(*edgeAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, edgeAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  auto& edges = edgesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = (i % 2) + 1;
    edges[(i * 2) + 0] = i;
    edges[(i * 2) + 1] = i + 1;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  edges[((k_TupleCount - 1) * 2) + 1] = 0;

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {2 + 1}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_EdgeAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}, std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("unified"));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < unified.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      REQUIRE(unified[(i * 6) + j] == expectedValues[i - 1][j]);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Edge Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "EdgeGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = EdgeGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* edgesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "edges", {k_TupleCount}, {2}, geom->getId());
  geom->setEdgeList(*edgesArray);

  const std::string k_EdgeAMName = "Edge Data";
  const DataPath k_EdgeAMPath = k_GeomPath.createChildPath(k_EdgeAMName);
  auto* edgeAttributeMatrix = AttributeMatrix::Create(dataStructure, k_EdgeAMName, {k_TupleCount}, geom->getId());
  geom->setEdgeAttributeMatrix(*edgeAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, edgeAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  auto& edges = edgesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = (i % 2) + 1;
    edges[(i * 2) + 0] = i;
    edges[(i * 2) + 1] = i + 1;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  edges[((k_TupleCount - 1) * 2) + 1] = 0;

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {2 + 1}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_EdgeAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}, std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("max"));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < min.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      if(j < 3)
      {
        REQUIRE(min[(i * 3) + j] == expectedValues[i - 1][j]);
      }
      else
      {
        REQUIRE(max[(i * 3) + j - 3] == expectedValues[i - 1][j]);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Triangle Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "TriangleGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = TriangleGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geom->getId());
  geom->setFaceList(*facesArray);

  const std::string k_FaceAMName = "Face Data";
  const DataPath k_FaceAMPath = k_GeomPath.createChildPath(k_FaceAMName);
  auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, k_FaceAMName, {2}, geom->getId());
  geom->setFaceAttributeMatrix(*faceAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {2 + 1}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f}, std::array<float32, 6>{3.0f, 3.0f, 3.0f, 5.0f, 5.0f, 5.0f}};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("unified"));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < unified.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      REQUIRE(unified[(i * 6) + j] == expectedValues[i - 1][j]);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Triangle Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "TriangleGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = TriangleGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geom->getId());
  geom->setFaceList(*facesArray);

  const std::string k_FaceAMName = "Face Data";
  const DataPath k_FaceAMPath = k_GeomPath.createChildPath(k_FaceAMName);
  auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, k_FaceAMName, {2}, geom->getId());
  geom->setFaceAttributeMatrix(*faceAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {3}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f}, std::array<float32, 6>{3.0f, 3.0f, 3.0f, 5.0f, 5.0f, 5.0f}};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("max"));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < min.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      if(j < 3)
      {
        REQUIRE(min[(i * 3) + j] == expectedValues[i - 1][j]);
      }
      else
      {
        REQUIRE(max[(i * 3) + j - 3] == expectedValues[i - 1][j]);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Quad Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "QuadGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = QuadGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {4}, geom->getId());
  geom->setFaceList(*facesArray);

  const std::string k_FaceAMName = "Face Data";
  const DataPath k_FaceAMPath = k_GeomPath.createChildPath(k_FaceAMName);
  auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, k_FaceAMName, {2}, geom->getId());
  geom->setFaceAttributeMatrix(*faceAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {2 + 1}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f}, std::array<float32, 6>{4.0f, 4.0f, 4.0f, 7.0f, 7.0f, 7.0f}};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("unified"));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < unified.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      REQUIRE(unified[(i * 6) + j] == expectedValues[i - 1][j]);
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Quad Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "QuadGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = QuadGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {4}, geom->getId());
  geom->setFaceList(*facesArray);

  const std::string k_FaceAMName = "Face Data";
  const DataPath k_FaceAMPath = k_GeomPath.createChildPath(k_FaceAMName);
  auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, k_FaceAMName, {2}, geom->getId());
  geom->setFaceAttributeMatrix(*faceAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {2 + 1}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f}, std::array<float32, 6>{4.0f, 4.0f, 4.0f, 7.0f, 7.0f, 7.0f}};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("max"));
  // Start from 1 because feature 0 is junk
  for(usize i = 1; i < min.getNumberOfTuples(); i++)
  {
    for(usize j = 0; j < 6; j++)
    {
      if(j < 3)
      {
        REQUIRE(min[(i * 3) + j] == expectedValues[i - 1][j]);
      }
      else
      {
        REQUIRE(max[(i * 3) + j - 3] == expectedValues[i - 1][j]);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Invalid Preflight - Unexpected Feature Id Size", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "TriangleGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = TriangleGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geom->getId());
  geom->setFaceList(*facesArray);

  const std::string k_FaceAMName = "Face Data";
  const DataPath k_FaceAMPath = k_GeomPath.createChildPath(k_FaceAMName);
  auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, k_FaceAMName, {2}, geom->getId());
  geom->setFaceAttributeMatrix(*faceAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {4}, {1});

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {3}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"feature_ids"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Invalid Execute - Feature AM Size Invalid", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  DataStructure dataStructure;

  const std::string k_GeomName = "TriangleGeom";
  const DataPath k_GeomPath({k_GeomName});
  const std::string k_FeatureAMName = "Feature Data";
  const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);

  auto* geom = TriangleGeom::Create(dataStructure, k_GeomName);
  auto* vertexArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geom->getId());
  geom->setVertices(*vertexArray);

  auto* facesArray = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geom->getId());
  geom->setFaceList(*facesArray);

  const std::string k_FaceAMName = "Face Data";
  const DataPath k_FaceAMPath = k_GeomPath.createChildPath(k_FaceAMName);
  auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, k_FaceAMName, {2}, geom->getId());
  geom->setFaceAttributeMatrix(*faceAttributeMatrix);

  auto* dataArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrix->getId());
  auto& dataStore = dataArray->getDataStoreRef();
  auto& vertices = vertexArray->getDataStoreRef();
  auto& faces = facesArray->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 4;

  AttributeMatrix::Create(dataStructure, k_FeatureAMName, {3}, geom->getId());

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }
}
