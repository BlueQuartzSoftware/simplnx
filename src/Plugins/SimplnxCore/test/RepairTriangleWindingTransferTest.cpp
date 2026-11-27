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

constexpr InjectedFailure k_TriangleReadFailure{.operation = Transfer::ReadTriangles, .code = -95251, .message = "Injected triangle winding connectivity read refusal."};
constexpr InjectedFailure k_IdReadFailure{.operation = Transfer::ReadIds, .code = -95252, .message = "Injected triangle winding ID read refusal."};
constexpr InjectedFailure k_TriangleWriteFailure{.operation = Transfer::WriteTriangles, .code = -95253, .message = "Injected triangle winding connectivity write refusal."};

struct TransferTrace
{
  bool armed = false;
  const InjectedFailure* failure = nullptr;
  std::vector<TransferCall> calls;
  std::vector<Transfer> rejected_calls;
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

  ~RecordingStore() override = default;
  RecordingStore(const RecordingStore&) = default;
  RecordingStore(RecordingStore&&) = default;
  RecordingStore& operator=(const RecordingStore&) = delete;
  RecordingStore& operator=(RecordingStore&&) = delete;

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
        m_Trace.rejected_calls.push_back(m_ReadOperation);
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
      attempted_write.assign(buffer.begin(), buffer.end());
      if(m_Trace.failure != nullptr && m_Trace.failure->operation == m_WriteOperation)
      {
        m_Trace.rejected_calls.push_back(m_WriteOperation);
        return MakeErrorResult(m_Trace.failure->code, m_Trace.failure->message);
      }
    }
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  // The payload survives a refused write so the test can inspect the computed connectivity.
  std::vector<T> attempted_write;

private:
  TransferTrace& m_Trace;
  const Transfer m_ReadOperation;
  const Transfer m_WriteOperation;
};

struct WindingCase
{
  const char* name;
  const InjectedFailure* failure;
  bool pre_cancelled;
  bool cancel_after_feature;
  usize expected_calls;
  const ConnectivityType* expected_stored_triangles;
  const ConnectivityType* expected_written_triangles;
};

constexpr std::array<WindingCase, 4> k_TransferCases{{
    {.name = "successful control",
     .failure = nullptr,
     .pre_cancelled = false,
     .cancel_after_feature = false,
     .expected_calls = 3,
     .expected_stored_triangles = &k_RepairedTriangles,
     .expected_written_triangles = &k_RepairedTriangles},
    {.name = "connectivity read refusal",
     .failure = &k_TriangleReadFailure,
     .pre_cancelled = false,
     .cancel_after_feature = false,
     .expected_calls = 1,
     .expected_stored_triangles = &k_InputTriangles,
     .expected_written_triangles = nullptr},
    {.name = "ID read refusal",
     .failure = &k_IdReadFailure,
     .pre_cancelled = false,
     .cancel_after_feature = false,
     .expected_calls = 2,
     .expected_stored_triangles = &k_InputTriangles,
     .expected_written_triangles = nullptr},
    {.name = "connectivity write refusal",
     .failure = &k_TriangleWriteFailure,
     .pre_cancelled = false,
     .cancel_after_feature = false,
     .expected_calls = 3,
     .expected_stored_triangles = &k_InputTriangles,
     .expected_written_triangles = &k_RepairedTriangles},
}};

