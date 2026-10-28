#include "SimplnxCore/Filters/LaplacianSmoothingFilter.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <array>
#include <cmath>
#include <string_view>

using namespace nx::core;

namespace
{
// The four-vertex cache fits without a temporary record-store provider.
class SmoothingBudgetScope
{
public:
  SmoothingBudgetScope()
  : m_OriginalBudget(CacheMemoryBudgetManager::instance().budgetBytes())
  {
    CacheMemoryBudgetManager::instance().setBudgetBytes(1 << 20);
  }
  ~SmoothingBudgetScope()
  {
    CacheMemoryBudgetManager::instance().setBudgetBytes(m_OriginalBudget);
  }
  SmoothingBudgetScope(const SmoothingBudgetScope&) = delete;
  SmoothingBudgetScope(SmoothingBudgetScope&&) = delete;
  SmoothingBudgetScope& operator=(const SmoothingBudgetScope&) = delete;
  SmoothingBudgetScope& operator=(SmoothingBudgetScope&&) = delete;

private:
  uint64 m_OriginalBudget;
};

/** @brief Stores all three coordinates of the four analytic vertices. */
using Coordinates = std::array<float32, 12>;
/** @brief Stores one classification for each analytic vertex. */
using NodeTypes = std::array<int8, 4>;

/** @brief Identifies the graph geometry selected by the filter. */
const DataPath k_GeometryPath{{"Mesh"}};
/** @brief Identifies the coordinate array modified in place. */
const DataPath k_VerticesPath{{"Mesh", "Vertices"}};
/** @brief Identifies the unchanged vertex classifications. */
const DataPath k_NodeTypesPath{{"Mesh", "Vertex Data", "Node Types"}};
/** @brief Identifies the supplied faces or edges that must survive smoothing. */
const DataPath k_ConnectivityPath{{"Mesh", "Connectivity"}};
/** @brief Supplies a tetrahedron with centroid (1/2, 1/2, 1/2). */
constexpr Coordinates k_InputCoordinates{0.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F, 0.0F, 2.0F};
/** @brief Supplies the four faces whose unique edges form the complete four-vertex graph. */
constexpr std::array<IGeometry::MeshIndexType, 12> k_Faces{0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3};
/** @brief Supplies the same complete graph directly as six edges. */
constexpr std::array<IGeometry::MeshIndexType, 12> k_Edges{0, 1, 0, 2, 0, 3, 1, 2, 1, 3, 2, 3};

/**
 * @enum GeometryKind
 * @brief Selects generated or supplied edge connectivity for the same graph.
 */
enum class GeometryKind
{
  Triangle, ///< Generates edges from four triangular faces.
  Edge      ///< Uses six supplied edges.
};

/**
 * @struct NodeTypeCase
 * @brief Pairs one node classification with independently derived one-pass coordinates.
 */
struct NodeTypeCase
{
  std::string_view name;
  int8 nodeType;
  Coordinates expected;
};

// For K4, a uniform pass scales each displacement from the centroid by 1 - 4*lambda/3.
// These explicit coordinates use that closed form, not the production edge traversal.
constexpr std::array<NodeTypeCase, 7> k_NodeTypeCases{{
    {"Unused", NodeType::Unused, k_InputCoordinates},
    {"Default", NodeType::Default, {1.0F / 6.0F, 1.0F / 6.0F, 1.0F / 6.0F, 3.0F / 2.0F, 1.0F / 6.0F, 1.0F / 6.0F, 1.0F / 6.0F, 3.0F / 2.0F, 1.0F / 6.0F, 1.0F / 6.0F, 1.0F / 6.0F, 3.0F / 2.0F}},
    {"TriplePoint", NodeType::TriplePoint, {1.0F / 3.0F, 1.0F / 3.0F, 1.0F / 3.0F, 1.0F, 1.0F / 3.0F, 1.0F / 3.0F, 1.0F / 3.0F, 1.0F, 1.0F / 3.0F, 1.0F / 3.0F, 1.0F / 3.0F, 1.0F}},
    {"QuadPoint", NodeType::QuadPoint, {0.5F, 0.5F, 0.5F, 0.5F, 0.5F, 0.5F, 0.5F, 0.5F, 0.5F, 0.5F, 0.5F, 0.5F}},
    {"SurfaceDefault",
     NodeType::SurfaceDefault,
     {1.0F / 12.0F, 1.0F / 12.0F, 1.0F / 12.0F, 7.0F / 4.0F, 1.0F / 12.0F, 1.0F / 12.0F, 1.0F / 12.0F, 7.0F / 4.0F, 1.0F / 12.0F, 1.0F / 12.0F, 1.0F / 12.0F, 7.0F / 4.0F}},
    {"SurfaceTriplePoint", NodeType::SurfaceTriplePoint, {0.25F, 0.25F, 0.25F, 1.25F, 0.25F, 0.25F, 0.25F, 1.25F, 0.25F, 0.25F, 0.25F, 1.25F}},
    {"SurfaceQuadPoint",
     NodeType::SurfaceQuadPoint,
     {5.0F / 12.0F, 5.0F / 12.0F, 5.0F / 12.0F, 0.75F, 5.0F / 12.0F, 5.0F / 12.0F, 5.0F / 12.0F, 0.75F, 5.0F / 12.0F, 5.0F / 12.0F, 5.0F / 12.0F, 0.75F}},
}};

/**
 * @struct IterationCase
 * @brief Supplies an independent oracle for repeated passes or the optional Taubin pass.
 */
struct IterationCase
{
  std::string_view name;
  int32 iterations;
  bool useTaubin;
  Coordinates expected;
};

// Two lambda=1/4 passes have centroid factor 4/9. One Taubin pair with a mu factor of -1 has a centroid factor of 8/9.
constexpr std::array<IterationCase, 4> k_IterationCases{{
    {"zero plain iterations", 0, false, k_InputCoordinates},
    {"zero Taubin iterations", 0, true, k_InputCoordinates},
    {"two plain iterations",
     2,
     false,
     {5.0F / 18.0F, 5.0F / 18.0F, 5.0F / 18.0F, 7.0F / 6.0F, 5.0F / 18.0F, 5.0F / 18.0F, 5.0F / 18.0F, 7.0F / 6.0F, 5.0F / 18.0F, 5.0F / 18.0F, 5.0F / 18.0F, 7.0F / 6.0F}},
    {"one Taubin iteration",
     1,
     true,
     {1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F, 11.0F / 6.0F, 1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F, 11.0F / 6.0F, 1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F, 11.0F / 6.0F}},
}};

/**
 * @brief Creates one small input array through the destination's storage resolver.
 * @tparam T Specifies the scalar type.
 * @tparam N Specifies the literal input value count.
 * @param dataStructure Owns the array and supplies storage policy.
 * @param path Names the new array.
 * @param parentId Identifies its existing parent.
 * @param tuples Supplies the tuple count.
 * @param components Supplies values per tuple.
 * @param values Supplies every initial value.
 * @return The inserted array.
 */
template <typename T, usize N>
DataArray<T>* CreateInputArray(DataStructure& dataStructure, const DataPath& path, DataObject::IdType parentId, usize tuples, usize components, const std::array<T, N>& values)
{
  REQUIRE(values.size() == tuples * components);
  auto store = DataStoreUtilities::CreateDataStore<T>(dataStructure, path, {tuples}, {components});
  REQUIRE(store != nullptr);
  auto* arrayPtr = DataArray<T>::Create(dataStructure, path.getTargetName(), store, parentId);
  REQUIRE(arrayPtr != nullptr);
  auto writeResult = store->copyFromBuffer(0, {values.data(), values.size()});
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  return arrayPtr;
}

/**
 * @brief Builds the same four-vertex graph from triangular faces or explicit edges.
 * @param dataStructure Receives the complete fixture.
 * @param kind Selects the connectivity representation.
 * @param nodeTypes Supplies the four vertex classifications.
 */
void BuildGraph(DataStructure& dataStructure, GeometryKind kind, const NodeTypes& nodeTypes)
{
  INodeGeometry1D* geometryPtr = nullptr;
  if(kind == GeometryKind::Triangle)
  {
    geometryPtr = TriangleGeom::Create(dataStructure, "Mesh");
  }
  else
  {
    geometryPtr = EdgeGeom::Create(dataStructure, "Mesh");
  }
  REQUIRE(geometryPtr != nullptr);
  auto* verticesPtr = CreateInputArray(dataStructure, k_VerticesPath, geometryPtr->getId(), 4, 3, k_InputCoordinates);
  geometryPtr->setVertices(*verticesPtr);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", {4}, geometryPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  geometryPtr->setVertexAttributeMatrix(*vertexDataPtr);
  CreateInputArray(dataStructure, k_NodeTypesPath, vertexDataPtr->getId(), 4, 1, nodeTypes);
  if(kind == GeometryKind::Triangle)
  {
    auto* facesPtr = CreateInputArray(dataStructure, k_ConnectivityPath, geometryPtr->getId(), 4, 3, k_Faces);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_GeometryPath));
    auto& triangle = dataStructure.getDataRefAs<TriangleGeom>(k_GeometryPath);
    triangle.setFaceList(*facesPtr);
    REQUIRE(triangle.getEdges() == nullptr);
  }
  else
  {
    auto* edgesPtr = CreateInputArray(dataStructure, k_ConnectivityPath, geometryPtr->getId(), 6, 2, k_Edges);
    geometryPtr->setEdgeList(*edgesPtr);
  }
}

