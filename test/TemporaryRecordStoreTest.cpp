#include <catch2/catch.hpp>

#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/BoundedRecordPageCache.hpp"
#include "simplnx/Utilities/ExternalEquivalence.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"

#include <array>
#include <cstring>
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
  usize evaluationCount = 0;
  const auto makeInvalidResult = [&evaluationCount]() {
    ++evaluationCount;
    return MakeErrorResult(-9004, "expected test error");
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

  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.unite(12, 4, shouldCancel));
  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.unite(4, 9, shouldCancel));
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
  SIMPLNX_RESULT_REQUIRE_VALID(store->write(1, 2, input, active));
  auto read = store->read(1, 2, result, active);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(output == values);
  SIMPLNX_RESULT_REQUIRE_INVALID(store->write(0, 3, input, active));
  SIMPLNX_RESULT_REQUIRE_INVALID(store->read(0, 1, result.first(sizeof(uint64)), cancelled));
  SIMPLNX_RESULT_REQUIRE_VALID(store->write(4, 0, nonstd::span<const std::byte>{}, active));
  auto emptyRead = store->read(4, 0, nonstd::span<std::byte>{}, active);
  SIMPLNX_RESULT_REQUIRE_VALID(emptyRead);
  REQUIRE(emptyRead.value() == 0);
  SIMPLNX_RESULT_REQUIRE_INVALID(store->write(5, 0, nonstd::span<const std::byte>{}, active));
  SIMPLNX_RESULT_REQUIRE_VALID(store->resize(6, active));
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
  SIMPLNX_RESULT_REQUIRE_INVALID(collection.createTemporaryRecordStore(config));
  SIMPLNX_RESULT_REQUIRE_VALID(collection.addIOManager(std::make_shared<TemporaryStoreManager>()));
  REQUIRE(collection.hasTemporaryRecordStoreCapability());
  auto result = collection.createTemporaryRecordStore(config);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.value()->recordCount() == 4);
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
  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.unite(90, 70, active));
  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.unite(70, 50, active));
  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.unite(50, 30, active));
  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.unite(30, 10, active));
  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.unite(10, 30, active));
  auto rootResult = equivalence.find(90, active);
  SIMPLNX_RESULT_REQUIRE_VALID(rootResult);
  REQUIRE(rootResult.value() == 10);
  auto sizeResult = equivalence.componentSize(70, active);
  SIMPLNX_RESULT_REQUIRE_VALID(sizeResult);
  REQUIRE(sizeResult.value() == 5);
  SIMPLNX_RESULT_REQUIRE_VALID(equivalence.flush(active));
  SIMPLNX_RESULT_REQUIRE_INVALID(equivalence.find(128, active));
  SIMPLNX_RESULT_REQUIRE_INVALID(equivalence.find(10, cancelled));
}

