#include <catch2/catch.hpp>

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/ExternalEquivalence.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"

#include <array>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
class SpyTemporaryRecordStore : public ITemporaryRecordStore
{
public:
  SpyTemporaryRecordStore(uint64 count, uint64 width, uint64 batch)
  : m_Count(count)
  , m_Width(width)
  , m_Batch(batch)
  , m_Bytes(static_cast<usize>(count * width))
  {
  }
  uint64 recordSize() const override
  {
    return m_Width;
  }
  uint64 recordCount() const override
  {
    return m_Count;
  }
  uint64 maxRecordsPerBatch() const override
  {
    return m_Batch;
  }
  bool isReadOnly() const override
  {
    return false;
  }
  Result<uint64> read(uint64 offset, uint64 count, nonstd::span<std::byte> bytes, const std::atomic_bool& cancel) const override
  {
    ++readCalls;
    readOffsets.push_back(offset);
    if(cancel || failRead)
      return MakeErrorResult<uint64>(-9000, "spy read failure");
    if(shortRead)
      return {count == 0 ? 0 : count - 1};
    if(offset > m_Count || count > m_Count - offset || bytes.size() < count * m_Width)
      return MakeErrorResult<uint64>(-9001, "spy invalid read");
    std::memcpy(bytes.data(), m_Bytes.data() + offset * m_Width, static_cast<usize>(count * m_Width));
    return {count};
  }
  Result<> write(uint64 offset, uint64 count, nonstd::span<const std::byte> bytes, const std::atomic_bool& cancel) override
  {
    ++writeCalls;
    writeOffsets.push_back(offset);
    if(cancel || failWrite)
      return MakeErrorResult(-9002, "spy write failure");
    if(offset > m_Count || count > m_Count - offset || bytes.size() != count * m_Width)
      return MakeErrorResult(-9003, "spy invalid write");
    std::memcpy(m_Bytes.data() + offset * m_Width, bytes.data(), static_cast<usize>(count * m_Width));
    return {};
  }
  Result<> fill(uint64, uint64, nonstd::span<const std::byte>, const std::atomic_bool&) override
  {
    return {};
  }
  Result<> resize(uint64, const std::atomic_bool&) override
  {
    return {};
  }
  mutable usize readCalls = 0;
  usize writeCalls = 0;
  bool shortRead = false;
  bool failRead = false;
  bool failWrite = false;
  mutable std::vector<uint64> readOffsets;
  std::vector<uint64> writeOffsets;
  std::vector<std::byte> m_Bytes;

private:
  uint64 m_Count;
  uint64 m_Width;
  uint64 m_Batch;
};
} // namespace

TEST_CASE("TemporaryRecordStore configuration retains 64-bit fixed-record limits", "[TemporaryRecordStore]")
{
  TemporaryRecordStoreConfig config;
  config.recordSize = 8;
  config.maxRecordsPerBatch = 3;
  config.initialRecordCount = uint64{1} << 40;

  REQUIRE(config.recordSize == 8);
  REQUIRE(config.initialRecordCount == (uint64{1} << 40));
}

TEST_CASE("Result assertion macros evaluate expressions once", "[TemporaryRecordStore]")
{
  STATIC_REQUIRE(NX_IS_LVALUE_RESULT(std::declval<Result<>&>()));
  STATIC_REQUIRE_FALSE(NX_IS_LVALUE_RESULT(std::declval<Result<>&&>()));
  STATIC_REQUIRE_FALSE(NX_IS_LVALUE_RESULT(std::declval<int&>()));

  usize evaluationCount = 0;
  auto invalidResult = MakeErrorResult(-9004, "expected test error");
  const auto makeInvalidResult = [&evaluationCount, &invalidResult]() -> Result<>& {
    ++evaluationCount;
    return invalidResult;
  };

  SIMPLNX_RESULT_REQUIRE_INVALID(makeInvalidResult());
  REQUIRE(evaluationCount == 1);
}

