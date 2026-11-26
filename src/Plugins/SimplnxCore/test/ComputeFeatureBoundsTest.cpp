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
const std::vector<DataPath> k_TargetDataArrays{k_VertexDataPath.createChildPath("DataArray")};

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

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_BenchmarkGeomName);
  imageGeomPtr->setDimensions({k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim});
  REQUIRE(imageGeomPtr->setOrigin({-10.0f, 2.0f, 5.0f}).valid());
  REQUIRE(imageGeomPtr->setSpacing({0.5f, 1.5f, 2.0f}).valid());

  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, k_BenchmarkCellDataName, cellTupleShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellDataPtr);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_BenchmarkFeatureIdsPath, cellTupleShape, {1});
  auto* featureIdsPtr = Int32Array::Create(dataStructure, k_BenchmarkFeatureIdsName, featureIdsStore, cellDataPtr->getId());
  auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();

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

  AttributeMatrix::Create(dataStructure, k_BenchmarkFeatureDataName, {k_BenchmarkFeatureCount + 1}, imageGeomPtr->getId());
}
} // namespace

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Edge Geom Keeps All Boxes And Source Ids", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 3, 2});
  REQUIRE(imageGeomPtr->setOrigin({1, 2, 3}).valid());
  REQUIRE(imageGeomPtr->setSpacing({0.5f, 1, 2}).valid());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {2, 3, 4}, imageGeomPtr->getId());
  REQUIRE(cellAmPtr != nullptr);
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Feature Ids", {2, 3, 4}, {1}, cellAmPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  featureIdsPtr->fill(-1);
  (*featureIdsPtr)[0] = 1;
  for(usize cellIdx : {18, 19, 22, 23})
  {
    (*featureIdsPtr)[cellIdx] = 3;
  }
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", {4}, imageGeomPtr->getId()) != nullptr);

  ComputeFeatureBoundsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Cell Data", "Feature Ids"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Feature Data"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_CreateEdgeGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputEdgeGeometryPath_Key, std::make_any<DataPath>(DataPath({"EdgeGeom"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_EdgeAttributeMatrixName_Key, std::make_any<std::string>("Edge Data"));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_CreatedFeatureIdsArrayName_Key, std::make_any<std::string>("Edge Feature Ids"));
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<EdgeGeom>(DataPath({"EdgeGeom"})));
  const auto& edgeGeom = dataStructure.getDataRefAs<EdgeGeom>(DataPath({"EdgeGeom"}));
  const auto& vertices = edgeGeom.getVerticesRef();
  const auto& edges = edgeGeom.getEdgesRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"EdgeGeom", "Edge Data", "Edge Feature Ids"})));
  const auto& edgeFeatureIds = dataStructure.getDataRefAs<Int32Array>(DataPath({"EdgeGeom", "Edge Data", "Edge Feature Ids"}));

  SECTION("Box counts and coordinates")
  {
    REQUIRE(vertices.getNumberOfTuples() == 16);
    REQUIRE(edges.getNumberOfTuples() == 24);
    const std::array<usize, 4> vertexIndices = {0, 6, 8, 14};
    const std::array<std::array<float32, 3>, 4> expectedVertices = {{{1, 2, 3}, {1.5f, 3, 5}, {2, 3, 5}, {3, 5, 7}}};
    for(usize vertexIdx = 0; vertexIdx < vertexIndices.size(); vertexIdx++)
    {
      for(usize compIdx = 0; compIdx < 3; compIdx++)
      {
        REQUIRE(vertices[vertexIndices[vertexIdx] * 3 + compIdx] == expectedVertices[vertexIdx][compIdx]);
      }
    }
    REQUIRE(edges[12 * 2] == 8);
    REQUIRE(edges[12 * 2 + 1] == 9);
    REQUIRE(edges[23 * 2] == 11);
    REQUIRE(edges[23 * 2 + 1] == 15);
  }
  SECTION("Source Feature Ids")
  {
    REQUIRE(edgeFeatureIds.getNumberOfTuples() >= 12);
    for(usize edgeIdx = 0; edgeIdx < 12; edgeIdx++)
    {
      REQUIRE(edgeFeatureIds[edgeIdx] == 1);
    }
    REQUIRE(edgeFeatureIds.getNumberOfTuples() == 24);
    for(usize edgeIdx = 12; edgeIdx < 24; edgeIdx++)
    {
      REQUIRE(edgeFeatureIds[edgeIdx] == 3);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Feature AM Larger Than Max Id", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;

  auto* vertexGeomPtr = VertexGeom::Create(dataStructure, "VertexGeom");
  REQUIRE(vertexGeomPtr != nullptr);
  auto* verticesPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {3}, {3}, vertexGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  vertexGeomPtr->setVertices(*verticesPtr);
  const std::array<float32, 9> coordinates = {-1, 0.5f, 7, 1, 2, 3, 4, 6, 9};
  for(usize valueIdx = 0; valueIdx < coordinates.size(); valueIdx++)
  {
    (*verticesPtr)[valueIdx] = coordinates[valueIdx];
  }
  auto* vertexAmPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", {3}, vertexGeomPtr->getId());
  REQUIRE(vertexAmPtr != nullptr);
  vertexGeomPtr->setVertexAttributeMatrix(*vertexAmPtr);
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Feature Ids", {3}, {1}, vertexAmPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  (*featureIdsPtr)[0] = 0;
  (*featureIdsPtr)[1] = 1;
  (*featureIdsPtr)[2] = 1;
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", {5}, vertexGeomPtr->getId()) != nullptr);

  ComputeFeatureBoundsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", "Vertex Data", "Feature Ids"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"VertexGeom", "Feature Data"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("Bounds"));
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath boundsPath({"VertexGeom", "Feature Data", "Bounds"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(boundsPath));
  const auto& bounds = dataStructure.getDataRefAs<Float32Array>(boundsPath);
  REQUIRE(bounds.getNumberOfTuples() == 5);
  REQUIRE(bounds.getNumberOfComponents() == 6);
  const std::array<float32, 12> expectedBounds = {-1, 0.5f, 7, -1, 0.5f, 7, 1, 2, 3, 4, 6, 9};
  for(usize valueIdx = 0; valueIdx < expectedBounds.size(); valueIdx++)
  {
    REQUIRE(bounds[valueIdx] == expectedBounds[valueIdx]);
  }
  for(usize featureIdx = 2; featureIdx < 5; featureIdx++)
  {
    for(usize compIdx = 0; compIdx < 6; compIdx++)
    {
      CAPTURE(featureIdx, compIdx, bounds[featureIdx * 6 + compIdx]);
      REQUIRE(std::isnan(bounds[featureIdx * 6 + compIdx]));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Reject Multi Component Feature Ids", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({1, 1, 1});
  REQUIRE(imageGeomPtr->setOrigin({0, 0, 0}).valid());
  REQUIRE(imageGeomPtr->setSpacing({1, 1, 1}).valid());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "Cell Data", {1, 1, 1}, imageGeomPtr->getId());
  REQUIRE(cellAmPtr != nullptr);
  imageGeomPtr->setCellData(*cellAmPtr);
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Feature Ids", {1, 1, 1}, {2}, cellAmPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  featureIdsPtr->fill(0);
  REQUIRE(AttributeMatrix::Create(dataStructure, "Feature Data", {1}, imageGeomPtr->getId()) != nullptr);

  ComputeFeatureBoundsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Cell Data", "Feature Ids"})));
  args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(DataPath({"ImageGeom", "Feature Data"})));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -208);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Output Edge Geom Test - Image Geom/Split", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;

  const std::string kGeomName = "ImageGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, kGeomName);
  constexpr size_t k_DimsIn[3] = {5, 5, 1};
  imageGeomPtr->setDimensions(k_DimsIn);
  REQUIRE(imageGeomPtr->setOrigin({0, 0, 0}).valid());
  REQUIRE(imageGeomPtr->setSpacing({1, 1, 1}).valid());
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  const std::string kCellAmName = "Cell Data";
  const DataPath kCellAmPath = kGeomPath.createChildPath(kCellAmName);
  AttributeMatrix* cellAmPtr = AttributeMatrix::Create(dataStructure, kCellAmName, dims, imageGeomPtr->getId());

  Int32Array* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "feature_ids", dims, std::vector<usize>{1}, cellAmPtr->getId());
  featureIdsPtr->fill(-1);
  (*featureIdsPtr)[6] = 1;
  (*featureIdsPtr)[7] = 1;
  (*featureIdsPtr)[8] = 1;
  (*featureIdsPtr)[11] = 1;
  (*featureIdsPtr)[12] = 1;
  (*featureIdsPtr)[13] = 1;
  (*featureIdsPtr)[16] = 1;
  (*featureIdsPtr)[17] = 1;
  (*featureIdsPtr)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAmPtr = AttributeMatrix::Create(dataStructure, kFeatureAmName, dims, imageGeomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kCellAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
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
  static constexpr std::array<std::array<float32, 3>, 8> k_ExpectedVertices = {
      {{1.0f, 1.0f, 0.0f}, {4.0f, 1.0f, 0.0f}, {4.0f, 4.0f, 0.0f}, {1.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {4.0f, 1.0f, 1.0f}, {4.0f, 4.0f, 1.0f}, {1.0f, 4.0f, 1.0f}}};

  // define all 12 cube edges as pairs of vertex indices
  static constexpr std::array<std::array<int32, 2>, 12> k_ExpectedEdges = {{// bottom face
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
    REQUIRE(sharedVertList[(i * 3) + 0] == k_ExpectedVertices[i][0]);
    REQUIRE(sharedVertList[(i * 3) + 1] == k_ExpectedVertices[i][1]);
    REQUIRE(sharedVertList[(i * 3) + 2] == k_ExpectedVertices[i][2]);
  }

  for(usize i = 0; i < sharedEdgeList.getNumberOfTuples(); i++)
  {
    // Feature 0 is reserved, so begin comparisons at feature 1.
    REQUIRE(edgeFeatureIds[i] == 1);

    REQUIRE(sharedEdgeList[(i * 2) + 0] == k_ExpectedEdges[i][0]);
    REQUIRE(sharedEdgeList[(i * 2) + 1] == k_ExpectedEdges[i][1]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeFeatureBoundsFilter: Image Geom Test - Unified", "[SimplnxCore][ComputeFeatureBoundsFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;

  const std::string kGeomName = "EdgeGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, kGeomName);
  constexpr size_t k_DimsIn[3] = {5, 5, 1};
  imageGeomPtr->setDimensions(k_DimsIn);
  REQUIRE(imageGeomPtr->setOrigin({0, 0, 0}).valid());
  REQUIRE(imageGeomPtr->setSpacing({1, 1, 1}).valid());
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  const std::string kCellAmName = "Cell Data";
  const DataPath kCellAmPath = kGeomPath.createChildPath(kCellAmName);
  AttributeMatrix* cellAmPtr = AttributeMatrix::Create(dataStructure, kCellAmName, dims, imageGeomPtr->getId());

  Int32Array* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "feature_ids", dims, std::vector<usize>{1}, cellAmPtr->getId());
  featureIdsPtr->fill(0);
  (*featureIdsPtr)[6] = 1;
  (*featureIdsPtr)[7] = 1;
  (*featureIdsPtr)[8] = 1;
  (*featureIdsPtr)[11] = 1;
  (*featureIdsPtr)[12] = 1;
  (*featureIdsPtr)[13] = 1;
  (*featureIdsPtr)[16] = 1;
  (*featureIdsPtr)[17] = 1;
  (*featureIdsPtr)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAmPtr = AttributeMatrix::Create(dataStructure, kFeatureAmName, dims, imageGeomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kCellAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<float32, 6> expectedValues = std::array<float32, 6>{1.0f, 1.0f, 0.0f, 4.0f, 4.0f, 1.0f};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("unified"));
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

  const std::string kGeomName = "EdgeGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, kGeomName);
  constexpr size_t k_DimsIn[3] = {5, 5, 1};
  imageGeomPtr->setDimensions(k_DimsIn);
  REQUIRE(imageGeomPtr->setOrigin({0, 0, 0}).valid());
  REQUIRE(imageGeomPtr->setSpacing({1, 1, 1}).valid());
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  const std::string kCellAmName = "Cell Data";
  const DataPath kCellAmPath = kGeomPath.createChildPath(kCellAmName);
  AttributeMatrix* cellAmPtr = AttributeMatrix::Create(dataStructure, kCellAmName, dims, imageGeomPtr->getId());

  Int32Array* featureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "feature_ids", dims, std::vector<usize>{1}, cellAmPtr->getId());
  featureIdsPtr->fill(0);
  (*featureIdsPtr)[6] = 1;
  (*featureIdsPtr)[7] = 1;
  (*featureIdsPtr)[8] = 1;
  (*featureIdsPtr)[11] = 1;
  (*featureIdsPtr)[12] = 1;
  (*featureIdsPtr)[13] = 1;
  (*featureIdsPtr)[16] = 1;
  (*featureIdsPtr)[17] = 1;
  (*featureIdsPtr)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAmPtr = AttributeMatrix::Create(dataStructure, kFeatureAmName, dims, imageGeomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kCellAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const std::array<float32, 6> expectedValues = std::array<float32, 6>{1.0f, 1.0f, 0.0f, 4.0f, 4.0f, 1.0f};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("max"));
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
  auto* vertexGeomPtr = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeomPtr->getId());
  vertexGeomPtr->setVertices(*vertexArrayPtr);

  auto* vertexAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, k_VertexAttributeMatrixName, {k_TupleCount}, vertexGeomPtr->getId());
  vertexGeomPtr->setVertexAttributeMatrix(*vertexAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, vertexAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = (i % 2) + 1;
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  AttributeMatrix::Create(dataStructure, "feature_data", {2 + 1}, vertexGeomPtr->getId());

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
  auto* vertexGeomPtr = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, vertexGeomPtr->getId());
  vertexGeomPtr->setVertices(*vertexArrayPtr);

  auto* vertexAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, k_VertexAttributeMatrixName, {k_TupleCount}, vertexGeomPtr->getId());
  vertexGeomPtr->setVertexAttributeMatrix(*vertexAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, vertexAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    dataStore[i] = (i % 2) + 1;
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }

  AttributeMatrix::Create(dataStructure, "feature_data", {2 + 1}, vertexGeomPtr->getId());

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

  const std::string kGeomName = "EdgeGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = EdgeGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* edgesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "edges", {k_TupleCount}, {2}, geomPtr->getId());
  geomPtr->setEdgeList(*edgesArrayPtr);

  const std::string kEdgeAmName = "Edge Data";
  const DataPath kEdgeAmPath = kGeomPath.createChildPath(kEdgeAmName);
  auto* edgeAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kEdgeAmName, {k_TupleCount}, geomPtr->getId());
  geomPtr->setEdgeAttributeMatrix(*edgeAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, edgeAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  auto& edges = edgesArrayPtr->getDataStoreRef();
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

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {2 + 1}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kEdgeAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}, std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("unified"));
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

  const std::string kGeomName = "EdgeGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = EdgeGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* edgesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "edges", {k_TupleCount}, {2}, geomPtr->getId());
  geomPtr->setEdgeList(*edgesArrayPtr);

  const std::string kEdgeAmName = "Edge Data";
  const DataPath kEdgeAmPath = kGeomPath.createChildPath(kEdgeAmName);
  auto* edgeAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kEdgeAmName, {k_TupleCount}, geomPtr->getId());
  geomPtr->setEdgeAttributeMatrix(*edgeAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {k_TupleCount}, {1}, edgeAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  auto& edges = edgesArrayPtr->getDataStoreRef();
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

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {2 + 1}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kEdgeAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}, std::array<float32, 6>{0.0f, 0.0f, 0.0f, 7.0f, 7.0f, 7.0f}};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("max"));
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

  const std::string kGeomName = "TriangleGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = TriangleGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* facesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geomPtr->getId());
  geomPtr->setFaceList(*facesArrayPtr);

  const std::string kFaceAmName = "Face Data";
  const DataPath kFaceAmPath = kGeomPath.createChildPath(kFaceAmName);
  auto* faceAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kFaceAmName, {2}, geomPtr->getId());
  geomPtr->setFaceAttributeMatrix(*faceAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  auto& faces = facesArrayPtr->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {2 + 1}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kFaceAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f}, std::array<float32, 6>{3.0f, 3.0f, 3.0f, 5.0f, 5.0f, 5.0f}};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("unified"));
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

  const std::string kGeomName = "TriangleGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = TriangleGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* facesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geomPtr->getId());
  geomPtr->setFaceList(*facesArrayPtr);

  const std::string kFaceAmName = "Face Data";
  const DataPath kFaceAmPath = kGeomPath.createChildPath(kFaceAmName);
  auto* faceAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kFaceAmName, {2}, geomPtr->getId());
  geomPtr->setFaceAttributeMatrix(*faceAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  auto& faces = facesArrayPtr->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {3}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kFaceAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f}, std::array<float32, 6>{3.0f, 3.0f, 3.0f, 5.0f, 5.0f, 5.0f}};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("max"));
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

  const std::string kGeomName = "QuadGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = QuadGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* facesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {4}, geomPtr->getId());
  geomPtr->setFaceList(*facesArrayPtr);

  const std::string kFaceAmName = "Face Data";
  const DataPath kFaceAmPath = kGeomPath.createChildPath(kFaceAmName);
  auto* faceAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kFaceAmName, {2}, geomPtr->getId());
  geomPtr->setFaceAttributeMatrix(*faceAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  auto& faces = facesArrayPtr->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {2 + 1}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kFaceAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_UnifiedArrayName_Key, std::make_any<std::string>("unified"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f}, std::array<float32, 6>{4.0f, 4.0f, 4.0f, 7.0f, 7.0f, 7.0f}};

  const auto& unified = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("unified"));
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

  const std::string kGeomName = "QuadGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = QuadGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {k_TupleCount}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* facesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {4}, geomPtr->getId());
  geomPtr->setFaceList(*facesArrayPtr);

  const std::string kFaceAmName = "Face Data";
  const DataPath kFaceAmPath = kGeomPath.createChildPath(kFaceAmName);
  auto* faceAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kFaceAmName, {2}, geomPtr->getId());
  geomPtr->setFaceAttributeMatrix(*faceAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  auto& faces = facesArrayPtr->getDataStoreRef();
  for(usize i = 0; i < k_TupleCount; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 2;

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {2 + 1}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kFaceAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  std::array<std::array<float32, 6>, 2> expectedValues = {std::array<float32, 6>{0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 3.0f}, std::array<float32, 6>{4.0f, 4.0f, 4.0f, 7.0f, 7.0f, 7.0f}};

  const auto& min = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("min"));
  const auto& max = dataStructure.getDataRefAs<Float32Array>(kFeatureAmPath.createChildPath("max"));
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

  const std::string kGeomName = "TriangleGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = TriangleGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* facesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geomPtr->getId());
  geomPtr->setFaceList(*facesArrayPtr);

  const std::string kFaceAmName = "Face Data";
  const DataPath kFaceAmPath = kGeomPath.createChildPath(kFaceAmName);
  auto* faceAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kFaceAmName, {2}, geomPtr->getId());
  geomPtr->setFaceAttributeMatrix(*faceAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {4}, {1});

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {3}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"feature_ids"})));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
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

  const std::string kGeomName = "TriangleGeom";
  const DataPath kGeomPath({kGeomName});
  const std::string kFeatureAmName = "Feature Data";
  const DataPath kFeatureAmPath = kGeomPath.createChildPath(kFeatureAmName);

  auto* geomPtr = TriangleGeom::Create(dataStructure, kGeomName);
  auto* vertexArrayPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "Vertices", {6}, {3}, geomPtr->getId());
  geomPtr->setVertices(*vertexArrayPtr);

  auto* facesArrayPtr = IGeometry::MeshIndexArrayType::CreateWithStore<DataStore<IGeometry::MeshIndexType>>(dataStructure, "faces", {2}, {3}, geomPtr->getId());
  geomPtr->setFaceList(*facesArrayPtr);

  const std::string kFaceAmName = "Face Data";
  const DataPath kFaceAmPath = kGeomPath.createChildPath(kFaceAmName);
  auto* faceAttributeMatrixPtr = AttributeMatrix::Create(dataStructure, kFaceAmName, {2}, geomPtr->getId());
  geomPtr->setFaceAttributeMatrix(*faceAttributeMatrixPtr);

  auto* dataArrayPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "feature_ids", {2}, {1}, faceAttributeMatrixPtr->getId());
  auto& dataStore = dataArrayPtr->getDataStoreRef();
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  auto& faces = facesArrayPtr->getDataStoreRef();
  for(usize i = 0; i < 6; ++i)
  {
    faces[i] = i;
    vertices[(i * 3) + 0] = i;
    vertices[(i * 3) + 1] = i;
    vertices[(i * 3) + 2] = i;
  }
  dataStore[0] = 1;
  dataStore[1] = 4;

  AttributeMatrix::Create(dataStructure, kFeatureAmName, {3}, geomPtr->getId());

  {
    // Configure the filter arguments.
    ComputeFeatureBoundsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureBoundsFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_SelectedGeometryPath_Key, std::make_any<DataPath>(kGeomPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(kFaceAmPath.createChildPath("feature_ids")));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(kFeatureAmPath));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MinArrayName_Key, std::make_any<std::string>("min"));
    args.insertOrAssign(ComputeFeatureBoundsFilter::k_MaxArrayName_Key, std::make_any<std::string>("max"));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
