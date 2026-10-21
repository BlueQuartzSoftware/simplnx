#include "SimplnxCore/Filters/Algorithms/LaplacianSmoothing.hpp"
#include "SimplnxCore/Filters/LaplacianSmoothingFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
/** @brief Stores the complete four-vertex coordinate oracle. */
using Coordinates = std::array<float32, 12>;
/** @brief Identifies the geometry changed by the public filter. */
const DataPath k_GeometryPath{{"Mesh"}};
/** @brief Identifies the coordinate payload supplied to writeback. */
const DataPath k_VerticesPath{{"Mesh", "Vertices"}};
/** @brief Identifies the four unchanged node classifications. */
const DataPath k_NodeTypesPath{{"Mesh", "Vertex Data", "Node Types"}};
/** @brief Identifies the six supplied edges retained by EdgeGeom. */
const DataPath k_EdgesPath{{"Mesh", "Edges"}};
/** @brief Supplies a tetrahedron with centroid (1/2, 1/2, 1/2). */
constexpr Coordinates k_InputCoordinates{0.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F, 0.0F, 2.0F};
/** @brief Supplies the one-pass result for lambda 1/4 on the complete four-vertex graph. */
constexpr Coordinates k_PlainCoordinates{1.0F / 6.0F, 1.0F / 6.0F, 1.0F / 6.0F, 1.5F, 1.0F / 6.0F, 1.0F / 6.0F, 1.0F / 6.0F, 1.5F, 1.0F / 6.0F, 1.0F / 6.0F, 1.0F / 6.0F, 1.5F};
/** @brief Supplies one Taubin pair with lambda 1/4 and mu factor -1. */
constexpr Coordinates k_TaubinCoordinates{1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F, 11.0F / 6.0F, 1.0F / 18.0F, 1.0F / 18.0F,
                                          1.0F / 18.0F, 11.0F / 6.0F, 1.0F / 18.0F, 1.0F / 18.0F, 1.0F / 18.0F, 11.0F / 6.0F};
/** @brief Supplies every edge once; each vertex has degree three. */
constexpr std::array<IGeometry::MeshIndexType, 12> k_Edges{0, 1, 0, 2, 0, 3, 1, 2, 1, 3, 2, 3};
/** @brief Makes every vertex use the default lambda. */
constexpr std::array<int8, 4> k_NodeTypes{NodeType::Default, NodeType::Default, NodeType::Default, NodeType::Default};
/** @brief Distinguishes the injected store failure from the algorithm's contextual error. */
constexpr int32 k_InjectedError = -95241;
/** @brief Supplies the exact backend diagnostic that the filter must preserve. */
constexpr const char* k_InjectedMessage = "Injected Laplacian smoothing bulk transfer failure.";

/**
 * @enum Transfer
 * @brief Identifies reads and writes on the three resident fixture stores.
 */
enum class Transfer
{
  ReadNodes,     ///< Reads vertex classifications.
  ReadVertices,  ///< Reads the original coordinates.
  ReadEdges,     ///< Reads the supplied connectivity.
  WriteVertices, ///< Publishes completed coordinate passes.
  WriteNodes,    ///< Detects an unexpected classification write.
  WriteEdges     ///< Detects an unexpected connectivity write.
};

/**
 * @struct TransferCall
 * @brief Records the actual operation and flat value range requested by smoothing.
 */
struct TransferCall
{
  Transfer operation;
  usize offset;
  usize values;
};

/** @brief Specifies the complete expected bulk order; failures permit only its corresponding prefix. */
constexpr std::array<TransferCall, 4> k_CompleteTransfers{{{Transfer::ReadNodes, 0, 4}, {Transfer::ReadVertices, 0, 12}, {Transfer::ReadEdges, 0, 12}, {Transfer::WriteVertices, 0, 12}}};

/**
 * @struct FailureCase
 * @brief Pairs a refused transfer with its contextual error and allowed call prefix.
 */
struct FailureCase
{
  const char* name;
  Transfer operation;
  int32 contextCode;
  usize expectedCalls;
};

/** @brief Supplies independent stopping-order expectations for each bulk failure. */
constexpr std::array<FailureCase, 4> k_FailureCases{
    {{"node read", Transfer::ReadNodes, -560, 1}, {"vertex read", Transfer::ReadVertices, -561, 2}, {"edge read", Transfer::ReadEdges, -562, 3}, {"vertex write", Transfer::WriteVertices, -563, 4}}};

/**
 * @struct TransferTrace
 * @brief Controls one synchronous fault and records calls only after fixture setup.
 */