/**
 * @brief Requires resident storage for each named smoothing input.
 * @param dataStructure Supplies the fixture before or after execution.
 */
void RequireResidentInputs(const DataStructure& dataStructure)
{
  for(const auto& path : {k_VerticesPath, k_NodeTypesPath, k_ConnectivityPath})
  {
    CAPTURE(path.toString());
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
    const auto& array = dataStructure.getDataRefAs<IDataArray>(path);
    REQUIRE(array.getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  }
}

/**
 * @brief Runs the public filter with distinct weights for all supported node classes.
 * @param scope Proves the selected algorithm and resident stores.
 * @param dataStructure Contains the mutable fixture.
 * @param iterations Supplies the number of smoothing iterations.
 * @param useTaubin Enables the second pass with mu factor -1.
 */
void ExecuteSmoothing(UnitTest::AlgorithmTestScope& scope, DataStructure& dataStructure, int32 iterations, bool useTaubin)
{
  LaplacianSmoothingFilter filter;
  Arguments args;
  args.insertOrAssign(LaplacianSmoothingFilter::k_IterationSteps_Key, std::make_any<int32>(iterations));
  args.insertOrAssign(LaplacianSmoothingFilter::k_Lambda_Key, std::make_any<float32>(0.25F));
  args.insertOrAssign(LaplacianSmoothingFilter::k_UseTaubinSmoothing_Key, std::make_any<bool>(useTaubin));
  args.insertOrAssign(LaplacianSmoothingFilter::k_MuFactor_Key, std::make_any<float32>(-1.0F));
  args.insertOrAssign(LaplacianSmoothingFilter::k_TripleLineLambda_Key, std::make_any<float32>(0.5F));
  args.insertOrAssign(LaplacianSmoothingFilter::k_QuadPointLambda_Key, std::make_any<float32>(0.75F));
  args.insertOrAssign(LaplacianSmoothingFilter::k_SurfacePointLambda_Key, std::make_any<float32>(0.125F));
  args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceTripleLineLambda_Key, std::make_any<float32>(0.375F));
  args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceQuadPointLambda_Key, std::make_any<float32>(0.625F));
  args.insertOrAssign(LaplacianSmoothingFilter::k_SurfaceMeshNodeTypeArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(LaplacianSmoothingFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(k_GeometryPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

/**
 * @brief Checks every output coordinate and the unchanged graph inputs.
 * @param dataStructure Supplies the completed fixture.
 * @param kind Selects the original connectivity and derived-edge cleanup contract.
 * @param nodeTypes Supplies the original classifications.
 * @param expected Supplies all twelve independently derived coordinates.
 * @param exact Requires exact coordinate equality for zero iterations.
 */
void CheckResult(const DataStructure& dataStructure, GeometryKind kind, const NodeTypes& nodeTypes, const Coordinates& expected, bool exact = false)
{
  RequireResidentInputs(dataStructure);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_VerticesPath));
  const auto& vertices = dataStructure.getDataRefAs<Float32Array>(k_VerticesPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(k_NodeTypesPath));
  const auto& nodes = dataStructure.getDataRefAs<Int8Array>(k_NodeTypesPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IGeometry::MeshIndexArrayType>(k_ConnectivityPath));
  const auto& connectivity = dataStructure.getDataRefAs<IGeometry::MeshIndexArrayType>(k_ConnectivityPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<INodeGeometry1D>(k_GeometryPath));
  const auto& geometry = dataStructure.getDataRefAs<INodeGeometry1D>(k_GeometryPath);
  CHECK(geometry.getVertices() == &vertices);
  REQUIRE(vertices.getTupleShape() == ShapeType{4});
  REQUIRE(vertices.getComponentShape() == ShapeType{3});
  REQUIRE(nodes.getTupleShape() == ShapeType{4});
  REQUIRE(nodes.getComponentShape() == ShapeType{1});
  for(usize vertexIdx = 0; vertexIdx < 4; ++vertexIdx)
  {
    CAPTURE(vertexIdx);
    CHECK(nodes.getValue(vertexIdx) == nodeTypes[vertexIdx]);
    for(usize compIdx = 0; compIdx < 3; ++compIdx)
    {
      const usize valueIdx = vertexIdx * 3 + compIdx;
      const float32 actual = vertices.getValue(valueIdx);
      CAPTURE(compIdx, actual, expected[valueIdx]);
      if(exact)
      {
        CHECK(actual == expected[valueIdx]);
      }
      else
      {
        CHECK(std::abs(actual - expected[valueIdx]) <= 1.0e-6F);
      }
    }
  }
  REQUIRE(connectivity.getTupleShape() == ShapeType{kind == GeometryKind::Triangle ? usize{4} : usize{6}});
  REQUIRE(connectivity.getComponentShape() == ShapeType{kind == GeometryKind::Triangle ? usize{3} : usize{2}});
  const auto& expectedConnectivity = kind == GeometryKind::Triangle ? k_Faces : k_Edges;
  for(usize valueIdx = 0; valueIdx < expectedConnectivity.size(); ++valueIdx)
  {
    CAPTURE(valueIdx);
    CHECK(connectivity.getValue(valueIdx) == expectedConnectivity[valueIdx]);
  }
  if(kind == GeometryKind::Triangle)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_GeometryPath));
    const auto& triangle = dataStructure.getDataRefAs<TriangleGeom>(k_GeometryPath);
    CHECK(triangle.getFaces() == &connectivity);
    CHECK(triangle.getEdges() == nullptr);
    CHECK(dataStructure.getData(k_GeometryPath.createChildPath(INodeGeometry1D::k_SharedEdgeListName)) == nullptr);
  }
  else
  {
    CHECK(geometry.getEdges() == &connectivity);
  }
}
} // namespace