TEST_CASE("BoundedRecordPageCache obeys cache hits, LRU eviction, partial pages, and failures", "[TemporaryRecordStore]")
{
  const std::atomic_bool active = false;
  SpyTemporaryRecordStore store(5, sizeof(uint64), 2);
  BoundedRecordPageCache<uint64> cache(store, 2, 2);
  SIMPLNX_RESULT_REQUIRE_VALID(cache.read(0, active));
  SIMPLNX_RESULT_REQUIRE_VALID(cache.read(1, active));
  REQUIRE(store.readCalls == 1);
  SIMPLNX_RESULT_REQUIRE_VALID(cache.read(2, active));
  SIMPLNX_RESULT_REQUIRE_VALID(cache.read(0, active));
  SIMPLNX_RESULT_REQUIRE_VALID(cache.read(4, active));
  REQUIRE(store.readOffsets == std::vector<uint64>{0, 2, 4});
  SIMPLNX_RESULT_REQUIRE_INVALID(cache.read(5, active));
  SIMPLNX_RESULT_REQUIRE_VALID(cache.write(4, 11, active));
  SIMPLNX_RESULT_REQUIRE_VALID(cache.flush(active));
  REQUIRE(store.writeOffsets.back() == 4);
  store.shortRead = true;
  BoundedRecordPageCache<uint64> shortCache(store, 2, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(shortCache.read(0, active));
  store.shortRead = false;
  store.failRead = true;
  BoundedRecordPageCache<uint64> failedRead(store, 2, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(failedRead.read(0, active));
  BoundedRecordPageCache<uint64> zeroPage(store, 0, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(zeroPage.read(0, active));
  BoundedRecordPageCache<uint64> zeroCache(store, 2, 0);
  SIMPLNX_RESULT_REQUIRE_INVALID(zeroCache.read(0, active));
  BoundedRecordPageCache<uint32> wrongWidth(store, 2, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(wrongWidth.read(0, active));
  BoundedRecordPageCache<uint64> pageTooLarge(store, 3, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(pageTooLarge.read(0, active));
}

TEST_CASE("BoundedRecordPageCache retains dirty data after failed eviction write", "[TemporaryRecordStore]")
{
  const std::atomic_bool active = false;
  SpyTemporaryRecordStore store(4, sizeof(uint64), 2);
  BoundedRecordPageCache<uint64> cache(store, 2, 1);
  SIMPLNX_RESULT_REQUIRE_VALID(cache.write(0, 17, active));
  store.failWrite = true;
  SIMPLNX_RESULT_REQUIRE_INVALID(cache.read(2, active));
  store.failWrite = false;
  SIMPLNX_RESULT_REQUIRE_VALID(cache.flush(active));
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
  SIMPLNX_RESULT_REQUIRE_VALID(equivalenceResult.value()->flush(active));
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
  SIMPLNX_RESULT_REQUIRE_INVALID(failedReadEquivalence.value()->find(0, active));

  auto failedWriteStore = std::make_unique<SpyTemporaryRecordStore>(2, sizeof(ExternalEquivalence::Node), 1);
  failedWriteStore->failWrite = true;
  auto failedWriteEquivalence = ExternalEquivalence::Create(std::move(failedWriteStore), 1, 1);
  SIMPLNX_RESULT_REQUIRE_VALID(failedWriteEquivalence);
  SIMPLNX_RESULT_REQUIRE_VALID(failedWriteEquivalence.value()->find(0, active));
  SIMPLNX_RESULT_REQUIRE_INVALID(failedWriteEquivalence.value()->find(1, active));

  auto overflowStore = std::make_unique<SpyTemporaryRecordStore>(2, sizeof(ExternalEquivalence::Node), 1);
  const ExternalEquivalence::Node firstNode{0, std::numeric_limits<uint64>::max()};
  const ExternalEquivalence::Node secondNode{1, 1};
  std::memcpy(overflowStore->m_Bytes.data(), &firstNode, sizeof(firstNode));
  std::memcpy(overflowStore->m_Bytes.data() + sizeof(firstNode), &secondNode, sizeof(secondNode));
  auto overflowEquivalence = ExternalEquivalence::Create(std::move(overflowStore), 1, 2);
  SIMPLNX_RESULT_REQUIRE_VALID(overflowEquivalence);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowEquivalence.value()->unite(0, 1, active));
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
  SIMPLNX_RESULT_REQUIRE_VALID(store->fill(0, 6, record, active));
  SIMPLNX_RESULT_REQUIRE_INVALID(store->fill(5, 2, record, active));
  std::array<uint64, 2> values{};
  auto bytes = nonstd::span<std::byte>(reinterpret_cast<std::byte*>(values.data()), sizeof(values));
  auto read = store->read(4, 2, bytes, active);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(values == std::array<uint64, 2>{9, 9});
  SIMPLNX_RESULT_REQUIRE_VALID(store->resize(8, active));
  read = store->read(6, 2, bytes, active);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(values == std::array<uint64, 2>{0, 0});
  SIMPLNX_RESULT_REQUIRE_VALID(store->resize(1, active));
  SIMPLNX_RESULT_REQUIRE_INVALID(store->read(1, 1, bytes.first(sizeof(uint64)), active));

  config.readOnly = true;
  auto readOnlyResult = InMemoryTemporaryRecordStore::Create(config);
  SIMPLNX_RESULT_REQUIRE_VALID(readOnlyResult);
  SIMPLNX_RESULT_REQUIRE_INVALID(readOnlyResult.value()->fill(0, 1, record, active));
  SIMPLNX_RESULT_REQUIRE_INVALID(readOnlyResult.value()->resize(7, active));
}