struct TransferTrace
{
  bool armed = false;
  std::optional<Transfer> failure;
  std::vector<TransferCall> calls;
};

/**
 * @class RecordingStore
 * @brief Uses real resident storage while recording and optionally refusing one bulk boundary.
 * @tparam T Specifies the stored scalar type.
 * @note Refusal happens before the underlying copy. It does not model rollback after partial backend I/O.
 */
template <typename T>
class RecordingStore final : public DataStore<T>
{
public:
  /**
   * @brief Creates only the small resident fixture and borrows its longer-lived trace.
   * @param tuples Supplies the fixture tuple count.
   * @param components Supplies the values per tuple.
   * @param trace Records calls after arming and outlives the store.
   * @param readOperation Identifies reads from this input.
   * @param writeOperation Identifies writes to this input.
   */
  RecordingStore(usize tuples, usize components, TransferTrace& trace, Transfer readOperation, Transfer writeOperation)
  : DataStore<T>({tuples}, {components}, T{})
  , m_Trace(trace)
  , m_ReadOperation(readOperation)
  , m_WriteOperation(writeOperation)
  {
  }

  /**
   * @brief Records a bulk read and optionally returns the injected error before copying.
   * @param startIndex Supplies the requested flat offset.
   * @param buffer Receives values only when the read succeeds.
   * @return The injected failure or the real resident copy result.
   */
  [[nodiscard]] Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    if(m_Trace.armed)
    {
      m_Trace.calls.push_back({m_ReadOperation, startIndex, buffer.size()});
      if(m_Trace.failure == m_ReadOperation)
      {
        return MakeErrorResult(k_InjectedError, k_InjectedMessage);
      }
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  /**
   * @brief Retains the attempted payload and optionally fails before resident values change.
   * @param startIndex Supplies the requested flat offset.
   * @param buffer Supplies the computed values offered to writeback.
   * @return The injected failure or the real resident copy result.
   */
  [[nodiscard]] Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    if(m_Trace.armed)
    {
      m_Trace.calls.push_back({m_WriteOperation, startIndex, buffer.size()});
      attemptedWrite.assign(buffer.begin(), buffer.end());
      if(m_Trace.failure == m_WriteOperation)
      {
        return MakeErrorResult(k_InjectedError, k_InjectedMessage);
      }
    }
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  /** @brief Owns a copy of the last attempted write payload, including a refused write. */
  std::vector<T> attemptedWrite;

private:
  TransferTrace& m_Trace;
  const Transfer m_ReadOperation;
  const Transfer m_WriteOperation;
};

/**
 * @struct InputStores
 * @brief Retains the actual stores whose attachments and values the tests verify.
 */
struct InputStores
{
  std::shared_ptr<RecordingStore<float32>> vertices;
  std::shared_ptr<RecordingStore<int8>> nodes;
  std::shared_ptr<RecordingStore<IGeometry::MeshIndexType>> edges;
};

/**
 * @brief Builds a small EdgeGeom without incidental edge-generation reads.
 * @param dataStructure Owns all fixture objects.
 * @param trace Outlives the DataStructure and returned store references.
 * @return References to the three resident fault/recording stores.
 */
InputStores BuildGraph(DataStructure& dataStructure, TransferTrace& trace)
{
  auto* geometryPtr = EdgeGeom::Create(dataStructure, k_GeometryPath.getTargetName());
  REQUIRE(geometryPtr != nullptr);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", {4}, geometryPtr->getId());
  REQUIRE(vertexDataPtr != nullptr);
  geometryPtr->setVertexAttributeMatrix(*vertexDataPtr);
  InputStores stores{std::make_shared<RecordingStore<float32>>(4, 3, trace, Transfer::ReadVertices, Transfer::WriteVertices),
                     std::make_shared<RecordingStore<int8>>(4, 1, trace, Transfer::ReadNodes, Transfer::WriteNodes),
                     std::make_shared<RecordingStore<IGeometry::MeshIndexType>>(6, 2, trace, Transfer::ReadEdges, Transfer::WriteEdges)};
  auto* verticesPtr = Float32Array::Create(dataStructure, k_VerticesPath.getTargetName(), stores.vertices, geometryPtr->getId());
  auto* nodesPtr = Int8Array::Create(dataStructure, k_NodeTypesPath.getTargetName(), stores.nodes, vertexDataPtr->getId());
  auto* edgesPtr = IGeometry::MeshIndexArrayType::Create(dataStructure, k_EdgesPath.getTargetName(), stores.edges, geometryPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(nodesPtr != nullptr);
  REQUIRE(edgesPtr != nullptr);
  geometryPtr->setVertices(*verticesPtr);
  geometryPtr->setEdgeList(*edgesPtr);
  const auto vertexResult = stores.vertices->copyFromBuffer(0, {k_InputCoordinates.data(), k_InputCoordinates.size()});
  const auto nodeResult = stores.nodes->copyFromBuffer(0, {k_NodeTypes.data(), k_NodeTypes.size()});
  const auto edgeResult = stores.edges->copyFromBuffer(0, {k_Edges.data(), k_Edges.size()});
  SIMPLNX_RESULT_REQUIRE_VALID(vertexResult);
  SIMPLNX_RESULT_REQUIRE_VALID(nodeResult);
  SIMPLNX_RESULT_REQUIRE_VALID(edgeResult);
  REQUIRE(trace.calls.empty());
  return stores;
}

/**
 * @brief Supplies the public filter arguments with distinct weights for every node class.
 * @param iterations Sets the requested outer pass count.
 * @param useTaubin Enables the second pass with mu factor -1.
 * @return Complete public filter arguments.
 */
Arguments MakeArguments(int32 iterations, bool useTaubin)
{
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
  return args;
}

/**
 * @brief Compares every actual call with the allowed prefix of the independent transfer sequence.
 * @param trace Supplies calls made after arming.
 * @param expectedCalls Supplies the prefix length, including the failed call when applicable.
 */
void CheckTransfers(const TransferTrace& trace, usize expectedCalls)
{
  REQUIRE(trace.calls.size() == expectedCalls);
  REQUIRE(expectedCalls <= k_CompleteTransfers.size());
  for(usize callIdx = 0; callIdx < expectedCalls; ++callIdx)
  {
    CAPTURE(callIdx);
    CHECK(trace.calls[callIdx].operation == k_CompleteTransfers[callIdx].operation);
    CHECK(trace.calls[callIdx].offset == k_CompleteTransfers[callIdx].offset);
    CHECK(trace.calls[callIdx].values == k_CompleteTransfers[callIdx].values);
  }
}

/**
 * @brief Checks the complete payload offered to the coordinate write.
 * @param actual Supplies the captured write buffer, including a refused write.
 * @param expected Supplies the independently derived completed-pass coordinates.
 */
void CheckPayload(const std::vector<float32>& actual, const Coordinates& expected)
{
  REQUIRE(actual.size() == expected.size());
  for(usize valueIdx = 0; valueIdx < expected.size(); ++valueIdx)
  {
    CAPTURE(valueIdx, actual[valueIdx], expected[valueIdx]);
    CHECK(std::abs(actual[valueIdx] - expected[valueIdx]) <= 1.0e-6F);
  }
}

/**
 * @brief Verifies the stored coordinates, unchanged inputs, and original resident attachments.
 * @param dataStructure Supplies the fixture before or after execution.
 * @param stores Supplies the original three store objects.
 * @param expected Supplies every expected coordinate.
 * @param exact Requires exact coordinate equality when no successful write is permitted.
 */
void CheckGraph(const DataStructure& dataStructure, const InputStores& stores, const Coordinates& expected, bool exact)
{
  const EdgeGeom* geometryPtr = nullptr;
  REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<EdgeGeom>(k_GeometryPath));
  const Float32Array* verticesPtr = nullptr;
  REQUIRE_NOTHROW(verticesPtr = &dataStructure.getDataRefAs<Float32Array>(k_VerticesPath));
  const Int8Array* nodesPtr = nullptr;
  REQUIRE_NOTHROW(nodesPtr = &dataStructure.getDataRefAs<Int8Array>(k_NodeTypesPath));
  const IGeometry::MeshIndexArrayType* edgesPtr = nullptr;
  REQUIRE_NOTHROW(edgesPtr = &dataStructure.getDataRefAs<IGeometry::MeshIndexArrayType>(k_EdgesPath));
  CHECK(geometryPtr->getVertices() == verticesPtr);
  CHECK(geometryPtr->getEdges() == edgesPtr);
  CHECK(&verticesPtr->getDataStoreRef() == stores.vertices.get());
  CHECK(&nodesPtr->getDataStoreRef() == stores.nodes.get());
  CHECK(&edgesPtr->getDataStoreRef() == stores.edges.get());
  for(const auto& path : {k_VerticesPath, k_NodeTypesPath, k_EdgesPath})
  {
    const IDataArray* arrayPtr = nullptr;
    REQUIRE_NOTHROW(arrayPtr = &dataStructure.getDataRefAs<IDataArray>(path));
    REQUIRE(arrayPtr->getIDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
  }
  REQUIRE(verticesPtr->getTupleShape() == ShapeType{4});
  REQUIRE(verticesPtr->getComponentShape() == ShapeType{3});
  REQUIRE(nodesPtr->getTupleShape() == ShapeType{4});
  REQUIRE(nodesPtr->getComponentShape() == ShapeType{1});
  REQUIRE(edgesPtr->getTupleShape() == ShapeType{6});
  REQUIRE(edgesPtr->getComponentShape() == ShapeType{2});
  for(usize valueIdx = 0; valueIdx < expected.size(); ++valueIdx)
  {
    const float32 actual = verticesPtr->getValue(valueIdx);
    CAPTURE(valueIdx, actual, expected[valueIdx]);
    if(exact)
    {
      CHECK(actual == expected[valueIdx]);
    }
    else
    {
      CHECK(std::abs(actual - expected[valueIdx]) <= 1.0e-6F);
    }
    CHECK(edgesPtr->getValue(valueIdx) == k_Edges[valueIdx]);
  }
  for(usize vertexIdx = 0; vertexIdx < k_NodeTypes.size(); ++vertexIdx)
  {
    CAPTURE(vertexIdx);
    CHECK(nodesPtr->getValue(vertexIdx) == k_NodeTypes[vertexIdx]);
  }
  CHECK(stores.nodes->attemptedWrite.empty());
  CHECK(stores.edges->attemptedWrite.empty());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::LaplacianSmoothing: bulk failures stop at the failed transfer", "[SimplnxCore][LaplacianSmoothingFilter][LaplacianTransfer]")
{
  UnitTest::LoadPlugins();
  const auto failure = GENERATE(Catch::Generators::from_range(k_FailureCases));
  CAPTURE(failure.name);
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  TransferTrace trace;
  DataStructure dataStructure;
  const auto stores = BuildGraph(dataStructure, trace);
  CheckGraph(dataStructure, stores, k_InputCoordinates, true);
  LaplacianSmoothingFilter filter;
  const auto args = MakeArguments(1, false);
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  trace.failure = failure.operation;
  trace.armed = true;
  const auto result = filter.execute(dataStructure, args).result;
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 2);
  CHECK(result.errors()[0].code == k_InjectedError);
  CHECK(result.errors()[0].message == k_InjectedMessage);
  CHECK(result.warnings().empty());
  CHECK(result.errors()[1].code == failure.contextCode);
  CHECK(result.errors()[1].message.find(failure.operation == Transfer::ReadNodes ? k_NodeTypesPath.toString() : k_GeometryPath.toString()) != std::string::npos);
  CheckTransfers(trace, failure.expectedCalls);
  if(failure.operation == Transfer::WriteVertices)
  {
    CheckPayload(stores.vertices->attemptedWrite, k_PlainCoordinates);
  }
  else
  {
    CHECK(stores.vertices->attemptedWrite.empty());
  }
  CheckGraph(dataStructure, stores, k_InputCoordinates, true);
}

TEST_CASE("SimplnxCore::LaplacianSmoothing: cancellation publishes completed passes and retains write failures", "[SimplnxCore][LaplacianSmoothingFilter][LaplacianCancellation]")
{
  UnitTest::LoadPlugins();
  const std::string schedule = GENERATE("after plain pass", "before Taubin second pass", "after Taubin pair");
  const bool failWrite = GENERATE(false, true);
  CAPTURE(schedule, failWrite);
  const bool useTaubin = schedule != "after plain pass";
  const auto& expected = schedule == "after Taubin pair" ? k_TaubinCoordinates : k_PlainCoordinates;
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  TransferTrace trace;
  DataStructure dataStructure;
  const auto stores = BuildGraph(dataStructure, trace);
  CheckGraph(dataStructure, stores, k_InputCoordinates, true);
  LaplacianSmoothingFilter filter;
  const auto args = MakeArguments(2, useTaubin);
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  std::atomic_bool shouldCancel = false;
  std::vector<std::string> iterations;
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Progress && message.message.starts_with("Smoothing Iterations: "))
    {
      iterations.push_back(message.message);
      const bool atCheckpoint = schedule == "before Taubin second pass" ? message.message.find("Starting Taubin pass") != std::string::npos : message.message.starts_with("Smoothing Iterations: 1/2");
      if(atCheckpoint)
      {
        shouldCancel.store(true);
      }
    }
  }};
  if(failWrite)
  {
    trace.failure = Transfer::WriteVertices;
  }
  trace.armed = true;
  const auto result = filter.execute(dataStructure, args, nullptr, handler, shouldCancel).result;
  REQUIRE(shouldCancel.load());
  REQUIRE(result.invalid());
  if(failWrite)
  {
    REQUIRE(result.errors().size() == 2);
    CHECK(result.errors()[0].code == k_InjectedError);
    CHECK(result.errors()[0].message == k_InjectedMessage);
    CHECK(result.errors()[1].code == -563);
    CHECK(result.errors()[1].message.find(k_GeometryPath.toString()) != std::string::npos);
  }
  else
  {
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -1);
  }
  CHECK(result.warnings().empty());
  REQUIRE(iterations.size() == (schedule == "after Taubin pair" ? 3 : 2));
  CHECK(iterations.front() == "Smoothing Iterations: 0/2");
  if(schedule == "before Taubin second pass")
  {
    CHECK(iterations.back().starts_with("Smoothing Iterations: 0/2"));
    CHECK(iterations.back().find("Starting Taubin pass") != std::string::npos);
  }
  else
  {
    CHECK(iterations.back().starts_with("Smoothing Iterations: 1/2"));
  }
  CheckTransfers(trace, k_CompleteTransfers.size());
  CheckPayload(stores.vertices->attemptedWrite, expected);
  CheckGraph(dataStructure, stores, failWrite ? k_InputCoordinates : expected, failWrite);
}

