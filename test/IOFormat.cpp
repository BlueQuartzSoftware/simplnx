#include <catch2/catch.hpp>

#include "simplnx/Common/SimplnxConfig.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

#include <algorithm>
#include <array>

using namespace nx::core;

TEST_CASE("Contains HDF5 IO Support", "IOTest")
{
  auto app = Application::GetOrCreateInstance();

  auto& ioCollection = app->getIOCollection();
  auto h5IO = ioCollection.getManager("HDF5");
  REQUIRE(h5IO != nullptr);
}

TEST_CASE("Memory Check", "IOTest")
{
  REQUIRE(Memory::GetTotalMemory() > 0);
  const auto storage = Memory::GetAvailableStorage();
  REQUIRE(storage.total > 0);
  REQUIRE(storage.free > 0);
}

// These tests verify DataStorageMode behavior when no OOC manager is registered.
// Every created store is therefore in memory. useOocData() still reports user intent.
// OOC builds cover disk-backed format selection in the OOC plugin tests.

TEST_CASE("Data Format: ForceInCore keeps useOocData false and stores in memory", "[IOTest][DataFormat]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceInCore, app->getPreferences()->valueAs<int64>(Preferences::k_LargeDataSize_Key));

  // ForceInCore is the only mode for which OOC is "not in use".
  REQUIRE_FALSE(app->getPreferences()->useOocData());

  // CreateDataStore should produce an InMemory store regardless of size.
  DataStructure ds;
  DataPath dp({"TestArray"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, dp, {100, 100, 100}, {1});
  REQUIRE(store != nullptr);
  REQUIRE(store->getStoreType() == IDataStore::StoreType::InMemory);
}

TEST_CASE("Data Format: Adaptive and ForceOutOfCore report useOocData true", "[IOTest][DataFormat]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::Adaptive, app->getPreferences()->valueAs<int64>(Preferences::k_LargeDataSize_Key));

  // OOC is "in use" for both size-driven and always-out-of-core intents.
  REQUIRE(app->getPreferences()->useOocData());

  app->getPreferences()->setDataStorageMode(DataStorageMode::ForceOutOfCore);
  REQUIRE(app->getPreferences()->useOocData());

  // With no OOC manager registered in this build, even ForceOutOfCore can only
  // produce an in-memory store — the resolver has no disk-backed format to return.
  DataStructure ds;
  DataPath dp({"TestArray"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, dp, {100, 100, 100}, {1});
  REQUIRE(store != nullptr);
  REQUIRE(store->getStoreType() == IDataStore::StoreType::InMemory);
}

TEST_CASE("Data Format: Cannot register IO manager with reserved InMemory name", "[IOTest][DataFormat]")
{
  // Create a dummy IDataIOManager subclass that returns k_InMemoryFormat
  class ReservedNameManager : public IDataIOManager
  {
  public:
    std::string formatName() const override
    {
      return std::string(Preferences::k_InMemoryFormat);
    }
  };

  auto& ioCollection = Application::GetOrCreateInstance()->getIOCollection();
  auto badManager = std::make_shared<ReservedNameManager>();
  auto addResult = ioCollection.addIOManager(badManager);
  SIMPLNX_RESULT_REQUIRE_INVALID(addResult);
}

TEST_CASE("Data Format: core factories reject null managers and retain in-memory fallback", "[IOTest][DataFormat]")
{
  class OtherManager : public IDataIOManager
  {
  public:
    std::string formatName() const override
    {
      return "OtherFormat";
    }
  };

  DataIOCollection collection;
  const Result<> nullResult = collection.addIOManager(nullptr);
  REQUIRE(nullResult.invalid());
  REQUIRE(nullResult.errors().size() == 1);
  REQUIRE(nullResult.errors().front().code == -6010);

  auto addIOManagerResult = collection.addIOManager(std::make_shared<OtherManager>());
  SIMPLNX_RESULT_REQUIRE_VALID(addIOManagerResult);

  const ShapeType tupleShape = {2, 3};
  const ShapeType componentShape = {2};
  auto inMemoryStore = collection.createDataStoreWithType<int32>(std::string(Preferences::k_InMemoryFormat), tupleShape, componentShape);
  REQUIRE(inMemoryStore != nullptr);
  CHECK(inMemoryStore->getDataType() == DataType::int32);
  CHECK(inMemoryStore->getStoreType() == IDataStore::StoreType::InMemory);
  CHECK(inMemoryStore->getTupleShape() == tupleShape);
  CHECK(inMemoryStore->getComponentShape() == componentShape);
  inMemoryStore->setValue(7, 42);
  CHECK(inMemoryStore->getValue(7) == 42);

  auto fallbackStore = collection.createDataStoreWithType<int32>("UnknownFormat", tupleShape, componentShape);
  REQUIRE(fallbackStore != nullptr);
  CHECK(fallbackStore->getDataType() == DataType::int32);
  CHECK(fallbackStore->getStoreType() == IDataStore::StoreType::InMemory);
  CHECK(fallbackStore->getTupleShape() == tupleShape);
  CHECK(fallbackStore->getComponentShape() == componentShape);
  fallbackStore->setValue(11, 84);
  CHECK(fallbackStore->getValue(11) == 84);

  auto fallbackListStore = collection.createListStoreWithType<int32>("UnknownFormat", tupleShape);
  REQUIRE(fallbackListStore != nullptr);
  CHECK_FALSE(fallbackListStore->isOutOfCore());
  CHECK(fallbackListStore->getTupleShape() == tupleShape);
  fallbackListStore->setList(4, {3, 5, 8});
  CHECK((fallbackListStore->getList(4) == std::vector<int32>{3, 5, 8}));
}

TEST_CASE("Data Format: NeighborList action retains an explicit InMemory format", "[IOTest][DataFormat]")
{
  const DataPath path({"Neighbors"});
  const CreateNeighborListAction action(DataType::int32, {3}, path, std::string(Preferences::k_InMemoryFormat));

  const auto clonedAction = action.clone();
  const auto* clonedNeighborListAction = dynamic_cast<const CreateNeighborListAction*>(clonedAction.get());
  REQUIRE(clonedNeighborListAction != nullptr);
  CHECK(clonedNeighborListAction->dataFormat() == Preferences::k_InMemoryFormat);

  DataStructure preflightDataStructure;
  auto applyResult2 = action.apply(preflightDataStructure, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult2);
  NeighborList<int32>* preflightList = nullptr;
  REQUIRE_NOTHROW(preflightList = &preflightDataStructure.getDataRefAs<NeighborList<int32>>(path));
  REQUIRE(preflightList != nullptr);
  CHECK(preflightList->getIListStore()->getTupleShape() == ShapeType{3});

  DataStructure executeDataStructure;
  auto applyResult = action.apply(executeDataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  NeighborList<int32>* executeList = nullptr;
  REQUIRE_NOTHROW(executeList = &executeDataStructure.getDataRefAs<NeighborList<int32>>(path));
  REQUIRE(executeList != nullptr);
  CHECK(executeList->getDataType() == DataType::int32);
  CHECK_FALSE(executeList->getIListStore()->isOutOfCore());
  CHECK(executeList->getIListStore()->getTupleShape() == ShapeType{3});
  executeList->setList(1, {13, 21});
  CHECK((executeList->getList(1) == std::vector<int32>{13, 21}));
}

TEST_CASE("Data Format: external-sort capability is optional and storage neutral", "[IOTest][ExternalSort]")
{
  class FakeExternalSort : public IExternalSort
  {
  public:
    Result<> append(uint64 recordCount, nonstd::span<const std::byte> records, const std::atomic_bool& shouldCancel, const ExternalSortProgressCallback& progressCallback) override
    {
      if(shouldCancel)
      {
        return MakeErrorResult(-7000, "cancelled");
      }
      m_AppendedRecordCount += recordCount;
      m_LastBatchSize = records.size();
      if(progressCallback)
      {
        progressCallback(recordCount, recordCount);
      }
      return {};
    }

    Result<> finish(const std::atomic_bool& shouldCancel, const ExternalSortProgressCallback&) override
    {
      return shouldCancel ? MakeErrorResult(-7001, "cancelled") : Result<>{};
    }

    Result<uint64> read(uint64 recordOffset, uint64 recordCount, nonstd::span<std::byte> records, const std::atomic_bool& shouldCancel) const override
    {
      if(shouldCancel)
      {
        return MakeErrorResult<uint64>(-7002, "cancelled");
      }
      std::fill(records.begin(), records.end(), std::byte{0x5A});
      return {recordCount};
    }

    uint64 recordCount() const override
    {
      return m_AppendedRecordCount;
    }

    uint64 m_AppendedRecordCount = 0;
    usize m_LastBatchSize = 0;
  };

  class FakeExternalSortManager : public IDataIOManager
  {
  public:
    std::string formatName() const override
    {
      return "FakeExternalSort";
    }

    bool supportsExternalSort() const override
    {
      return true;
    }

    Result<std::unique_ptr<IExternalSort>> createExternalSort(const ExternalSortConfig& config) const override
    {
      if(config.recordSize != 2 || config.maxRecordsPerBatch != 3 || !config.compare)
      {
        return MakeErrorResult<std::unique_ptr<IExternalSort>>(-7004, "unexpected config");
      }
      return {std::make_unique<FakeExternalSort>()};
    }
  };

  DataIOCollection collection;
  REQUIRE_FALSE(collection.hasExternalSortCapability());

  auto addResult = collection.addIOManager(std::make_shared<FakeExternalSortManager>());
  SIMPLNX_RESULT_REQUIRE_VALID(addResult);
  REQUIRE(collection.hasExternalSortCapability());

  ExternalSortConfig config;
  config.recordSize = 2;
  config.maxRecordsPerBatch = 3;
  config.compare = [](nonstd::span<const std::byte> left, nonstd::span<const std::byte> right) { return left[0] < right[0] ? -1 : (left[0] > right[0] ? 1 : 0); };

  auto sortResult = collection.createExternalSort(config);
  SIMPLNX_RESULT_REQUIRE_VALID(sortResult);
  auto sort = std::move(sortResult.value());

  const std::array<std::byte, 6> input = {std::byte{3}, std::byte{0}, std::byte{1}, std::byte{0}, std::byte{2}, std::byte{0}};
  const std::atomic_bool shouldCancel = false;
  uint64 reportedProgress = 0;
  auto sortAppendResult = sort->append(3, input, shouldCancel, [&reportedProgress](uint64 completed, uint64 total) {
    REQUIRE(completed == 3);
    REQUIRE(total == 3);
    reportedProgress = completed;
  });
  SIMPLNX_RESULT_REQUIRE_VALID(sortAppendResult);
  auto sortFinishResult = sort->finish(shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_VALID(sortFinishResult);

  std::array<std::byte, 6> output = {};
  auto readResult = sort->read(0, 3, output, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  REQUIRE(readResult.value() == 3);
  REQUIRE(reportedProgress == 3);
  REQUIRE(output.front() == std::byte{0x5A});
}