TEST_CASE("SimplnxCore::LaplacianSmoothing: every node type uses its analytic lambda", "[SimplnxCore][LaplacianSmoothingFilter][LaplacianNumerical]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(Catch::Generators::from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  const auto testCase = GENERATE(Catch::Generators::from_range(k_NodeTypeCases));
  const auto kind = GENERATE(GeometryKind::Triangle, GeometryKind::Edge);
  DYNAMIC_SECTION(testCase.name << " on " << (kind == GeometryKind::Triangle ? "TriangleGeom" : "EdgeGeom"))
  {
    UnitTest::AlgorithmTestScope scope(scenario);
    const SmoothingBudgetScope budget;
    // Keep the original one-byte large-data threshold and force resident storage.
    const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
    DataStructure dataStructure;
    NodeTypes nodeTypes;
    nodeTypes.fill(testCase.nodeType);
    BuildGraph(dataStructure, kind, nodeTypes);
    RequireResidentInputs(dataStructure);
    ExecuteSmoothing(scope, dataStructure, 1, false);
    CheckResult(dataStructure, kind, nodeTypes, testCase.expected);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::LaplacianSmoothing: iterations and Taubin match analytic coordinates", "[SimplnxCore][LaplacianSmoothingFilter][LaplacianNumerical]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(Catch::Generators::from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  const auto testCase = GENERATE(Catch::Generators::from_range(k_IterationCases));
  const auto kind = GENERATE(GeometryKind::Triangle, GeometryKind::Edge);
  DYNAMIC_SECTION(testCase.name << " on " << (kind == GeometryKind::Triangle ? "TriangleGeom" : "EdgeGeom"))
  {
    UnitTest::AlgorithmTestScope scope(scenario);
    const SmoothingBudgetScope budget;
    // Keep the original one-byte large-data threshold and force resident storage.
    const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
    DataStructure dataStructure;
    const NodeTypes nodeTypes{NodeType::Default, NodeType::Default, NodeType::Default, NodeType::Default};
    BuildGraph(dataStructure, kind, nodeTypes);
    RequireResidentInputs(dataStructure);
    ExecuteSmoothing(scope, dataStructure, testCase.iterations, testCase.useTaubin);
    CheckResult(dataStructure, kind, nodeTypes, testCase.expected, testCase.iterations == 0);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::LaplacianSmoothing: mixed node classes use input-pass coordinates", "[SimplnxCore][LaplacianSmoothingFilter][LaplacianNumerical]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(Catch::Generators::from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  const auto kind = GENERATE(GeometryKind::Triangle, GeometryKind::Edge);
  DYNAMIC_SECTION((kind == GeometryKind::Triangle ? "TriangleGeom" : "EdgeGeom"))
  {
    UnitTest::AlgorithmTestScope scope(scenario);
    const SmoothingBudgetScope budget;
    // Keep the original one-byte large-data threshold and force resident storage.
    const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
    DataStructure dataStructure;
    const NodeTypes nodeTypes{NodeType::Unused, NodeType::Default, NodeType::TriplePoint, NodeType::Unused};
    const Coordinates expected{0.0F, 0.0F, 0.0F, 1.5F, 1.0F / 6.0F, 1.0F / 6.0F, 1.0F / 3.0F, 1.0F, 1.0F / 3.0F, 0.0F, 0.0F, 2.0F};
    BuildGraph(dataStructure, kind, nodeTypes);
    RequireResidentInputs(dataStructure);
    ExecuteSmoothing(scope, dataStructure, 1, false);
    CheckResult(dataStructure, kind, nodeTypes, expected);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