TEST_CASE("SimplnxCore::LaplacianSmoothing: zero iterations and pre-cancellation do not write coordinates", "[SimplnxCore][LaplacianSmoothingFilter][LaplacianTransfer]")
{
  UnitTest::LoadPlugins();
  const std::string scenario = GENERATE("zero plain iterations", "zero Taubin iterations", "pre-cancelled");
  CAPTURE(scenario);
  const bool preCancelled = scenario == "pre-cancelled";
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  TransferTrace trace;
  DataStructure dataStructure;
  const auto stores = BuildGraph(dataStructure, trace);
  CheckGraph(dataStructure, stores, k_InputCoordinates, true);
  LaplacianSmoothingFilter filter;
  const auto args = MakeArguments(preCancelled ? 2 : 0, scenario == "zero Taubin iterations");
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const std::atomic_bool shouldCancel = preCancelled;
  trace.armed = true;
  const auto result = filter.execute(dataStructure, args, nullptr, {}, shouldCancel).result;
  if(preCancelled)
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -1);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  CHECK(result.warnings().empty());
  if(preCancelled)
  {
    CheckTransfers(trace, 0);
  }
  else
  {
    // A zero-iteration implementation may skip the reads; it must never publish a coordinate write.
    for(const auto& call : trace.calls)
    {
      CHECK(call.operation != Transfer::WriteVertices);
      CHECK(call.operation != Transfer::WriteNodes);
      CHECK(call.operation != Transfer::WriteEdges);
    }
  }
  CHECK(stores.vertices->attemptedWrite.empty());
  CheckGraph(dataStructure, stores, k_InputCoordinates, true);
}