constexpr std::array<WindingCase, 3> k_CancellationCases{{
    {.name = "pre-cancelled control",
     .failure = nullptr,
     .pre_cancelled = true,
     .cancel_after_feature = false,
     .expected_calls = 3,
     .expected_stored_triangles = &k_InputTriangles,
     .expected_written_triangles = &k_InputTriangles},
    {.name = "cancellation after feature 1",
     .failure = nullptr,
     .pre_cancelled = false,
     .cancel_after_feature = true,
     .expected_calls = 3,
     .expected_stored_triangles = &k_PartialTriangles,
     .expected_written_triangles = &k_PartialTriangles},
    {.name = "write refusal after feature 1 cancellation",
     .failure = &k_TriangleWriteFailure,
     .pre_cancelled = false,
     .cancel_after_feature = true,
     .expected_calls = 3,
     .expected_stored_triangles = &k_InputTriangles,
     .expected_written_triangles = &k_PartialTriangles},
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
  REQUIRE(trace.rejected_calls.empty());
  REQUIRE(triangles.attempted_write.empty());
  REQUIRE(ids.attempted_write.empty());

  std::atomic_bool shouldCancel = windingCase.pre_cancelled;
  usize cancellationWitnesses = 0;
  const IFilter::MessageHandler handler{IFilter::MessageHandler::CallbackType{[&](const IFilter::Message& message) {
    // reset() opens the first progress gate; this synchronous callback stops traversal before feature 2.
    if(windingCase.cancel_after_feature && message.type == IFilter::Message::Type::Progress && message.progress == 50 && message.message.starts_with("Repairing Feature Windings"))
    {
      ++cancellationWitnesses;
      shouldCancel.store(true);
    }
  }}};

  trace.failure = windingCase.failure;
  trace.armed = true;
  const auto result = meshing_utilities::RepairTriangleWinding(triangles, *neighborsPtr, ids, shouldCancel, handler);
  trace.armed = false;

  REQUIRE(result.warnings().empty());
  if(windingCase.failure == nullptr)
  {
    REQUIRE(result.valid());
    REQUIRE(trace.rejected_calls.empty());
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == windingCase.failure->code);
    REQUIRE(result.errors()[0].message == windingCase.failure->message);
    REQUIRE(trace.rejected_calls.size() == 1);
    REQUIRE(trace.rejected_calls[0] == windingCase.failure->operation);
  }

  const std::array<TransferCall, 3> expectedTransfers{{{.operation = Transfer::ReadTriangles, .offset = 0, .values = 12},
                                                       {.operation = Transfer::ReadIds, .offset = 0, .values = inputIds.size()},
                                                       {.operation = Transfer::WriteTriangles, .offset = 0, .values = 12}}};
  REQUIRE(trace.calls.size() == windingCase.expected_calls);
  REQUIRE(windingCase.expected_calls <= expectedTransfers.size());
  for(usize callIdx = 0; callIdx < windingCase.expected_calls; ++callIdx)
  {
    CAPTURE(callIdx);
    REQUIRE(trace.calls[callIdx].operation == expectedTransfers[callIdx].operation);
    REQUIRE(trace.calls[callIdx].offset == expectedTransfers[callIdx].offset);
    REQUIRE(trace.calls[callIdx].values == expectedTransfers[callIdx].values);
  }
  REQUIRE(cancellationWitnesses == (windingCase.cancel_after_feature ? 1 : 0));
  REQUIRE(shouldCancel.load() == (windingCase.pre_cancelled || windingCase.cancel_after_feature));

  REQUIRE(triangles.getSize() == windingCase.expected_stored_triangles->size());
  for(usize valueIdx = 0; valueIdx < triangles.getSize(); ++valueIdx)
  {
    CAPTURE(valueIdx);
    REQUIRE(triangles[valueIdx] == (*windingCase.expected_stored_triangles)[valueIdx]);
  }
  if(windingCase.expected_written_triangles == nullptr)
  {
    REQUIRE(triangles.attempted_write.empty());
  }
  else
  {
    REQUIRE(triangles.attempted_write.size() == windingCase.expected_written_triangles->size());
    for(usize valueIdx = 0; valueIdx < triangles.attempted_write.size(); ++valueIdx)
    {
      CAPTURE(valueIdx);
      REQUIRE(triangles.attempted_write[valueIdx] == (*windingCase.expected_written_triangles)[valueIdx]);
    }
  }
  REQUIRE(ids.attempted_write.empty());
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
