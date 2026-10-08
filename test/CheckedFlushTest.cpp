#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AbstractListStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IListStore.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @enum FlushFailure
 * @brief Selects an exception from a custom flush operation.
 */
enum class FlushFailure
{
  None,      ///< Completes the operation.
  Standard,  ///< Throws a storage error with backing context.
  Unknown,   ///< Throws a nonstandard exception.
  Allocation ///< Throws a simulated allocation failure.
};

/**
 * @brief Throws the selected exception while diagnostic memory remains available.
 * @param failure Selects the exception category.
 */
void ThrowFlushFailure(FlushFailure failure)
{
  switch(failure)
  {
  case FlushFailure::None:
    return;
  case FlushFailure::Standard:
    throw std::runtime_error("backing.h5:/values: injected flush failure");
  case FlushFailure::Unknown:
    throw 42;
  case FlushFailure::Allocation:
    throw std::bad_alloc{};
  }
}

/**
 * @class LegacyFlushStore
 * @brief Supplies a legacy numeric override for the checked default adapter.
 */
class LegacyFlushStore : public DataStore<int32>
{
public:
  /**
   * @brief Creates two initialized resident values.
   */
  LegacyFlushStore()
  : DataStore<int32>(ShapeType{2}, ShapeType{1}, int32{0})
  {
  }

  /**
   * @brief Counts the legacy call and throws the selected exception.
   */
  void flush() const override
  {
    ++FlushCalls;
    ThrowFlushFailure(Failure);
  }

  FlushFailure Failure = FlushFailure::None;
  mutable usize FlushCalls = 0;
};

/**
 * @class ReportingFlushStore
 * @brief Supplies numeric diagnostics without calling the legacy operation.
 */
class ReportingFlushStore : public DataStore<int32>
{
public:
  /**
   * @brief Creates two initialized resident values.
   */
  ReportingFlushStore()
  : DataStore<int32>(ShapeType{2}, ShapeType{1}, int32{0})
  {
  }

  /**
   * @brief Counts calls to the separate legacy operation.
   */
  void flush() const override
  {
    ++LegacyCalls;
  }

  /**
   * @brief Counts the checked call and returns the selected diagnostics.
   * @return The configured result, including warnings.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    ++CheckedCalls;
    return FlushResult;
  }

  Result<> FlushResult;
  mutable usize CheckedCalls = 0;
  mutable usize LegacyCalls = 0;
};

/**
 * @class ReportingFlushListStore
 * @brief Supplies list diagnostics to a real NeighborList object.
 */
class ReportingFlushListStore : public ListStore<int32>
{
public:
  /**
   * @brief Creates two empty resident lists.
   */
  ReportingFlushListStore()
  : ListStore<int32>(ShapeType{2})
  {
  }

  /**
   * @brief Counts the checked call and returns the selected diagnostics.
   * @return The configured result, including warnings.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    ++CheckedCalls;
    return FlushResult;
  }

  Result<> FlushResult;
  mutable usize CheckedCalls = 0;
};

/**
 * @class LegacyFlushGroup
 * @brief Supplies a legacy object override for the DataObject default adapter.
 */
class LegacyFlushGroup : public DataGroup
{
public:
  /**
   * @brief Creates a named group with normal DataObject identity.
   * @param dataStructure Owns the group after insertion.
   * @param name Names the group.
   */
  LegacyFlushGroup(DataStructure& dataStructure, std::string name)
  : DataGroup(dataStructure, std::move(name))
  {
  }

  /**
   * @brief Counts the legacy call and throws the selected exception.
   */
  void flush() const override
  {
    ++FlushCalls;
    ThrowFlushFailure(Failure);
  }