TEST_CASE("ExternalEquivalence resolves ordered unions and isolated labels", "[TemporaryRecordStore]")
{
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(ExternalEquivalence::Node);
  config.maxRecordsPerBatch = 8;
  config.initialRecordCount = 128;
  auto storeResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
  auto equivalenceResult = ExternalEquivalence::Create(std::move(storeResult.value()), 4, 2);
  SIMPLNX_RESULT_REQUIRE_VALID(equivalenceResult);
  auto& equivalence = *equivalenceResult.value();
  const std::atomic_bool shouldCancel = false;

  auto labels12And4UnionResult = equivalence.unite(12, 4, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(labels12And4UnionResult);
  auto labels4And9UnionResult = equivalence.unite(4, 9, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(labels4And9UnionResult);
  auto rootResult = equivalence.find(9, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(rootResult);
  REQUIRE(rootResult.value() == 4);
  auto isolatedResult = equivalence.find(99, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(isolatedResult);
  REQUIRE(isolatedResult.value() == 99);
}

TEST_CASE("InMemoryTemporaryRecordStore enforces bounded bulk contracts", "[TemporaryRecordStore]")
{
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(uint64);
  config.maxRecordsPerBatch = 2;
  config.initialRecordCount = 4;
  const std::atomic_bool active = false;
  const std::atomic_bool cancelled = true;
  auto storeResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
  auto store = std::move(storeResult.value());
  const std::array<uint64, 2> values = {3, 4};
  const auto input = nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(values.data()), sizeof(values));
  std::array<uint64, 2> output = {};
  const auto result = nonstd::span<std::byte>(reinterpret_cast<std::byte*>(output.data()), sizeof(output));
  auto bulkWriteResult = store->write(1, 2, input, active);
  SIMPLNX_RESULT_REQUIRE_VALID(bulkWriteResult);
  auto read = store->read(1, 2, result, active);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(output == values);
  auto oversizedWriteResult = store->write(0, 3, input, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(oversizedWriteResult);
  auto cancelledReadResult = store->read(0, 1, result.first(sizeof(uint64)), cancelled);
  SIMPLNX_RESULT_REQUIRE_INVALID(cancelledReadResult);
  auto endEmptyWriteResult = store->write(4, 0, nonstd::span<const std::byte>{}, active);
  SIMPLNX_RESULT_REQUIRE_VALID(endEmptyWriteResult);
  auto emptyRead = store->read(4, 0, nonstd::span<std::byte>{}, active);
  SIMPLNX_RESULT_REQUIRE_VALID(emptyRead);
  REQUIRE(emptyRead.value() == 0);
  auto pastEndEmptyWriteResult = store->write(5, 0, nonstd::span<const std::byte>{}, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(pastEndEmptyWriteResult);
  auto growStoreResult = store->resize(6, active);
  SIMPLNX_RESULT_REQUIRE_VALID(growStoreResult);
  REQUIRE(store->recordCount() == 6);
}

TEST_CASE("DataIOCollection discovers a temporary record-store provider", "[TemporaryRecordStore]")
{
  class TemporaryStoreManager : public IDataIOManager
  {
  public:
    std::string formatName() const override
    {
      return "TemporaryStoreTest";
    }
    bool supportsTemporaryRecordStore() const override
    {
      return true;
    }
    Result<std::unique_ptr<ITemporaryRecordStore>> createTemporaryRecordStore(const TemporaryRecordStoreConfig& config) const override
    {
      auto result = InMemoryTemporaryRecordStore::Create(config);
      if(result.invalid())
      {
        return ConvertInvalidResult<std::unique_ptr<ITemporaryRecordStore>>(std::move(result));
      }
      return {std::unique_ptr<ITemporaryRecordStore>(std::move(result.value()))};
    }
  };

  DataIOCollection collection;
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(uint64);
  config.maxRecordsPerBatch = 2;
  config.initialRecordCount = 4;
  REQUIRE_FALSE(collection.hasTemporaryRecordStoreCapability());
  auto temporaryStoreCreationResult = collection.createTemporaryRecordStore(config);
  SIMPLNX_RESULT_REQUIRE_INVALID(temporaryStoreCreationResult);
  auto managerRegistrationResult = collection.addIOManager(std::make_shared<TemporaryStoreManager>());
  SIMPLNX_RESULT_REQUIRE_VALID(managerRegistrationResult);
  REQUIRE(collection.hasTemporaryRecordStoreCapability());
  auto result = collection.createTemporaryRecordStore(config);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.value()->recordCount() == 4);
}

TEST_CASE("DataIOCollection forwards datastore chunk-shape hints and CreateArrayAction retains them", "[TemporaryRecordStore]")
{
  struct RecordedHint
  {
    std::optional<ShapeType> value;
    DataStoreInitializationMode initializationMode = DataStoreInitializationMode::Default;
  };

  class HintRecordingManager : public IDataIOManager
  {
  public:
    explicit HintRecordingManager(std::shared_ptr<RecordedHint> recordedHintRef)
    : m_RecordedHint(std::move(recordedHintRef))
    {
      addDataStoreCreationFnc(formatName(), [recordedState = m_RecordedHint](DataType dataType, const ShapeType& tupleShape, const ShapeType& componentShape,
                                                                             const std::optional<ShapeType>& chunkShapeHint, DataStoreInitializationMode initializationMode) {
        recordedState->value = chunkShapeHint;
        recordedState->initializationMode = initializationMode;
        if(dataType != DataType::int32)
        {
          return std::unique_ptr<IDataStore>{};
        }
        return std::unique_ptr<IDataStore>(std::make_unique<DataStore<int32>>(tupleShape, componentShape, 0));
      });
    }

    std::string formatName() const override
    {
      return "ChunkHintRecording";
    }

  private:
    std::shared_ptr<RecordedHint> m_RecordedHint;
  };

  const std::optional<ShapeType> chunkShapeHint = ShapeType{1, 7, 32};
  auto recordedHint = std::make_shared<RecordedHint>();
  DataIOCollection collection;
  Result<> addIOManagerResult = collection.addIOManager(std::make_shared<HintRecordingManager>(recordedHint));
  SIMPLNX_RESULT_REQUIRE_VALID(addIOManagerResult);
  auto store = collection.createDataStoreWithType<int32>("ChunkHintRecording", ShapeType{8, 17, 32}, ShapeType{1}, chunkShapeHint);
  REQUIRE(store != nullptr);
  REQUIRE(recordedHint->value == chunkShapeHint);
  REQUIRE(recordedHint->initializationMode == DataStoreInitializationMode::Default);

  store = collection.createDataStoreWithType<int32>("ChunkHintRecording", ShapeType{8, 17, 32}, ShapeType{1}, chunkShapeHint, DataStoreInitializationMode::DeferredZeroFill);
  REQUIRE(store != nullptr);
  REQUIRE(recordedHint->value == chunkShapeHint);
  REQUIRE(recordedHint->initializationMode == DataStoreInitializationMode::DeferredZeroFill);

  CreateArrayAction action(DataType::int32, ShapeType{8, 17, 32}, ShapeType{1}, DataPath({"Image", "Data"}), "", "", chunkShapeHint);
  REQUIRE(action.chunkShapeHint() == chunkShapeHint);
  REQUIRE(action.initializationMode() == DataStoreInitializationMode::Default);
  const auto clone = action.clone();
  const auto* clonedAction = dynamic_cast<const CreateArrayAction*>(clone.get());
  REQUIRE(clonedAction != nullptr);
  REQUIRE(clonedAction->chunkShapeHint() == chunkShapeHint);
  REQUIRE(clonedAction->initializationMode() == DataStoreInitializationMode::Default);

  CreateArrayAction deferredAction(DataType::int32, ShapeType{8, 17, 32}, ShapeType{1}, DataPath({"Image", "Deferred"}), "", "", chunkShapeHint, DataStoreInitializationMode::DeferredZeroFill);
  REQUIRE(deferredAction.initializationMode() == DataStoreInitializationMode::DeferredZeroFill);
  const auto deferredClone = deferredAction.clone();
  const auto* clonedDeferredAction = dynamic_cast<const CreateArrayAction*>(deferredClone.get());
  REQUIRE(clonedDeferredAction != nullptr);
  REQUIRE(clonedDeferredAction->initializationMode() == DataStoreInitializationMode::DeferredZeroFill);

  CreateArrayAction fillAction(DataType::int32, ShapeType{8, 17, 32}, ShapeType{1}, DataPath({"Image", "Filled"}), "", "12", chunkShapeHint, DataStoreInitializationMode::DeferredZeroFill);
  REQUIRE(fillAction.initializationMode() == DataStoreInitializationMode::Default);
}

TEST_CASE("ExternalEquivalence handles chains, sparse labels, duplicates, cancellation, and flush", "[TemporaryRecordStore]")
{
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(ExternalEquivalence::Node);
  config.maxRecordsPerBatch = 4;
  config.initialRecordCount = 128;
  const std::atomic_bool active = false;
  const std::atomic_bool cancelled = true;
  auto storeResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
  auto equivalenceResult = ExternalEquivalence::Create(std::move(storeResult.value()), 2, 1);
  SIMPLNX_RESULT_REQUIRE_VALID(equivalenceResult);
  auto& equivalence = *equivalenceResult.value();
  auto labels90And70UnionResult = equivalence.unite(90, 70, active);
  SIMPLNX_RESULT_REQUIRE_VALID(labels90And70UnionResult);
  auto labels70And50UnionResult = equivalence.unite(70, 50, active);
  SIMPLNX_RESULT_REQUIRE_VALID(labels70And50UnionResult);
  auto labels50And30UnionResult = equivalence.unite(50, 30, active);
  SIMPLNX_RESULT_REQUIRE_VALID(labels50And30UnionResult);
  auto labels30And10UnionResult = equivalence.unite(30, 10, active);
  SIMPLNX_RESULT_REQUIRE_VALID(labels30And10UnionResult);
  auto labels10And30UnionResult = equivalence.unite(10, 30, active);
  SIMPLNX_RESULT_REQUIRE_VALID(labels10And30UnionResult);
  auto rootResult = equivalence.find(90, active);
  SIMPLNX_RESULT_REQUIRE_VALID(rootResult);
  REQUIRE(rootResult.value() == 10);
  auto sizeResult = equivalence.componentSize(70, active);
  SIMPLNX_RESULT_REQUIRE_VALID(sizeResult);
  REQUIRE(sizeResult.value() == 5);
  auto equivalenceFlushResult = equivalence.flush(active);
  SIMPLNX_RESULT_REQUIRE_VALID(equivalenceFlushResult);
  auto outOfBoundsFindResult = equivalence.find(128, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(outOfBoundsFindResult);
  auto cancelledFindResult = equivalence.find(10, cancelled);
  SIMPLNX_RESULT_REQUIRE_INVALID(cancelledFindResult);
}

TEST_CASE("BoundedRecordPageCache obeys cache hits, LRU eviction, partial pages, and failures", "[TemporaryRecordStore]")
{
  const std::atomic_bool active = false;
  SpyTemporaryRecordStore store(5, sizeof(uint64), 2);
  BoundedRecordPageCache<uint64> cache(store, 2, 2);
  auto page0ReadResult = cache.read(0, active);
  SIMPLNX_RESULT_REQUIRE_VALID(page0ReadResult);
  auto page1ReadResult = cache.read(1, active);
  SIMPLNX_RESULT_REQUIRE_VALID(page1ReadResult);
  REQUIRE(store.readCalls == 1);
  auto page2ReadResult = cache.read(2, active);
  SIMPLNX_RESULT_REQUIRE_VALID(page2ReadResult);
  auto cachedPage0ReadResult = cache.read(0, active);
  SIMPLNX_RESULT_REQUIRE_VALID(cachedPage0ReadResult);
  auto partialPageReadResult = cache.read(4, active);
  SIMPLNX_RESULT_REQUIRE_VALID(partialPageReadResult);
  REQUIRE(store.readOffsets == std::vector<uint64>{0, 2, 4});
  auto outOfBoundsPageReadResult = cache.read(5, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(outOfBoundsPageReadResult);
  auto partialPageWriteResult = cache.write(4, 11, active);
  SIMPLNX_RESULT_REQUIRE_VALID(partialPageWriteResult);
  auto cacheFlushResult = cache.flush(active);
  SIMPLNX_RESULT_REQUIRE_VALID(cacheFlushResult);
  REQUIRE(store.writeOffsets.back() == 4);
  store.shortRead = true;
  BoundedRecordPageCache<uint64> shortCache(store, 2, 1);
  auto shortCacheReadResult = shortCache.read(0, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(shortCacheReadResult);
  store.shortRead = false;
  store.failRead = true;
  BoundedRecordPageCache<uint64> failedRead(store, 2, 1);
  auto failedReadReadResult = failedRead.read(0, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(failedReadReadResult);
  BoundedRecordPageCache<uint64> zeroPage(store, 0, 1);
  auto zeroPageReadResult = zeroPage.read(0, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(zeroPageReadResult);
  BoundedRecordPageCache<uint64> zeroCache(store, 2, 0);
  auto zeroCacheReadResult = zeroCache.read(0, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(zeroCacheReadResult);
  BoundedRecordPageCache<uint32> wrongWidth(store, 2, 1);
  auto wrongWidthReadResult = wrongWidth.read(0, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(wrongWidthReadResult);
  BoundedRecordPageCache<uint64> pageTooLarge(store, 3, 1);
  auto pageTooLargeReadResult = pageTooLarge.read(0, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(pageTooLargeReadResult);
}

TEST_CASE("BoundedRecordPageCache bulk read preserves request order across pages", "[TemporaryRecordStore]")
{
  const std::atomic_bool active = false;
  SpyTemporaryRecordStore store(8, sizeof(uint64), 4);
  const std::array<uint64, 8> storedValues = {11, 12, 13, 14, 21, 22, 23, 24};
  std::memcpy(store.m_Bytes.data(), storedValues.data(), sizeof(storedValues));
  BoundedRecordPageCache<uint64> cache(store, 4, 2);

  const std::array<uint64, 6> indices = {0, 1, 3, 4, 5, 7};
  std::array<uint64, 6> values = {};
  Result<> readManyResult = cache.readMany(indices, values, active);
  SIMPLNX_RESULT_REQUIRE_VALID(readManyResult);
  REQUIRE(values == std::array<uint64, 6>{11, 12, 14, 21, 22, 24});
  REQUIRE(store.readOffsets == std::vector<uint64>{0, 4});

  Result<> shortIndicesResult = cache.readMany(nonstd::span<const uint64>(indices.data(), indices.size() - 1), values, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(shortIndicesResult);
  const std::array<uint64, 1> invalidIndex = {8};
  std::array<uint64, 1> invalidValue = {};
  Result<> invalidIndexResult = cache.readMany(invalidIndex, invalidValue, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(invalidIndexResult);
}

TEST_CASE("BoundedRecordPageCache inspects and modifies a cached record in place", "[TemporaryRecordStore]")
{
  const std::atomic_bool active = false;
  SpyTemporaryRecordStore store(4, sizeof(uint64), 2);
  const std::array<uint64, 4> storedValues = {3, 5, 7, 9};
  std::memcpy(store.m_Bytes.data(), storedValues.data(), sizeof(storedValues));
  BoundedRecordPageCache<uint64> cache(store, 2, 1);

  uint64 inspectedValue = 0;
  Result<> inspectResult = cache.inspect(1, [&inspectedValue](const uint64& value) noexcept { inspectedValue = value; }, active);
  SIMPLNX_RESULT_REQUIRE_VALID(inspectResult);
  REQUIRE(inspectedValue == 5);
  Result<> modifyResult = cache.modify(1, [](uint64& value) noexcept { value = 41; }, active);
  SIMPLNX_RESULT_REQUIRE_VALID(modifyResult);
  Result<> flushResult = cache.flush(active);
  SIMPLNX_RESULT_REQUIRE_VALID(flushResult);

  uint64 storedValue = 0;
  std::memcpy(&storedValue, store.m_Bytes.data() + sizeof(uint64), sizeof(storedValue));
  REQUIRE(storedValue == 41);
  REQUIRE(store.readCalls == 1);
  REQUIRE(store.writeCalls == 1);
}

TEST_CASE("BoundedRecordPageCache retains dirty data after failed eviction write", "[TemporaryRecordStore]")
{
  const std::atomic_bool active = false;
  SpyTemporaryRecordStore store(4, sizeof(uint64), 2);
  BoundedRecordPageCache<uint64> cache(store, 2, 1);
  auto page0WriteResult = cache.write(0, 17, active);
  SIMPLNX_RESULT_REQUIRE_VALID(page0WriteResult);
  store.failWrite = true;
  auto failedFlushPageReadResult = cache.read(2, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(failedFlushPageReadResult);
  store.failWrite = false;
  auto cacheFlushResult = cache.flush(active);
  SIMPLNX_RESULT_REQUIRE_VALID(cacheFlushResult);
  REQUIRE(store.writeCalls >= 2);
  REQUIRE(std::all_of(store.writeOffsets.begin(), store.writeOffsets.end(), [](uint64 offset) { return offset == 0; }));
}

TEST_CASE("ExternalEquivalence compresses a 4096-node chain without resident path state", "[TemporaryRecordStore]")
{
  constexpr uint64 k_Count = 4096;
  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(ExternalEquivalence::Node);
  config.maxRecordsPerBatch = 64;
  config.initialRecordCount = k_Count;
  auto store = std::make_unique<SpyTemporaryRecordStore>(k_Count, sizeof(ExternalEquivalence::Node), 64);
  auto* spy = store.get();
  for(uint64 label = 0; label < k_Count; ++label)
  {
    const ExternalEquivalence::Node node = {label == 0 ? 0 : label - 1, label == 0 ? k_Count : 1};
    std::memcpy(spy->m_Bytes.data() + label * sizeof(node), &node, sizeof(node));
  }
  auto equivalenceResult = ExternalEquivalence::Create(std::move(store), 64, 2);
  SIMPLNX_RESULT_REQUIRE_VALID(equivalenceResult);
  const std::atomic_bool active = false;
  auto root = equivalenceResult.value()->find(k_Count - 1, active);
  SIMPLNX_RESULT_REQUIRE_VALID(root);
  REQUIRE(root.value() == 0);
  auto size = equivalenceResult.value()->componentSize(k_Count - 1, active);
  SIMPLNX_RESULT_REQUIRE_VALID(size);
  REQUIRE(size.value() == k_Count);
  auto equivalenceResultFlushResult = equivalenceResult.value()->flush(active);
  SIMPLNX_RESULT_REQUIRE_VALID(equivalenceResultFlushResult);
  ExternalEquivalence::Node tail{};
  std::memcpy(&tail, spy->m_Bytes.data() + (k_Count - 1) * sizeof(tail), sizeof(tail));
  REQUIRE(tail.parent < k_Count - 1);
}

TEST_CASE("ExternalEquivalence propagates backing-store failures and size overflow", "[TemporaryRecordStore]")
{
  const std::atomic_bool active = false;

  auto failedReadStore = std::make_unique<SpyTemporaryRecordStore>(2, sizeof(ExternalEquivalence::Node), 1);
  failedReadStore->failRead = true;
  auto failedReadEquivalence = ExternalEquivalence::Create(std::move(failedReadStore), 1, 1);
  SIMPLNX_RESULT_REQUIRE_VALID(failedReadEquivalence);
  auto failedStoreReadFindResult = failedReadEquivalence.value()->find(0, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(failedStoreReadFindResult);

  auto failedWriteStore = std::make_unique<SpyTemporaryRecordStore>(2, sizeof(ExternalEquivalence::Node), 1);
  failedWriteStore->failWrite = true;
  auto failedWriteEquivalence = ExternalEquivalence::Create(std::move(failedWriteStore), 1, 1);
  SIMPLNX_RESULT_REQUIRE_VALID(failedWriteEquivalence);
  auto existingNodeFindResult = failedWriteEquivalence.value()->find(0, active);
  SIMPLNX_RESULT_REQUIRE_VALID(existingNodeFindResult);
  auto failedStoreWriteFindResult = failedWriteEquivalence.value()->find(1, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(failedStoreWriteFindResult);

  auto overflowStore = std::make_unique<SpyTemporaryRecordStore>(2, sizeof(ExternalEquivalence::Node), 1);
  const ExternalEquivalence::Node firstNode{0, std::numeric_limits<uint64>::max()};
  const ExternalEquivalence::Node secondNode{1, 1};
  std::memcpy(overflowStore->m_Bytes.data(), &firstNode, sizeof(firstNode));
  std::memcpy(overflowStore->m_Bytes.data() + sizeof(firstNode), &secondNode, sizeof(secondNode));
  auto overflowEquivalence = ExternalEquivalence::Create(std::move(overflowStore), 1, 2);
  SIMPLNX_RESULT_REQUIRE_VALID(overflowEquivalence);
  auto overflowEquivalenceUnionResult = overflowEquivalence.value()->unite(0, 1, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowEquivalenceUnionResult);
}

TEST_CASE("InMemoryTemporaryRecordStore fills across batches and preserves invalid ranges", "[TemporaryRecordStore]")
{
  TemporaryRecordStoreConfig config{.recordSize = sizeof(uint64), .maxRecordsPerBatch = 2, .initialRecordCount = 6};
  auto storeResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(storeResult);
  auto store = std::move(storeResult.value());
  const std::atomic_bool active = false;
  const uint64 value = 9;
  const auto record = nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&value), sizeof(value));
  auto fullFillResult = store->fill(0, 6, record, active);
  SIMPLNX_RESULT_REQUIRE_VALID(fullFillResult);
  auto outOfBoundsFillResult = store->fill(5, 2, record, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(outOfBoundsFillResult);
  std::array<uint64, 2> values{};
  auto bytes = nonstd::span<std::byte>(reinterpret_cast<std::byte*>(values.data()), sizeof(values));
  auto read = store->read(4, 2, bytes, active);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(values == std::array<uint64, 2>{9, 9});
  auto growStoreResult = store->resize(8, active);
  SIMPLNX_RESULT_REQUIRE_VALID(growStoreResult);
  read = store->read(6, 2, bytes, active);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(values == std::array<uint64, 2>{0, 0});
  auto shrinkStoreResult = store->resize(1, active);
  SIMPLNX_RESULT_REQUIRE_VALID(shrinkStoreResult);
  auto pastEndReadResult = store->read(1, 1, bytes.first(sizeof(uint64)), active);
  SIMPLNX_RESULT_REQUIRE_INVALID(pastEndReadResult);

  config.readOnly = true;
  auto readOnlyResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(readOnlyResult);
  auto readOnlyStoreFillResult = readOnlyResult.value()->fill(0, 1, record, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(readOnlyStoreFillResult);
  auto readOnlyStoreResizeResult = readOnlyResult.value()->resize(7, active);
  SIMPLNX_RESULT_REQUIRE_INVALID(readOnlyStoreResizeResult);
}
