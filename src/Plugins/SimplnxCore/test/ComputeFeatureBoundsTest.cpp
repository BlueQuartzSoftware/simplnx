#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureBounds.hpp"
#include "SimplnxCore/Filters/ComputeFeatureBoundsFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <cmath>
#include <memory>

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

constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkBlockSize = k_BenchmarkDim / 2;
constexpr usize k_BenchmarkFeatureCount = 8;
const std::string k_BenchmarkGeomName = "Benchmark ImageGeom";
const std::string k_BenchmarkCellDataName = "Cell Data";
const std::string k_BenchmarkFeatureDataName = "Feature Data";
const std::string k_BenchmarkFeatureIdsName = "FeatureIds";
const DataPath k_BenchmarkGeomPath({k_BenchmarkGeomName});
const DataPath k_BenchmarkCellDataPath = k_BenchmarkGeomPath.createChildPath(k_BenchmarkCellDataName);
const DataPath k_BenchmarkFeatureDataPath = k_BenchmarkGeomPath.createChildPath(k_BenchmarkFeatureDataName);
const DataPath k_BenchmarkFeatureIdsPath = k_BenchmarkCellDataPath.createChildPath(k_BenchmarkFeatureIdsName);

void BuildBenchmarkInput(DataStructure& dataStructure)
{
  const ShapeType cellTupleShape = {k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim};

  auto* imageGeom = ImageGeom::Create(dataStructure, k_BenchmarkGeomName);
  imageGeom->setDimensions({k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim});
  imageGeom->setOrigin({-10.0f, 2.0f, 5.0f});
  imageGeom->setSpacing({0.5f, 1.5f, 2.0f});

  auto* cellData = AttributeMatrix::Create(dataStructure, k_BenchmarkCellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_BenchmarkFeatureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = Int32Array::Create(dataStructure, k_BenchmarkFeatureIdsName, featureIdsStore, cellData->getId());
  auto& featureIdsStoreRef = featureIds->getDataStoreRef();

  constexpr usize k_SliceSize = k_BenchmarkDim * k_BenchmarkDim;
  auto sliceBuffer = std::make_unique<int32[]>(k_SliceSize);
  for(usize z = 0; z < k_BenchmarkDim; z++)
  {
    const int32 zBlock = z >= k_BenchmarkBlockSize ? 4 : 0;
    for(usize y = 0; y < k_BenchmarkDim; y++)
    {
      const int32 yBlock = y >= k_BenchmarkBlockSize ? 2 : 0;
      for(usize x = 0; x < k_BenchmarkDim; x++)
      {
        const int32 xBlock = x >= k_BenchmarkBlockSize ? 1 : 0;
        sliceBuffer[(y * k_BenchmarkDim) + x] = 1 + zBlock + yBlock + xBlock;
      }
    }

    const Result<> writeResult = featureIdsStoreRef.copyFromBuffer(z * k_SliceSize, nonstd::span<const int32>(sliceBuffer.get(), k_SliceSize));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  AttributeMatrix::Create(dataStructure, k_BenchmarkFeatureDataName, {k_BenchmarkFeatureCount + 1}, imageGeom->getId());
}
} // namespace

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Output Edge Geom Test - Image Geom/Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;

  const std::string k_GeomName = "ImageGeom";
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
  featureIds->fill(-1);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_CreateEdgeGeometry_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({"EdgeGeom"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>("EdgeAM"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_CreatedFeatureIdsArrayName_Key, std::make_any<std::string>("feature_ids"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // const std::array<float32, 6> expectedValues = std::array<float32, 6>{1.0f, 1.0f, 0.0f, 4.0f, 4.0f, 1.0f};
  static constexpr std::array<std::array<float32, 3>, 8> expectedVertices = {
      {{1.0f, 1.0f, 0.0f}, {4.0f, 1.0f, 0.0f}, {4.0f, 4.0f, 0.0f}, {1.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {4.0f, 1.0f, 1.0f}, {4.0f, 4.0f, 1.0f}, {1.0f, 4.0f, 1.0f}}};

  // define all 12 cube edges as pairs of vertex indices
  static constexpr std::array<std::array<int32, 2>, 12> expectedEdges = {{// bottom face
                                                                          {0, 1},
                                                                          {1, 2},
                                                                          {2, 3},
                                                                          {3, 0},
                                                                          // top face
                                                                          {4, 5},
                                                                          {5, 6},
                                                                          {6, 7},
                                                                          {7, 4},
                                                                          // vertical sides
                                                                          {0, 4},
                                                                          {1, 5},
                                                                          {2, 6},
                                                                          {3, 7}}};

  const auto& edgeGeom = dataStructure.getDataRefAs<EdgeGeom>(DataPath({"EdgeGeom"}));
  const auto& sharedVertList = edgeGeom.getVerticesRef();
  const auto& sharedEdgeList = edgeGeom.getEdgesRef();
  const auto& edgeFeatureIds = dataStructure.getDataRefAs<Int32Array>(DataPath({"EdgeGeom", "EdgeAM", "feature_ids"}));

  for(usize i = 0; i < sharedVertList.getNumberOfTuples(); i++)
  {
    REQUIRE(sharedVertList[(i * 3) + 0] == expectedVertices[i][0]);
    REQUIRE(sharedVertList[(i * 3) + 1] == expectedVertices[i][1]);
    REQUIRE(sharedVertList[(i * 3) + 2] == expectedVertices[i][2]);
  }

  for(usize i = 0; i < sharedEdgeList.getNumberOfTuples(); i++)
  {
    // Feature 0 is reserved, so begin comparisons at feature 1.
    REQUIRE(edgeFeatureIds[i] == 1);

    REQUIRE(sharedEdgeList[(i * 2) + 0] == expectedEdges[i][0]);
    REQUIRE(sharedEdgeList[(i * 2) + 1] == expectedEdges[i][1]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Image Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<float32, 6> expectedValues = std::array<float32, 6>{1.0f, 1.0f, 0.0f, 4.0f, 4.0f, 1.0f};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(k_FeatureAMPath.createChildPath("unified"));
  // Feature 0 is reserved, so begin comparisons at feature 1.
  for(usize j = 0; j < 6; j++)
  {
    REQUIRE(unified[6 + j] == expectedValues[j]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Image Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Vertex Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", k_VertexAttributeMatrixName, "feature_ids"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", "feature_data"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Vertex Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", k_VertexAttributeMatrixName, "feature_ids"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", "feature_data"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Edge Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_EdgeAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Edge Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_EdgeAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Triangle Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Triangle Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Quad Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Quad Geom Test - Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"feature_ids"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Invalid Execute - Feature AM Size Invalid", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
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
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FaceAMPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
