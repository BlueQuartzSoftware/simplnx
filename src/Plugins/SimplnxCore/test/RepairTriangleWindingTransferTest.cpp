#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <vector>

using namespace nx::core;

namespace
{
using ConnectivityType = std::array<IGeometry::MeshIndexType, 12>;
using NeighborsType = DynamicListArray<uint16, IGeometry::MeshIndexType>;

// Each pair starts with the same directed shared edge. Reversing the second face resolves that conflict.
constexpr ConnectivityType k_InputTriangles{0, 1, 2, 0, 1, 3, 4, 5, 6, 4, 5, 7};
constexpr ConnectivityType k_RepairedTriangles{0, 1, 2, 3, 1, 0, 4, 5, 6, 7, 5, 4};
constexpr ConnectivityType k_PartialTriangles{0, 1, 2, 3, 1, 0, 4, 5, 6, 4, 5, 7};
constexpr std::array<IGeometry::MeshIndexType, 4> k_NeighborIds{1, 0, 3, 2};

enum class Transfer
{
  ReadTriangles,
  ReadIds,
  WriteTriangles,
  WriteIds
};

struct TransferCall
{
  Transfer operation;
  usize offset;
  usize values;
};

struct InjectedFailure
{
  Transfer operation;
  int32 code;
  const char* message;
};

constexpr InjectedFailure k_TriangleReadFailure{Transfer::ReadTriangles, -95251, "Injected triangle winding connectivity read refusal."};
constexpr InjectedFailure k_IdReadFailure{Transfer::ReadIds, -95252, "Injected triangle winding ID read refusal."};
constexpr InjectedFailure k_TriangleWriteFailure{Transfer::WriteTriangles, -95253, "Injected triangle winding connectivity write refusal."};

struct TransferTrace
{
  bool armed = false;
  const InjectedFailure* failure = nullptr;
  std::vector<TransferCall> calls;
  std::vector<Transfer> rejectedCalls;
};

/**
 * @class RecordingStore
 * @brief Records transfers on real resident storage and refuses selected calls before copying.
 * @tparam T Specifies the stored scalar type.
 * @note Refusal does not model rollback after partial backend I/O.
 */
template <typename T>
class RecordingStore final : public DataStore<T>
{
public:
  /**
   * @brief Creates a four-triangle input store and borrows its longer-lived trace.
   * @param components Specifies the values per triangle.
   * @param trace Records calls after setup and outlives the store.
   * @param readOperation Identifies this store's reads.
   * @param writeOperation Identifies this store's writes.
   */
  RecordingStore(usize components, TransferTrace& trace, Transfer readOperation, Transfer writeOperation)
  : DataStore<T>({4}, {components}, T{})
  , m_Trace(trace)
  , m_ReadOperation(readOperation)
  , m_WriteOperation(writeOperation)
  {
  }