  FlushFailure Failure = FlushFailure::None;
  mutable usize FlushCalls = 0;
};

/**
 * @class ThrowingCheckedFlushGroup
 * @brief Bypasses the default adapter to exercise DataStructure failure handling.
 */
class ThrowingCheckedFlushGroup : public DataGroup
{
public:
  /**
   * @brief Creates a named group with normal DataObject identity.
   * @param dataStructure Owns the group after insertion.
   * @param name Names the group.
   */
  ThrowingCheckedFlushGroup(DataStructure& dataStructure, std::string name)
  : DataGroup(dataStructure, std::move(name))
  {
  }

  /**
   * @brief Counts the checked call and throws the selected exception.
   * @return The configured result when no exception is selected.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    ++CheckedCalls;
    ThrowFlushFailure(Failure);
    return FlushResult;
  }

  FlushFailure Failure = FlushFailure::None;
  Result<> FlushResult;
  mutable usize CheckedCalls = 0;
};
} // namespace

TEST_CASE("CheckedFlush resident defaults succeed", "[simplnx][CheckedFlush]")
{
  DataStructure dataStructure;
  const DataStructure& constStructure = dataStructure;
  const auto emptyResult = constStructure.flushChecked();
  REQUIRE(emptyResult.valid());
  CHECK(emptyResult.warnings().empty());

  auto numericStore = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, int32{7});
  auto listStore = std::make_shared<ListStore<int32>>(ShapeType{2});
  listStore->setList(0, std::vector<int32>{11, 13});
  const AbstractDataStore<int32>& constNumericStore = *numericStore;
  const IListStore& constListStore = *listStore;
  REQUIRE(constNumericStore.flushChecked().valid());
  REQUIRE(constListStore.flushChecked().valid());

  const DataStore<bool> boolStore(ShapeType{2}, ShapeType{1}, true);
  REQUIRE(boolStore.flushChecked().valid());
  const auto* groupPtr = DataGroup::Create(dataStructure, "Group");
  REQUIRE(groupPtr != nullptr);
  const auto* arrayPtr = DataArray<int32>::Create(dataStructure, "Values", numericStore, groupPtr->getId());
  const auto* listPtr = NeighborList<int32>::Create(dataStructure, "Neighbors", listStore, groupPtr->getId());
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(listPtr != nullptr);
  REQUIRE(groupPtr->flushChecked().valid());
  REQUIRE(arrayPtr->flushChecked().valid());
  REQUIRE(listPtr->flushChecked().valid());

  const auto result = constStructure.flushChecked();
  REQUIRE(result.valid());
  CHECK(result.warnings().empty());
  CHECK(numericStore->getValue(0) == 7);
  CHECK(listStore->getList(0) == std::vector<int32>{11, 13});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CheckedFlush adapts legacy numeric exceptions", "[simplnx][CheckedFlush]")
{
  const auto [failure, expectedCode, detail] = GENERATE(Catch::Generators::table<FlushFailure, int32, const char*>(
      {{FlushFailure::None, 0, ""}, {FlushFailure::Standard, -6070, "backing.h5:/values"}, {FlushFailure::Unknown, -6070, "unknown"}, {FlushFailure::Allocation, -272, "alloc"}}));
  DYNAMIC_SECTION("Exception category " << static_cast<int>(failure))
  {
    LegacyFlushStore store;
    store.Failure = failure;
    const AbstractDataStore<int32>& abstractStore = store;
    Result<> result;
    REQUIRE_NOTHROW(result = abstractStore.flushChecked());
    CHECK(store.FlushCalls == 1);
    CHECK(result.warnings().empty());
    if(expectedCode == 0)
    {
      REQUIRE(result.valid());
    }
    else
    {
      REQUIRE(result.invalid());
      REQUIRE(result.errors().size() == 1);
      CHECK(result.errors()[0].code == expectedCode);
      CHECK(result.errors()[0].message.find(detail) != std::string::npos);
    }
  }
}

TEST_CASE("CheckedFlush adapts legacy object exceptions", "[simplnx][CheckedFlush]")
{
  const auto [failure, expectedCode, detail] = GENERATE(Catch::Generators::table<FlushFailure, int32, const char*>(
      {{FlushFailure::None, 0, ""}, {FlushFailure::Standard, -6070, "backing.h5:/values"}, {FlushFailure::Unknown, -6070, "unknown"}, {FlushFailure::Allocation, -272, "alloc"}}));
  DYNAMIC_SECTION("Exception category " << static_cast<int>(failure))
  {
    DataStructure dataStructure;
    auto object = std::make_shared<LegacyFlushGroup>(dataStructure, "LegacyObject");
    REQUIRE(dataStructure.insert(object, DataPath{}));
    object->Failure = failure;
    const DataObject& abstractObject = *object;
    Result<> result;
    REQUIRE_NOTHROW(result = abstractObject.flushChecked());
    CHECK(object->FlushCalls == 1);
    CHECK(result.warnings().empty());
    if(expectedCode == 0)
    {
      REQUIRE(result.valid());
    }
    else
    {
      REQUIRE(result.invalid());
      REQUIRE(result.errors().size() == 1);
      CHECK(result.errors()[0].code == expectedCode);
      CHECK(result.errors()[0].message.find(detail) != std::string::npos);
      CHECK(result.errors()[0].message.find("LegacyObject") != std::string::npos);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("CheckedFlush forwards numeric and list diagnostics once", "[simplnx][CheckedFlush]")
{
  const bool useList = GENERATE(false, true);
  const bool hasErrors = GENERATE(false, true);
  DYNAMIC_SECTION("List " << useList << ", errors " << hasErrors)
  {
    DataStructure dataStructure;
    auto numericStore = std::make_shared<ReportingFlushStore>();
    auto listStore = std::make_shared<ReportingFlushListStore>();
    const auto* arrayPtr = DataArray<int32>::Create(dataStructure, "Values", numericStore);
    const auto* listPtr = NeighborList<int32>::Create(dataStructure, "Neighbors", listStore);
    REQUIRE(arrayPtr != nullptr);
    REQUIRE(listPtr != nullptr);

    Result<> reportedResult;
    if(hasErrors)
    {
      reportedResult = MakeErrorResult(-19001, "backing.h5:/values: primary storage error");
      reportedResult.errors().push_back({-272, "backing.h5:/values: memory allocation failed during cleanup"});
    }
    reportedResult.warnings().push_back({19001, "backing.h5:/values: first storage warning"});
    reportedResult.warnings().push_back({19002, "backing.h5:/values: second storage warning"});
    numericStore->FlushResult = reportedResult;
    listStore->FlushResult = reportedResult;

    const DataObject& object = useList ? static_cast<const DataObject&>(*listPtr) : static_cast<const DataObject&>(*arrayPtr);
    const auto result = object.flushChecked();
    CHECK(numericStore->CheckedCalls == (useList ? 0 : 1));
    CHECK(listStore->CheckedCalls == (useList ? 1 : 0));
    CHECK(numericStore->LegacyCalls == 0);
    CHECK(result.invalid() == hasErrors);
    REQUIRE(result.warnings().size() == 2);
    CHECK(result.warnings()[0].code == 19001);
    CHECK(result.warnings()[0].message.find("first storage warning") != std::string::npos);
    CHECK(result.warnings()[1].code == 19002);
    CHECK(result.warnings()[1].message.find("second storage warning") != std::string::npos);
    if(hasErrors)
    {
      REQUIRE(result.invalid());
      REQUIRE(result.errors().size() == 2);
      CHECK(result.errors()[0].code == -19001);
      CHECK(result.errors()[0].message.find("backing.h5:/values: primary storage error") != std::string::npos);
      CHECK(result.errors()[1].code == -272);
      CHECK(result.errors()[1].message.find("memory allocation failed during cleanup") != std::string::npos);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("CheckedFlush visits each live object once and keeps all paths", "[simplnx][CheckedFlush]")
{
  DataStructure dataStructure;
  const auto* firstParentPtr = DataGroup::Create(dataStructure, "First");
  const auto* secondParentPtr = DataGroup::Create(dataStructure, "Second");
  REQUIRE(firstParentPtr != nullptr);
  REQUIRE(secondParentPtr != nullptr);
  auto firstStore = std::make_shared<ReportingFlushStore>();
  firstStore->FlushResult = MakeErrorResult(-19101, "backing.h5:/first: primary failure");
  firstStore->FlushResult.errors().push_back({-272, "backing.h5:/first: memory allocation failed during cleanup"});
  firstStore->FlushResult.warnings().push_back({19101, "first warning"});
  firstStore->FlushResult.warnings().push_back({19102, "second warning"});
  const auto* firstArrayPtr = DataArray<int32>::Create(dataStructure, "Values", firstStore, firstParentPtr->getId());
  REQUIRE(firstArrayPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(firstArrayPtr->getId(), secondParentPtr->getId()));
  REQUIRE(firstArrayPtr->getDataPaths().size() == 2);

  auto laterStore = std::make_shared<ReportingFlushStore>();
  laterStore->FlushResult = MakeErrorResult(-19103, "backing.h5:/later: later failure");
  laterStore->FlushResult.warnings().push_back({19103, "later warning"});
  const auto* laterArrayPtr = DataArray<int32>::Create(dataStructure, "ALater", laterStore);
  REQUIRE(laterArrayPtr != nullptr);
  REQUIRE(firstArrayPtr->getId() < laterArrayPtr->getId());

  const DataStructure& constStructure = dataStructure;
  const auto result = constStructure.flushChecked();
  REQUIRE(result.invalid());
  CHECK(firstStore->CheckedCalls == 1);
  CHECK(laterStore->CheckedCalls == 1);
  CHECK(firstStore->LegacyCalls == 0);
  CHECK(laterStore->LegacyCalls == 0);
  REQUIRE(result.errors().size() == 3);
  CHECK(result.errors()[0].code == -19101);
  CHECK(result.errors()[1].code == -272);
  CHECK(result.errors()[2].code == -19103);
  for(usize errorIdx = 0; errorIdx < 2; ++errorIdx)
  {
    CHECK(result.errors()[errorIdx].message.find("backing.h5:/first") != std::string::npos);
    CHECK(result.errors()[errorIdx].message.find("First/Values") != std::string::npos);
    CHECK(result.errors()[errorIdx].message.find("Second/Values") != std::string::npos);
    CHECK(result.errors()[errorIdx].message.find(std::to_string(firstArrayPtr->getId())) != std::string::npos);
  }
  CHECK(result.errors()[0].message.find("primary failure") != std::string::npos);
  CHECK(result.errors()[1].message.find("memory allocation failed during cleanup") != std::string::npos);
  CHECK(result.errors()[2].message.find("ALater") != std::string::npos);
  CHECK(result.errors()[2].message.find("backing.h5:/later: later failure") != std::string::npos);
  REQUIRE(result.warnings().size() == 3);
  CHECK(result.warnings()[0].code == 19101);
  CHECK(result.warnings()[0].message.find("first warning") != std::string::npos);
  CHECK(result.warnings()[1].code == 19102);
  CHECK(result.warnings()[1].message.find("second warning") != std::string::npos);
  CHECK(result.warnings()[2].code == 19103);
  CHECK(result.warnings()[2].message.find("later warning") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CheckedFlush forwarders preserve invalid results with no errors", "[simplnx][CheckedFlush]")
{
  const bool useList = GENERATE(false, true);
  DYNAMIC_SECTION("List " << useList)
  {
    DataStructure dataStructure;
    auto numericStore = std::make_shared<ReportingFlushStore>();
    auto listStore = std::make_shared<ReportingFlushListStore>();
    const auto* arrayPtr = DataArray<int32>::Create(dataStructure, "Values", numericStore);
    const auto* listPtr = NeighborList<int32>::Create(dataStructure, "Neighbors", listStore);
    REQUIRE(arrayPtr != nullptr);
    REQUIRE(listPtr != nullptr);
    Result<> emptyFailure;
    emptyFailure.m_Expected = nonstd::make_unexpected(ErrorCollection{});
    emptyFailure.warnings().push_back({19501, "backing.h5:/values: retained warning"});
    REQUIRE(emptyFailure.invalid());
    REQUIRE(emptyFailure.errors().empty());
    numericStore->FlushResult = emptyFailure;
    listStore->FlushResult = emptyFailure;

    const DataObject& object = useList ? static_cast<const DataObject&>(*listPtr) : static_cast<const DataObject&>(*arrayPtr);
    const auto result = object.flushChecked();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -6070);
    CHECK(result.errors()[0].message.find(useList ? "Neighbors" : "Values") != std::string::npos);
    REQUIRE(result.warnings().size() == 1);
    CHECK(result.warnings()[0].code == 19501);
    CHECK(result.warnings()[0].message.find("backing.h5:/values: retained warning") != std::string::npos);
    CHECK(numericStore->CheckedCalls == (useList ? 0 : 1));
    CHECK(listStore->CheckedCalls == (useList ? 1 : 0));
    CHECK(numericStore->LegacyCalls == 0);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("CheckedFlush traversal preserves invalid custom objects with no errors", "[simplnx][CheckedFlush]")
{
  DataStructure dataStructure;
  auto object = std::make_shared<ThrowingCheckedFlushGroup>(dataStructure, "EmptyFailure");
  object->FlushResult.m_Expected = nonstd::make_unexpected(ErrorCollection{});
  object->FlushResult.warnings().push_back({19601, "empty failure warning"});
  REQUIRE(object->FlushResult.invalid());
  REQUIRE(object->FlushResult.errors().empty());
  REQUIRE(dataStructure.insert(object, DataPath{}));
  auto laterStore = std::make_shared<ReportingFlushStore>();
  laterStore->FlushResult = MakeWarningVoidResult(19602, "later warning");
  const auto* laterArrayPtr = DataArray<int32>::Create(dataStructure, "Later", laterStore);
  REQUIRE(laterArrayPtr != nullptr);
  REQUIRE(object->getId() < laterArrayPtr->getId());

  const auto result = std::as_const(dataStructure).flushChecked();
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -6070);
  CHECK(result.errors()[0].message.find("EmptyFailure") != std::string::npos);
  CHECK(result.errors()[0].message.find(std::to_string(object->getId())) != std::string::npos);
  REQUIRE(result.warnings().size() == 2);
  CHECK(result.warnings()[0].code == 19601);
  CHECK(result.warnings()[0].message.find("empty failure warning") != std::string::npos);
  CHECK(result.warnings()[1].code == 19602);
  CHECK(result.warnings()[1].message.find("later warning") != std::string::npos);
  CHECK(object->CheckedCalls == 1);
  CHECK(laterStore->CheckedCalls == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CheckedFlush calls a shared store once for each distinct object", "[simplnx][CheckedFlush]")
{
  DataStructure dataStructure;
  auto store = std::make_shared<ReportingFlushStore>();
  store->FlushResult = MakeWarningVoidResult(19201, "shared backing warning");
  REQUIRE(DataArray<int32>::Create(dataStructure, "First", store) != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Second", store) != nullptr);
  const auto result = std::as_const(dataStructure).flushChecked();
  REQUIRE(result.valid());
  CHECK(store->CheckedCalls == 2);
  CHECK(store->LegacyCalls == 0);
  REQUIRE(result.warnings().size() == 2);
  CHECK(result.warnings()[0].code == 19201);
  CHECK(result.warnings()[1].code == 19201);
  CHECK(result.warnings()[0].message.find("shared backing warning") != std::string::npos);
  CHECK(result.warnings()[1].message.find("shared backing warning") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CheckedFlush retains earlier diagnostics and continues after custom checked exceptions", "[simplnx][CheckedFlush]")
{
  const auto [failure, expectedCode, detail] = GENERATE(Catch::Generators::table<FlushFailure, int32, const char*>(
      {{FlushFailure::Standard, -6070, "backing.h5:/values"}, {FlushFailure::Unknown, -6070, "unknown"}, {FlushFailure::Allocation, -272, "alloc"}}));
  DYNAMIC_SECTION("Exception category " << static_cast<int>(failure))
  {
    DataStructure dataStructure;
    auto firstStore = std::make_shared<ReportingFlushStore>();
    firstStore->FlushResult = MakeErrorResult(-19301, "first ordinary failure");
    firstStore->FlushResult.warnings().push_back({19301, "first warning"});
    const auto* firstArrayPtr = DataArray<int32>::Create(dataStructure, "First", firstStore);
    REQUIRE(firstArrayPtr != nullptr);
    auto throwingObject = std::make_shared<ThrowingCheckedFlushGroup>(dataStructure, "ThrowingObject");
    REQUIRE(dataStructure.insert(throwingObject, DataPath{}));
    throwingObject->Failure = failure;
    auto laterStore = std::make_shared<ReportingFlushStore>();
    laterStore->FlushResult = MakeErrorResult(-19302, "later ordinary failure");
    laterStore->FlushResult.warnings().push_back({19302, "later warning"});
    const auto* laterArrayPtr = DataArray<int32>::Create(dataStructure, "Later", laterStore);
    REQUIRE(laterArrayPtr != nullptr);
    REQUIRE(firstArrayPtr->getId() < throwingObject->getId());
    REQUIRE(throwingObject->getId() < laterArrayPtr->getId());

    Result<> result;
    REQUIRE_NOTHROW(result = std::as_const(dataStructure).flushChecked());
    REQUIRE(result.invalid());
    CHECK(firstStore->CheckedCalls == 1);
    CHECK(throwingObject->CheckedCalls == 1);
    CHECK(laterStore->CheckedCalls == 1);
    REQUIRE(result.errors().size() == 3);
    CHECK(result.errors()[0].code == -19301);
    CHECK(result.errors()[0].message.find("first ordinary failure") != std::string::npos);
    CHECK(result.errors()[1].code == expectedCode);
    CHECK(result.errors()[1].message.find(detail) != std::string::npos);
    CHECK(result.errors()[1].message.find("ThrowingObject") != std::string::npos);
    CHECK(result.errors()[1].message.find(std::to_string(throwingObject->getId())) != std::string::npos);
    CHECK(result.errors()[2].code == -19302);
    CHECK(result.errors()[2].message.find("later ordinary failure") != std::string::npos);
    REQUIRE(result.warnings().size() == 2);
    CHECK(result.warnings()[0].code == 19301);
    CHECK(result.warnings()[0].message.find("first warning") != std::string::npos);
    CHECK(result.warnings()[1].code == 19302);
    CHECK(result.warnings()[1].message.find("later warning") != std::string::npos);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("CheckedFlush reports a moved-from numeric store without changing object identity", "[simplnx][CheckedFlush]")
{
  DataStructure dataStructure;
  auto sourceStore = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, int32{7});
  auto destinationStore = std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, int32{11});
  auto* sourcePtr = DataArray<int32>::Create(dataStructure, "Source", sourceStore);
  auto* destinationPtr = DataArray<int32>::Create(dataStructure, "Destination", destinationStore);
  REQUIRE(sourcePtr != nullptr);
  REQUIRE(destinationPtr != nullptr);
  const auto sourceId = sourcePtr->getId();
  const auto destinationId = destinationPtr->getId();

  *destinationPtr = std::move(*sourcePtr);
  REQUIRE(sourcePtr->getDataStore() == nullptr);
  REQUIRE(destinationPtr->getDataStore() == sourceStore.get());
  CHECK(sourcePtr->getId() == sourceId);
  CHECK(destinationPtr->getId() == destinationId);
  CHECK(dataStructure.getData(DataPath({"Source"})) == sourcePtr);
  CHECK(dataStructure.getData(DataPath({"Destination"})) == destinationPtr);
  const auto directResult = std::as_const(*sourcePtr).flushChecked();
  REQUIRE(directResult.invalid());
  REQUIRE_FALSE(directResult.errors().empty());
  CHECK(directResult.errors()[0].message.find("Source") != std::string::npos);
  const auto result = std::as_const(dataStructure).flushChecked();
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].message.find("Source") != std::string::npos);
  CHECK(sourceStore->getValue(0) == 7);

  REQUIRE(sourcePtr->setDataStore(sourceStore).valid());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CheckedFlush reports a missing list store from the public setter", "[simplnx][CheckedFlush]")
{
  DataStructure dataStructure;
  auto store = std::make_shared<ListStore<int32>>(ShapeType{2});
  auto* listPtr = NeighborList<int32>::Create(dataStructure, "Neighbors", store);
  REQUIRE(listPtr != nullptr);
  listPtr->setStore(std::shared_ptr<AbstractListStore<int32>>{});
  REQUIRE(listPtr->getIListStore() == nullptr);
  const auto directResult = std::as_const(*listPtr).flushChecked();
  REQUIRE(directResult.invalid());
  REQUIRE_FALSE(directResult.errors().empty());
  CHECK(directResult.errors()[0].message.find("Neighbors") != std::string::npos);
  const auto result = std::as_const(dataStructure).flushChecked();
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].message.find("Neighbors") != std::string::npos);

  listPtr->setStore(store);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("CheckedFlush preserves legacy void failure behavior", "[simplnx][CheckedFlush]")
{
  const auto failure = GENERATE(FlushFailure::Standard, FlushFailure::Unknown, FlushFailure::Allocation);
  DYNAMIC_SECTION("Exception category " << static_cast<int>(failure))
  {
    DataStructure dataStructure;
    auto firstStore = std::make_shared<LegacyFlushStore>();
    firstStore->Failure = failure;
    auto laterStore = std::make_shared<ReportingFlushStore>();
    REQUIRE(DataArray<int32>::Create(dataStructure, "First", firstStore) != nullptr);
    REQUIRE(DataArray<int32>::Create(dataStructure, "Later", laterStore) != nullptr);
    const DataStructure& constStructure = dataStructure;
    switch(failure)
    {
    case FlushFailure::Standard:
      REQUIRE_THROWS_AS(constStructure.flush(), std::runtime_error);
      break;
    case FlushFailure::Unknown:
      REQUIRE_THROWS_AS(constStructure.flush(), int);
      break;
    case FlushFailure::Allocation:
      REQUIRE_THROWS_AS(constStructure.flush(), std::bad_alloc);
      break;
    case FlushFailure::None:
      FAIL("The legacy failure test requires an exception.");
    }
    CHECK(firstStore->FlushCalls == 1);
    CHECK(laterStore->LegacyCalls == 0);
    CHECK(laterStore->CheckedCalls == 0);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("CheckedFlush leaves legacy success separate from checked overrides", "[simplnx][CheckedFlush]")
{
  DataStructure dataStructure;
  auto numericStore = std::make_shared<ReportingFlushStore>();
  numericStore->FlushResult = MakeErrorResult(-19401, "checked numeric failure");
  auto listStore = std::make_shared<ReportingFlushListStore>();
  listStore->FlushResult = MakeErrorResult(-19402, "checked list failure");
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", numericStore) != nullptr);
  REQUIRE(NeighborList<int32>::Create(dataStructure, "Neighbors", listStore) != nullptr);
  REQUIRE_NOTHROW(std::as_const(dataStructure).flush());
  CHECK(numericStore->LegacyCalls == 1);
  CHECK(numericStore->CheckedCalls == 0);
  CHECK(listStore->CheckedCalls == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