TEST_CASE("SimplnxCore::LaplacianSmoothing: missing vertices fail the direct algorithm guard", "[SimplnxCore][LaplacianSmoothingFilter][LaplacianTransfer]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  DataStructure dataStructure;
  auto* geometryPtr = EdgeGeom::Create(dataStructure, k_GeometryPath.getTargetName());
  REQUIRE(geometryPtr != nullptr);
  REQUIRE(geometryPtr->getVertices() == nullptr);
  LaplacianSmoothingInputValues values{};
  values.pTriangleGeometryDataPath = k_GeometryPath;
  values.pSurfaceMeshNodeTypeArrayPath = k_NodeTypesPath;
  const std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler handler{};
  Result<> result;
  // Public parameter validation rejects the absent node array before this distinct algorithm guard can run.
  REQUIRE_NOTHROW(result = LaplacianSmoothing(dataStructure, &values, shouldCancel, handler)());
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -559);
  CHECK(result.errors()[0].message.find(k_GeometryPath.toString()) != std::string::npos);
  CHECK(result.warnings().empty());
  CHECK(geometryPtr->getVertices() == nullptr);
  CHECK(geometryPtr->getEdges() == nullptr);
  CHECK(dataStructure.getData(k_VerticesPath) == nullptr);
  CHECK(dataStructure.getData(k_NodeTypesPath) == nullptr);
  CHECK(dataStructure.getData(k_EdgesPath) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