  /**
   * @brief Records a read and refuses the selected call before changing the destination.
   * @param startIndex Specifies the first flat value index.
   * @param buffer Receives values only when the read succeeds.
   * @return The injected failure or the resident copy result.
   */
  [[nodiscard]] Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    if(m_Trace.armed)
    {
      m_Trace.calls.push_back({m_ReadOperation, startIndex, buffer.size()});
      if(m_Trace.failure != nullptr && m_Trace.failure->operation == m_ReadOperation)
      {
        m_Trace.rejectedCalls.push_back(m_ReadOperation);
        return MakeErrorResult(m_Trace.failure->code, m_Trace.failure->message);
      }
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  /**
   * @brief Saves the attempted payload and refuses the selected write before changing resident values.
   * @param startIndex Specifies the first flat value index.
   * @param buffer Supplies the computed values offered for publication.
   * @return The injected failure or the resident copy result.
   */
  [[nodiscard]] Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    if(m_Trace.armed)
    {
      m_Trace.calls.push_back({m_WriteOperation, startIndex, buffer.size()});
      attemptedWrite.assign(buffer.begin(), buffer.end());
      if(m_Trace.failure != nullptr && m_Trace.failure->operation == m_WriteOperation)
      {
        m_Trace.rejectedCalls.push_back(m_WriteOperation);
        return MakeErrorResult(m_Trace.failure->code, m_Trace.failure->message);
      }
    }
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  // The payload survives a refused write so the test can inspect the computed connectivity.
  std::vector<T> attemptedWrite;

private:
  TransferTrace& m_Trace;
  const Transfer m_ReadOperation;
  const Transfer m_WriteOperation;
};

struct WindingCase
{
  const char* name;
  const InjectedFailure* failure;
  bool preCancelled;
  bool cancelAfterFeature;
  usize expectedCalls;
  const ConnectivityType* expectedStoredTriangles;
  const ConnectivityType* expectedWrittenTriangles;
};

constexpr std::array<WindingCase, 4> k_TransferCases{{
    {"successful control", nullptr, false, false, 3, &k_RepairedTriangles, &k_RepairedTriangles},
    {"connectivity read refusal", &k_TriangleReadFailure, false, false, 1, &k_InputTriangles, nullptr},
    {"ID read refusal", &k_IdReadFailure, false, false, 2, &k_InputTriangles, nullptr},
    {"connectivity write refusal", &k_TriangleWriteFailure, false, false, 3, &k_InputTriangles, &k_RepairedTriangles},
}};

constexpr std::array<WindingCase, 3> k_CancellationCases{{
    {"pre-cancelled control", nullptr, true, false, 3, &k_InputTriangles, &k_InputTriangles},
    {"cancellation after feature 1", nullptr, false, true, 3, &k_PartialTriangles, &k_PartialTriangles},
    {"write refusal after feature 1 cancellation", &k_TriangleWriteFailure, false, true, 3, &k_InputTriangles, &k_PartialTriangles},
}};

/**
 * @brief Runs one direct utility call with valid topology and independently checks transfers and values.
 * @param useFaceLabels Selects two-component face labels instead of one-component regions.
 * @param windingCase Supplies independent transfer, result, and connectivity expectations.
 */
void CheckWindingCase(bool useFaceLabels, const WindingCase& windingCase)
{
  // The trace and DataStructure outlive the stores and neighbor reference used by the utility.
  TransferTrace trace;
  DataStructure dataStructure;
  RecordingStore<IGeometry::MeshIndexType> triangles(3, trace, Transfer::ReadTriangles, Transfer::WriteTriangles);
  RecordingStore<int32> ids(useFaceLabels ? 2 : 1, trace, Transfer::ReadIds, Transfer::WriteIds);
  const std::vector<int32> inputIds = useFaceLabels ? std::vector<int32>{0, 1, 0, 1, 0, 2, 0, 2} : std::vector<int32>{1, 1, 2, 2};
  auto* neighborsPtr = NeighborsType::Create(dataStructure, "Neighbors", {});
  REQUIRE(neighborsPtr != nullptr);
  neighborsPtr->allocateLists(std::array<uint16, 4>{1, 1, 1, 1});
  for(usize triangleIdx = 0; triangleIdx < k_NeighborIds.size(); ++triangleIdx)
  {
    neighborsPtr->insertCellReference(triangleIdx, 0, k_NeighborIds[triangleIdx]);
  }

  const auto triangleSeedResult = triangles.copyFromBuffer(0, {k_InputTriangles.data(), k_InputTriangles.size()});
  const auto idSeedResult = ids.copyFromBuffer(0, {inputIds.data(), inputIds.size()});
  REQUIRE(triangleSeedResult.valid());
  REQUIRE(idSeedResult.valid());
  REQUIRE(triangles.getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(ids.getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(trace.calls.empty());
  REQUIRE(trace.rejectedCalls.empty());
  REQUIRE(triangles.attemptedWrite.empty());
  REQUIRE(ids.attemptedWrite.empty());

  std::atomic_bool shouldCancel = windingCase.preCancelled;
  usize cancellationWitnesses = 0;
  const IFilter::MessageHandler handler{IFilter::MessageHandler::Callback{[&](const IFilter::Message& message) {
    // reset() opens the first progress gate; this synchronous callback stops traversal before feature 2.
    if(windingCase.cancelAfterFeature && message.type == IFilter::Message::Type::Progress && message.progress == 50 && message.message.rfind("Repairing Feature Windings", 0) == 0)
    {
      ++cancellationWitnesses;
      shouldCancel.store(true);
    }
  }}};

  trace.failure = windingCase.failure;
  trace.armed = true;
  const auto result = MeshingUtilities::RepairTriangleWinding(triangles, *neighborsPtr, ids, shouldCancel, handler);
  trace.armed = false;

  REQUIRE(result.warnings().empty());
  if(windingCase.failure == nullptr)
  {
    REQUIRE(result.valid());
    REQUIRE(trace.rejectedCalls.empty());
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == windingCase.failure->code);
    REQUIRE(result.errors()[0].message == windingCase.failure->message);
    REQUIRE(trace.rejectedCalls.size() == 1);
    REQUIRE(trace.rejectedCalls[0] == windingCase.failure->operation);
  }

  const std::array<TransferCall, 3> expectedTransfers{{{Transfer::ReadTriangles, 0, 12}, {Transfer::ReadIds, 0, inputIds.size()}, {Transfer::WriteTriangles, 0, 12}}};
  REQUIRE(trace.calls.size() == windingCase.expectedCalls);
  REQUIRE(windingCase.expectedCalls <= expectedTransfers.size());
  for(usize callIdx = 0; callIdx < windingCase.expectedCalls; ++callIdx)
  {
    CAPTURE(callIdx);
    REQUIRE(trace.calls[callIdx].operation == expectedTransfers[callIdx].operation);
    REQUIRE(trace.calls[callIdx].offset == expectedTransfers[callIdx].offset);
    REQUIRE(trace.calls[callIdx].values == expectedTransfers[callIdx].values);
  }
  REQUIRE(cancellationWitnesses == (windingCase.cancelAfterFeature ? 1 : 0));
  REQUIRE(shouldCancel.load() == (windingCase.preCancelled || windingCase.cancelAfterFeature));

  REQUIRE(triangles.getSize() == windingCase.expectedStoredTriangles->size());
  for(usize valueIdx = 0; valueIdx < triangles.getSize(); ++valueIdx)
  {
    CAPTURE(valueIdx);
    REQUIRE(triangles[valueIdx] == (*windingCase.expectedStoredTriangles)[valueIdx]);
  }
  if(windingCase.expectedWrittenTriangles == nullptr)
  {
    REQUIRE(triangles.attemptedWrite.empty());
  }
  else
  {
    REQUIRE(triangles.attemptedWrite.size() == windingCase.expectedWrittenTriangles->size());
    for(usize valueIdx = 0; valueIdx < triangles.attemptedWrite.size(); ++valueIdx)
    {
      CAPTURE(valueIdx);
      REQUIRE(triangles.attemptedWrite[valueIdx] == (*windingCase.expectedWrittenTriangles)[valueIdx]);
    }
  }
  REQUIRE(ids.attemptedWrite.empty());
  REQUIRE(ids.getSize() == inputIds.size());
  for(usize valueIdx = 0; valueIdx < inputIds.size(); ++valueIdx)
  {
    CAPTURE(valueIdx);
    REQUIRE(ids[valueIdx] == inputIds[valueIdx]);
  }
  for(usize triangleIdx = 0; triangleIdx < k_NeighborIds.size(); ++triangleIdx)
  {
    CAPTURE(triangleIdx);
    REQUIRE(neighborsPtr->getNumberOfElements(triangleIdx) == 1);
    const auto* neighborListPtr = neighborsPtr->getElementListPointer(triangleIdx);
    REQUIRE(neighborListPtr != nullptr);
    REQUIRE(neighborListPtr[0] == k_NeighborIds[triangleIdx]);
  }
}
} // namespace

TEST_CASE("SimplnxCore::RepairTriangleWinding: buffered transfer results", "[SimplnxCore][RepairTriangleWinding][ooc-review-winding-transfer]")
{
  const bool useFaceLabels = GENERATE(false, true);
  const auto windingCase = GENERATE(from_range(k_TransferCases));
  DYNAMIC_SECTION((useFaceLabels ? "face labels" : "regions") << ": " << windingCase.name)
  {
    CheckWindingCase(useFaceLabels, windingCase);
  }
}

TEST_CASE("SimplnxCore::RepairTriangleWinding: buffered cancellation publication", "[SimplnxCore][RepairTriangleWinding][ooc-review-winding-transfer]")
{
  const bool useFaceLabels = GENERATE(false, true);
  const auto windingCase = GENERATE(from_range(k_CancellationCases));
  DYNAMIC_SECTION((useFaceLabels ? "face labels" : "regions") << ": " << windingCase.name)
  {
    CheckWindingCase(useFaceLabels, windingCase);
  }
}
