#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/Observers/AbstractDataStructureObserver.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
constexpr int32 k_FilterError = -76101;
constexpr int32 k_FlushError = -76102;
constexpr int32 k_FilterWarning = 76103;
constexpr int32 k_FlushWarning = 76104;
const DataPath k_ValuesPath({"Group", "Values"});
const DataPath k_ListPath({"Group", "Neighbors"});

/**
 * @enum StoreKind
 * @brief Selects the public store adapter under test.
 */
enum class StoreKind
{
  Numeric, ///< Uses a DataArray with resident values.
  List     ///< Uses a NeighborList with resident lists.
};

/**
 * @enum SnapshotFailure
 * @brief Selects a copy exception while diagnostic memory is available.
 */
enum class SnapshotFailure
{
  None,       ///< Permits shallow copies.
  Standard,   ///< Throws a runtime error.
  Allocation, ///< Throws bad_alloc without exhausting memory.
  Unknown     ///< Throws a nonstandard exception.
};

/**
 * @struct FlushControl
 * @brief Retains the same diagnostic state across shallow snapshots and retries.
 */
struct FlushControl
{
  Result<> result;
  usize checkedCalls = 0;
  usize legacyCalls = 0;
  std::atomic_bool* cancelOnFlush = nullptr;
};

/**
 * @class NumericCompletionStore
 * @brief Separates legacy calls from checked calls at the pipeline boundary.
 */
class NumericCompletionStore : public DataStore<int32>
{
public:
  /**
   * @brief Creates two literal values and retains the reporting control.
   * @param control Owns counters and selected diagnostics.
   */
  explicit NumericCompletionStore(std::shared_ptr<FlushControl> control)
  : DataStore<int32>(ShapeType{2}, ShapeType{1}, int32{0})
  , m_Control(std::move(control))
  {
    setValue(0, 11);
    setValue(1, 13);
  }

  /**
   * @brief Counts a legacy call without returning checked diagnostics.
   */
  void flush() const override
  {
    ++m_Control->legacyCalls;
    if(m_Control->cancelOnFlush != nullptr)
    {
      m_Control->cancelOnFlush->store(true);
    }
  }

  /**
   * @brief Returns diagnostics and can request cancellation after execute.
   * @return The configured completion result.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    ++m_Control->checkedCalls;
    if(m_Control->cancelOnFlush != nullptr)
    {
      m_Control->cancelOnFlush->store(true);
    }
    return m_Control->result;
  }

private:
  std::shared_ptr<FlushControl> m_Control;
};

/**
 * @class ListCompletionStore
 * @brief Returns checked diagnostics through a real public NeighborList.
 */
class ListCompletionStore : public ListStore<int32>
{
public:
  /**
   * @brief Creates two literal lists and retains the reporting control.
   * @param control Owns counters and selected diagnostics.
   */
  explicit ListCompletionStore(std::shared_ptr<FlushControl> control)
  : ListStore<int32>(ShapeType{2})
  , m_Control(std::move(control))
  {
    setList(0, std::vector<int32>{11, 13});
    setList(1, std::vector<int32>{17});
  }

  /**
   * @brief Returns the selected list completion diagnostics.
   * @return The configured completion result.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    ++m_Control->checkedCalls;
    return m_Control->result;
  }

private:
  std::shared_ptr<FlushControl> m_Control;
};

/**
 * @struct FilterControl
 * @brief Retains execute results and hooks across filter clones.
 */
struct FilterControl
{
  Result<> result;
  usize executeCalls = 0;
  bool throwAllocation = false;
  bool invalidEmptyPreflight = false;
  std::function<void()> onExecute;
};

/**
 * @class CompletionFilter
 * @brief Exercises normal IFilter execution without private backend dependencies.
 */
class CompletionFilter : public IFilter
{
public:
  /**
   * @brief Retains the selected execute behavior.
   * @param control Owns the result, call count, and optional hook.
   */
  explicit CompletionFilter(std::shared_ptr<FilterControl> control)
  : m_Control(std::move(control))
  {
  }

  std::string name() const override
  {
    return "CompletionFilter";
  }
  std::string className() const override
  {
    return "CompletionFilter";
  }
  Uuid uuid() const override
  {
    return *Uuid::FromString("82a26d85-21a7-429a-a208-a5f0ba24a6bb");
  }
  std::string humanName() const override
  {
    return "Completion Filter";
  }
  VersionType parametersVersion() const override
  {
    return 1;
  }

  /**
   * @brief Uses no parameters so validation cannot select another failure phase.
   * @return An empty parameter set.
   */
  Parameters parameters() const override
  {
    return {};
  }

  /**
   * @brief Shares the same control with the cloned filter.
   * @return A new filter with the existing control.
   */
  UniquePointer clone() const override
  {
    return std::make_unique<CompletionFilter>(m_Control);
  }

protected:
  /**
   * @brief Supplies an output-value sentinel without changing the input structure.
   * @param dataStructure Provides existing test objects.
   * @param filterArgs Contains no parameters.
   * @param messageHandler Is unused by this fixture.
   * @param shouldCancel Is checked by the public IFilter wrapper.
   * @param executionContext Is supplied by the real parent pipeline.
   * @return Successful actions and a literal output value.
   */
  PreflightResult preflightImpl([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const Arguments& filterArgs, [[maybe_unused]] const MessageHandler& messageHandler,
                                [[maybe_unused]] const std::atomic_bool& shouldCancel, [[maybe_unused]] const ExecutionContext& executionContext) const override
  {
    PreflightResult result{};
    result.outputValues.push_back({"Completion value", "preserved"});
    if(m_Control->invalidEmptyPreflight)
    {
      result.outputActions.m_Expected = nonstd::make_unexpected(ErrorCollection{});
      result.outputActions.warnings().push_back({k_FilterWarning, "invalid-empty-original-warning"});
    }
    return result;
  }

  /**
   * @brief Runs the optional phase hook and returns selected execute diagnostics.
   * @param dataStructure Keeps the same objects for the completion boundary.
   * @param filterArgs Contains no parameters.
   * @param pipelineNode Identifies the real parented node.
   * @param messageHandler Is unused by this fixture.
   * @param shouldCancel Is checked by the public IFilter wrapper.
   * @param executionContext Is supplied by the real parent pipeline.
   * @return The configured execute result.
   * @throws std::bad_alloc If the synthetic execute allocation fault is enabled.
   */
  Result<> executeImpl([[maybe_unused]] DataStructure& dataStructure, [[maybe_unused]] const Arguments& filterArgs, [[maybe_unused]] const PipelineFilter* pipelineNode,
                       [[maybe_unused]] const MessageHandler& messageHandler, [[maybe_unused]] const std::atomic_bool& shouldCancel,
                       [[maybe_unused]] const ExecutionContext& executionContext) const override
  {
    ++m_Control->executeCalls;
    if(m_Control->onExecute)
    {
      m_Control->onExecute();
    }
    if(m_Control->throwAllocation)
    {
      throw std::bad_alloc{};
    }
    return m_Control->result;
  }

private:
  std::shared_ptr<FilterControl> m_Control;
};

/**
 * @class InterfaceCompletionNode
 * @brief Distinguishes the additive checked virtual from the legacy virtual.
 */
class InterfaceCompletionNode : public PipelineFilter
{
public:
  using PipelineFilter::PipelineFilter;

  bool overrideCompletion = false;
  Result<> completionResult;
  usize checkedCalls = 0;
  usize legacyCalls = 0;

  /**
   * @brief Exposes the retained legacy contract for a direct compatibility check.
   * @param dataStructure Supplies the original data.
   */
  void completeLegacy(DataStructure& dataStructure)
  {
    endExecution(dataStructure);
  }

protected:
  /**
   * @brief Counts checked completion and optionally returns an extension result.
   * @param dataStructure Supplies the actual pipeline data.
   * @return Configured extension diagnostics or the base completion result.
   */
  Result<> endExecutionChecked(DataStructure& dataStructure) override
  {
    ++checkedCalls;
    return overrideCompletion ? completionResult : AbstractPipelineNode::endExecutionChecked(dataStructure);
  }

  /**
   * @brief Counts the legacy virtual while retaining its original flush contract.
   * @param dataStructure Supplies data to flush and snapshot.
   */
  void endExecution(DataStructure& dataStructure) override
  {
    ++legacyCalls;
    AbstractPipelineNode::endExecution(dataStructure);
  }
};

/**
 * @class RemovalWitness
 * @brief Records existing temporary-copy notifications without throwing or changing the source.
 */
class RemovalWitness : public AbstractDataStructureObserver
{
public:
  usize liveRemovalCount = 0;
  DataObject::IdType lastLiveId = 0;

  /**
   * @brief Counts removal messages whose original object remains present.
   * @param target Supplies the source ID lookup.
   * @param message Identifies the destroyed temporary clone.
   */
  void onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& message) noexcept override
  {
    const auto* removed = dynamic_cast<const DataRemovedMessage*>(message.get());
    if(removed != nullptr && target->getData(removed->getId()) != nullptr)
    {
      ++liveRemovalCount;
      lastLiveId = removed->getId();
    }
  }
};

/**
 * @struct SnapshotControl
 * @brief Arms copy faults without throwing from destructors or observers.
 */
struct SnapshotControl
{
  SnapshotFailure failure = SnapshotFailure::None;
  usize copyCalls = 0;
};

/**
 * @class SnapshotGroup
 * @brief Preserves a shared copy-fault control in each successful shallow copy.
 */
class SnapshotGroup : public DataGroup
{
public:
  /**
   * @brief Creates an insertable group after earlier normal test objects.
   * @param dataStructure Owns the group after insertion.
   * @param control Selects the copy failure.
   */
  SnapshotGroup(DataStructure& dataStructure, std::shared_ptr<SnapshotControl> control)
  : DataGroup(dataStructure, "CopyFault")
  , m_Control(std::move(control))
  {
  }

  /**
   * @brief Copies the group topology and shares only the fault control.
   * @param other Supplies identity and the shared control.
   */
  SnapshotGroup(const SnapshotGroup& other)
  : DataGroup(other)
  , m_Control(other.m_Control)
  {
  }

  /**
   * @brief Copies the object or throws the selected synthetic exception.
   * @return A caller-owned shallow copy.
   * @throws std::runtime_error If an ordinary copy fault is armed.
   * @throws std::bad_alloc If an allocation copy fault is armed.
   * @throws int If a nonstandard copy fault is armed.
   */
  DataObject* shallowCopy() override
  {
    ++m_Control->copyCalls;
    switch(m_Control->failure)
    {
    case SnapshotFailure::Standard:
      throw std::runtime_error("copy-sentinel: shallow snapshot failed");
    case SnapshotFailure::Allocation:
      throw std::bad_alloc{};
    case SnapshotFailure::Unknown:
      throw 41;
    case SnapshotFailure::None:
      return new SnapshotGroup(*this);
    }
    throw std::logic_error("Unknown snapshot test mode");
  }

private:
  std::shared_ptr<SnapshotControl> m_Control;
};

/**
 * @struct DetailRecord
 * @brief Captures the emitted collection order independently of GUI sorting.
 */
struct DetailRecord
{
  int32 index = 0;
  WarningCollection warnings;
  ErrorCollection errors;
};

/**
 * @struct CompletionTrace
 * @brief Owns signal connections only while the referenced pipeline is alive.
 */
struct CompletionTrace
{
  /**
   * @brief Observes a root pipeline and an optional first filter.
   * @param pipeline Emits root completion and cancellation events.
   * @param firstNode Emits leaf details, state, and updates when nonnull.
   */
  CompletionTrace(Pipeline& pipeline, PipelineFilter* firstNode)
  {
    connections.push_back(pipeline.getPipelineRunStateSignal().connect([this](AbstractPipelineNode*, RunState state) { pipelineStates.push_back(state); }));
    connections.push_back(pipeline.getCancelledSignal().connect([this]() { ++cancelledCalls; }));
    connections.push_back(pipeline.getFilterFaultDetailSignal().connect(
        [this](AbstractPipelineNode*, int32 index, const WarningCollection& warnings, const ErrorCollection& errors) { rootDetails.push_back({index, warnings, errors}); }));
    if(firstNode != nullptr)
    {
      connections.push_back(firstNode->getFilterRunStateSignal().connect([this](AbstractPipelineNode*, int32, RunState state) { nodeStates.push_back(state); }));
      connections.push_back(firstNode->getFilterUpdateSignal().connect([this](AbstractPipelineNode*, int32, const std::string& message) { updates.push_back(message); }));
      connections.push_back(firstNode->getFilterFaultDetailSignal().connect(
          [this](AbstractPipelineNode*, int32 index, const WarningCollection& warnings, const ErrorCollection& errors) { nodeDetails.push_back({index, warnings, errors}); }));
    }
  }

  std::vector<RunState> pipelineStates;
  std::vector<RunState> nodeStates;
  std::vector<std::string> updates;
  std::vector<DetailRecord> nodeDetails;
  std::vector<DetailRecord> rootDetails;
  usize cancelledCalls = 0;
  std::vector<nod::scoped_connection> connections;
};

/**
 * @struct CompletionFixture
 * @brief Keeps source objects and their reporting stores alive through retries.
 */
struct CompletionFixture
{
  /**
   * @brief Creates literal input and properly parented real pipeline nodes.
   * @param kind Selects the reporting numeric or list adapter.
   * @param includeSecond Adds the later-filter execution witness when true.
   */
  explicit CompletionFixture(StoreKind kind, bool includeSecond = true)
  : storeKind(kind)
  {
    UnitTest::LoadPlugins();
    const auto* group = DataGroup::Create(dataStructure, "Group");
    REQUIRE(group != nullptr);
    if(kind == StoreKind::Numeric)
    {
      numericStore = std::make_shared<NumericCompletionStore>(flushControl);
      REQUIRE(DataArray<int32>::Create(dataStructure, "Values", numericStore, group->getId()) != nullptr);
    }
    else
    {
      listStore = std::make_shared<ListCompletionStore>(flushControl);
      REQUIRE(NeighborList<int32>::Create(dataStructure, "Neighbors", listStore, group->getId()) != nullptr);
    }
    REQUIRE(pipeline.push_back(std::make_unique<CompletionFilter>(firstControl), Arguments{}));
    firstNode = dynamic_cast<PipelineFilter*>(pipeline.at(0));
    REQUIRE(firstNode != nullptr);
    firstNode->setIndex(0);
    if(includeSecond)
    {
      REQUIRE(pipeline.push_back(std::make_unique<CompletionFilter>(secondControl), Arguments{}));
      auto* secondNode = dynamic_cast<PipelineFilter*>(pipeline.at(1));
      REQUIRE(secondNode != nullptr);
      secondNode->setIndex(1);
    }
  }

  StoreKind storeKind;
  DataStructure dataStructure;
  Pipeline pipeline;
  std::shared_ptr<FlushControl> flushControl = std::make_shared<FlushControl>();
  std::shared_ptr<FilterControl> firstControl = std::make_shared<FilterControl>();
  std::shared_ptr<FilterControl> secondControl = std::make_shared<FilterControl>();
  std::shared_ptr<NumericCompletionStore> numericStore;
  std::shared_ptr<ListCompletionStore> listStore;
  PipelineFilter* firstNode = nullptr;
};

/**
 * @brief Checks literal contents and shared storage in a completed shallow snapshot.
 * @param fixture Owns the source structure and original stores.
 * @param snapshot Supplies the captured object topology.
 */
void CheckSnapshot(const CompletionFixture& fixture, const DataStructure& snapshot)
{
  const DataPath& path = fixture.storeKind == StoreKind::Numeric ? k_ValuesPath : k_ListPath;
  const auto* sourceObject = fixture.dataStructure.getData(path);
  const auto* copiedObject = snapshot.getData(path);
  REQUIRE(sourceObject != nullptr);
  REQUIRE(copiedObject != nullptr);
  CHECK(copiedObject != sourceObject);
  CHECK(copiedObject->getId() == sourceObject->getId());
  CHECK(copiedObject->getParentIds() == sourceObject->getParentIds());
  CHECK(copiedObject->getDataStructure() == &snapshot);
  CHECK(sourceObject->getDataStructure() == &fixture.dataStructure);
  CHECK(snapshot.getNextId() == fixture.dataStructure.getNextId());
  CHECK(&snapshot.formatResolver() == &fixture.dataStructure.formatResolver());
  if(fixture.storeKind == StoreKind::Numeric)
  {
    const auto* array = snapshot.getDataAs<DataArray<int32>>(path);
    REQUIRE(array != nullptr);
    CHECK(&array->getDataStoreRef() == fixture.numericStore.get());
    CHECK(array->getDataStoreRef().getValue(0) == 11);
    CHECK(array->getDataStoreRef().getValue(1) == 13);
  }
  else
  {
    const auto* lists = snapshot.getDataAs<NeighborList<int32>>(path);
    REQUIRE(lists != nullptr);
    CHECK(lists->getIListStore() == fixture.listStore.get());
    CHECK(lists->getList(0) == std::vector<int32>{11, 13});
    CHECK(lists->getList(1) == std::vector<int32>{17});
  }
}

/**
 * @brief Checks normal terminal signals and the retained output-value sentinel.
 * @param trace Contains events from the completed or failed execution.
 * @param firstNode Owns the execute output values.
 */
void CheckNodeCompletion(const CompletionTrace& trace, const PipelineFilter& firstNode)
{
  REQUIRE_FALSE(trace.nodeStates.empty());
  CHECK(trace.nodeStates.back() == RunState::Idle);
  REQUIRE_FALSE(trace.pipelineStates.empty());
  CHECK(trace.pipelineStates.back() == RunState::Idle);
  REQUIRE_FALSE(trace.updates.empty());
  CHECK(trace.updates.back() == "End");
  const auto& values = firstNode.getPreflightValues();
  REQUIRE(values.size() == 1);
  CHECK(values[0].name == "Completion value");
  CHECK(values[0].value == "preserved");
}

/**
 * @brief Installs the copy-fault group after the normal literal objects.
 * @param fixture Owns the source structure.
 * @return Shared control for the inserted group and its shallow copies.
 */
std::shared_ptr<SnapshotControl> AddSnapshotGroup(CompletionFixture& fixture)
{
  auto control = std::make_shared<SnapshotControl>();
  auto group = std::make_shared<SnapshotGroup>(fixture.dataStructure, control);
  REQUIRE(fixture.dataStructure.insert(group, DataPath{}));
  return control;
}
} // namespace

TEST_CASE("PipelineFlush retains execute diagnostics and fails before later filters", "[simplnx][PipelineFlush]")
{
  const auto kind = GENERATE(StoreKind::Numeric, StoreKind::List);
  const bool originalError = GENERATE(false, true);
  DYNAMIC_SECTION("Store " << static_cast<int>(kind) << " original error " << originalError)
  {
    CompletionFixture fixture(kind);
    fixture.firstControl->result.warnings().push_back({k_FilterWarning, "execute-warning-sentinel"});
    if(originalError)
    {
      fixture.firstControl->result.m_Expected = nonstd::make_unexpected(ErrorCollection{{k_FilterError, "execute-error-sentinel"}});
    }
    fixture.flushControl->result = MakeErrorResult(k_FlushError, "backing.h5:/values: terminal-flush-sentinel");
    fixture.flushControl->result.warnings().push_back({k_FlushWarning, "flush-warning-sentinel"});
    CompletionTrace trace(fixture.pipeline, fixture.firstNode);
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
    CHECK_FALSE(succeeded);
    CHECK(fixture.firstControl->executeCalls == 1);
    CHECK(fixture.secondControl->executeCalls == 0);
    CHECK(fixture.flushControl->checkedCalls == 1);
    CHECK(fixture.flushControl->legacyCalls == 0);
    CHECK(fixture.firstNode->hasErrors());
    CHECK_FALSE(fixture.firstNode->hasWarnings());
    CHECK(fixture.pipeline.hasErrors());
    CHECK_FALSE(fixture.pipeline.hasWarnings());

    const auto errors = fixture.firstNode->getErrors();
    REQUIRE(errors.size() == (originalError ? 2 : 1));
    if(originalError)
    {
      CHECK(errors.front().code == k_FilterError);
      CHECK(errors.front().message == "execute-error-sentinel");
    }
    CHECK(errors.back().code == k_FlushError);
    CHECK(errors.back().message.find("backing.h5:/values") != std::string::npos);
    CHECK(errors.back().message.find(kind == StoreKind::Numeric ? "Values" : "Neighbors") != std::string::npos);
    const auto warnings = fixture.firstNode->getWarnings();
    REQUIRE(warnings.size() == 2);
    CHECK(warnings[0].code == k_FilterWarning);
    CHECK(warnings[1].code == k_FlushWarning);
    REQUIRE(trace.nodeDetails.size() == 1);
    CHECK(trace.nodeDetails[0].index == 0);
    REQUIRE(trace.nodeDetails[0].errors.size() == errors.size());
    for(usize index = 0; index < errors.size(); ++index)
    {
      CHECK(trace.nodeDetails[0].errors[index].code == errors[index].code);
      CHECK(trace.nodeDetails[0].errors[index].message == errors[index].message);
    }
    CHECK(trace.rootDetails.empty());
    CheckNodeCompletion(trace, *fixture.firstNode);
    CheckSnapshot(fixture, fixture.firstNode->getDataStructure());
    CheckSnapshot(fixture, fixture.pipeline.getDataStructure());
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("PipelineFlush retries the same stores and clears completion faults", "[simplnx][PipelineFlush]")
{
  const auto kind = GENERATE(StoreKind::Numeric, StoreKind::List);
  DYNAMIC_SECTION("Store " << static_cast<int>(kind))
  {
    CompletionFixture fixture(kind);
    fixture.flushControl->result = MakeErrorResult(k_FlushError, "retry-flush-sentinel");
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
    CHECK_FALSE(succeeded);
    CHECK(fixture.secondControl->executeCalls == 0);
    fixture.flushControl->result = {};
    CompletionTrace retryTrace(fixture.pipeline, fixture.firstNode);
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
    CHECK(succeeded);
    CHECK(fixture.firstControl->executeCalls == 2);
    CHECK(fixture.secondControl->executeCalls == 1);
    CHECK(fixture.flushControl->checkedCalls == 3);
    CHECK(fixture.flushControl->legacyCalls == 0);
    CHECK_FALSE(fixture.firstNode->hasErrors());
    CHECK_FALSE(fixture.pipeline.hasErrors());
    CHECK(fixture.firstNode->getErrors().empty());
    CHECK(retryTrace.nodeDetails.empty());
    CHECK(retryTrace.rootDetails.empty());
    CheckNodeCompletion(retryTrace, *fixture.firstNode);
    CheckSnapshot(fixture, fixture.firstNode->getDataStructure());
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("PipelineFlush warning survives a later clean completion", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric);
  fixture.flushControl->result = MakeWarningVoidResult(k_FlushWarning, "early-warning-sentinel");
  fixture.secondControl->onExecute = [&fixture]() { fixture.flushControl->result = {}; };
  CompletionTrace trace(fixture.pipeline, fixture.firstNode);
  bool succeeded = false;
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
  CHECK(succeeded);
  CHECK(fixture.secondControl->executeCalls == 1);
  CHECK(fixture.firstNode->hasWarnings());
  CHECK(fixture.pipeline.hasWarnings());
  CHECK_FALSE(fixture.pipeline.hasErrors());
  REQUIRE(trace.nodeDetails.size() == 1);
  REQUIRE(trace.nodeDetails[0].warnings.size() == 1);
  CHECK(trace.nodeDetails[0].warnings[0].code == k_FlushWarning);
  CHECK(trace.nodeDetails[0].errors.empty());
  CHECK(trace.rootDetails.empty());
  CheckNodeCompletion(trace, *fixture.firstNode);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

TEST_CASE("PipelineFlush preserves allocation classification and earlier errors", "[simplnx][PipelineFlush]")
{
  const bool allocationInExecute = GENERATE(false, true);
  DYNAMIC_SECTION("Allocation in execute " << allocationInExecute)
  {
    CompletionFixture fixture(StoreKind::Numeric);
    fixture.firstControl->throwAllocation = allocationInExecute;
    if(!allocationInExecute)
    {
      fixture.firstControl->result = MakeErrorResult(k_FilterError, "original-before-allocation");
    }
    fixture.flushControl->result = MakeErrorResult(allocationInExecute ? k_FlushError : -272, "completion-allocation-routing");
    CompletionTrace trace(fixture.pipeline, fixture.firstNode);
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
    CHECK_FALSE(succeeded);
    CHECK(fixture.secondControl->executeCalls == 0);
    const auto errors = fixture.firstNode->getErrors();
    REQUIRE(errors.size() == 2);
    CHECK(errors[0].code == (allocationInExecute ? -272 : k_FilterError));
    CHECK(errors[1].code == (allocationInExecute ? k_FlushError : -272));
    CheckNodeCompletion(trace, *fixture.firstNode);
    CheckSnapshot(fixture, fixture.firstNode->getDataStructure());
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("PipelineFlush cancellation at the flush boundary retains completion failure", "[simplnx][PipelineFlush]")
{
  const bool failFlush = GENERATE(false, true);
  DYNAMIC_SECTION("Flush fails " << failFlush)
  {
    CompletionFixture fixture(StoreKind::Numeric);
    std::atomic_bool cancelled = false;
    fixture.flushControl->cancelOnFlush = &cancelled;
    if(failFlush)
    {
      fixture.flushControl->result = MakeErrorResult(k_FlushError, "flush-after-cancel-sentinel");
    }
    CompletionTrace trace(fixture.pipeline, fixture.firstNode);
    bool succeeded = !failFlush;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, cancelled));
    CHECK(succeeded == !failFlush);
    CHECK(cancelled.load());
    CHECK(trace.cancelledCalls == 1);
    CHECK(fixture.secondControl->executeCalls == 0);
    CHECK(fixture.pipeline.hasErrors() == failFlush);
    if(failFlush)
    {
      const auto errors = fixture.firstNode->getErrors();
      REQUIRE(errors.size() == 1);
      CHECK(errors[0].code == k_FlushError);
    }
    CheckNodeCompletion(trace, *fixture.firstNode);
    fixture.flushControl->cancelOnFlush = nullptr;
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("PipelineFlush leaves execute-stage cancellation behavior unchanged", "[simplnx][PipelineFlush]")
{
  const bool originalError = GENERATE(false, true);
  DYNAMIC_SECTION("Original execute error " << originalError)
  {
    CompletionFixture fixture(StoreKind::Numeric);
    std::atomic_bool cancelled = false;
    fixture.firstControl->onExecute = [&cancelled]() { cancelled.store(true); };
    if(originalError)
    {
      fixture.firstControl->result = MakeErrorResult(k_FilterError, "execute-cancel-error-sentinel");
    }
    CompletionTrace trace(fixture.pipeline, fixture.firstNode);
    bool succeeded = false;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, cancelled));
    CHECK(succeeded);
    CHECK_FALSE(fixture.pipeline.hasErrors());
    CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
    CHECK_FALSE(fixture.firstNode->hasCompletionErrors());
    CHECK(trace.cancelledCalls == 1);
    CHECK(fixture.secondControl->executeCalls == 0);
    const auto errors = fixture.firstNode->getErrors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].code == (originalError ? k_FilterError : -1));
    CheckNodeCompletion(trace, *fixture.firstNode);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("PipelineFlush repeated snapshot exceptions retain diagnostics and settle", "[simplnx][PipelineFlush]")
{
  const auto failure = GENERATE(SnapshotFailure::Standard, SnapshotFailure::Allocation, SnapshotFailure::Unknown);
  DYNAMIC_SECTION("Copy failure " << static_cast<int>(failure))
  {
    CompletionFixture fixture(StoreKind::Numeric);
    auto snapshotControl = AddSnapshotGroup(fixture);
    fixture.firstControl->result = MakeErrorResult(k_FilterError, "before-copy-error-sentinel");
    fixture.flushControl->result = MakeErrorResult(k_FlushError, "before-copy-flush-sentinel");
    fixture.firstControl->onExecute = [snapshotControl, failure]() { snapshotControl->failure = failure; };
    CompletionTrace trace(fixture.pipeline, fixture.firstNode);
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
    snapshotControl->failure = SnapshotFailure::None;
    CHECK_FALSE(succeeded);
    CHECK(fixture.secondControl->executeCalls == 0);
    CHECK(snapshotControl->copyCalls >= 2);
    CHECK(fixture.pipeline.hasErrors());
    const auto errors = fixture.firstNode->getErrors();
    REQUIRE(errors.size() == 3);
    CHECK(errors[0].code == k_FilterError);
    CHECK(errors[1].code == k_FlushError);
    CHECK(errors[2].code == (failure == SnapshotFailure::Allocation ? -272 : -6071));
    REQUIRE(trace.rootDetails.size() == 1);
    CHECK(trace.rootDetails[0].index == -1);
    REQUIRE(trace.rootDetails[0].errors.size() == 1);
    CHECK(trace.rootDetails[0].errors[0].code == (failure == SnapshotFailure::Allocation ? -272 : -6071));
    CHECK(trace.rootDetails[0].warnings.empty());
    CheckNodeCompletion(trace, *fixture.firstNode);
    const auto* sourceGroup = fixture.dataStructure.getData(DataPath({"CopyFault"}));
    REQUIRE(sourceGroup != nullptr);
    CHECK(sourceGroup->getDataStructure() == &fixture.dataStructure);
    CHECK(fixture.numericStore->getValue(0) == 11);
    CHECK(fixture.numericStore->getValue(1) == 13);
    CHECK(fixture.firstNode->getDataStructure().getData(DataPath({"CopyFault"})) == nullptr);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("PipelineFlush root snapshot errors use a pipeline event and preserve prior snapshot", "[simplnx][PipelineFlush]")
{
  const auto failure = GENERATE(SnapshotFailure::Standard, SnapshotFailure::Allocation);
  DYNAMIC_SECTION("Root copy failure " << static_cast<int>(failure))
  {
    CompletionFixture fixture(StoreKind::Numeric, false);
    auto snapshotControl = AddSnapshotGroup(fixture);
    bool succeeded = false;
    REQUIRE_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
    REQUIRE(succeeded);
    const auto oldRootNextId = fixture.pipeline.getDataStructure().getNextId();
    REQUIRE(DataGroup::Create(fixture.dataStructure, "NewGroup") != nullptr);
    CompletionTrace trace(fixture.pipeline, fixture.firstNode);
    // End follows the successful node snapshot, so only the root snapshot sees this fault.
    nod::scoped_connection armRoot = fixture.firstNode->getFilterUpdateSignal().connect([snapshotControl, failure](AbstractPipelineNode*, int32, const std::string& message) {
      if(message == "End")
      {
        snapshotControl->failure = failure;
      }
    });
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
    snapshotControl->failure = SnapshotFailure::None;
    CHECK_FALSE(succeeded);
    CHECK(fixture.pipeline.hasErrors());
    CHECK_FALSE(fixture.firstNode->hasErrors());
    CHECK(trace.nodeDetails.empty());
    REQUIRE(trace.rootDetails.size() == 1);
    CHECK(trace.rootDetails[0].index == -1);
    REQUIRE(trace.rootDetails[0].errors.size() == 1);
    CHECK(trace.rootDetails[0].errors[0].code == (failure == SnapshotFailure::Allocation ? -272 : -6071));
    CHECK(fixture.pipeline.getDataStructure().getData(DataPath({"NewGroup"})) == nullptr);
    CHECK(fixture.pipeline.getDataStructure().getNextId() == oldRootNextId);
    REQUIRE(fixture.firstNode->getDataStructure().getData(DataPath({"NewGroup"})) != nullptr);
    CheckSnapshot(fixture, fixture.firstNode->getDataStructure());
    CheckNodeCompletion(trace, *fixture.firstNode);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("PipelineFlush empty pipeline reports a root snapshot exception", "[simplnx][PipelineFlush]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto control = std::make_shared<SnapshotControl>();
  auto group = std::make_shared<SnapshotGroup>(dataStructure, control);
  REQUIRE(dataStructure.insert(group, DataPath{}));
  control->failure = SnapshotFailure::Standard;
  Pipeline pipeline;
  CompletionTrace trace(pipeline, nullptr);
  bool succeeded = true;
  CHECK_NOTHROW(succeeded = pipeline.execute(dataStructure, false));
  control->failure = SnapshotFailure::None;
  CHECK_FALSE(succeeded);
  CHECK(pipeline.hasErrors());
  REQUIRE(trace.rootDetails.size() == 1);
  CHECK(trace.rootDetails[0].index == -1);
  REQUIRE(trace.rootDetails[0].errors.size() == 1);
  CHECK(trace.rootDetails[0].errors[0].code == -6071);
  REQUIRE_FALSE(trace.pipelineStates.empty());
  CHECK(trace.pipelineStates.back() == RunState::Idle);
  CHECK(dataStructure.getData(group->getId()) == group.get());
  CHECK(group->getDataStructure() == &dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("PipelineFlush snapshot preserves multiply parented object identity", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto* secondParent = DataGroup::Create(fixture.dataStructure, "Second");
  REQUIRE(secondParent != nullptr);
  auto* sourceArray = fixture.dataStructure.getDataAs<DataArray<int32>>(k_ValuesPath);
  REQUIRE(sourceArray != nullptr);
  REQUIRE(fixture.dataStructure.setAdditionalParent(sourceArray->getId(), secondParent->getId()));
  const auto sourceId = sourceArray->getId();
  CompletionTrace trace(fixture.pipeline, fixture.firstNode);
  bool succeeded = false;
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
  CHECK(succeeded);
  const auto& snapshot = fixture.firstNode->getDataStructure();
  const auto* firstCopy = snapshot.getData(k_ValuesPath);
  const auto* secondCopy = snapshot.getData(DataPath({"Second", "Values"}));
  REQUIRE(firstCopy != nullptr);
  CHECK(secondCopy == firstCopy);
  CHECK(firstCopy->getId() == sourceId);
  CHECK(fixture.flushControl->checkedCalls == 1);
  CHECK(fixture.dataStructure.getData(sourceId) == sourceArray);
  CheckSnapshot(fixture, snapshot);
  CheckNodeCompletion(trace, *fixture.firstNode);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

TEST_CASE("PipelineFlush nested cancellation preserves completion failure and clears it on retry", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto nestedPipeline = std::make_shared<Pipeline>(std::move(fixture.pipeline));
  Pipeline outerPipeline;
  REQUIRE(outerPipeline.push_back(nestedPipeline));
  REQUIRE(outerPipeline.push_back(std::make_unique<CompletionFilter>(fixture.secondControl), Arguments{}));
  std::atomic_bool cancelled = false;
  fixture.flushControl->cancelOnFlush = &cancelled;
  fixture.flushControl->result = MakeErrorResult(k_FlushError, "nested-completion-sentinel");
  CompletionTrace failureTrace(outerPipeline, fixture.firstNode);
  bool succeeded = true;
  CHECK_NOTHROW(succeeded = outerPipeline.execute(fixture.dataStructure, cancelled));
  CHECK_FALSE(succeeded);
  CHECK(outerPipeline.hasErrors());
  CHECK(nestedPipeline->hasErrors());
  CHECK(outerPipeline.hasCompletionErrors());
  CHECK(nestedPipeline->hasCompletionErrors());
  CHECK(fixture.firstNode->hasCompletionErrors());
  CHECK(fixture.secondControl->executeCalls == 0);
  CheckNodeCompletion(failureTrace, *fixture.firstNode);

  fixture.flushControl->result = {};
  fixture.flushControl->cancelOnFlush = nullptr;
  cancelled.store(false);
  CompletionTrace retryTrace(outerPipeline, fixture.firstNode);
  CHECK_NOTHROW(succeeded = outerPipeline.execute(fixture.dataStructure, cancelled));
  CHECK(succeeded);
  CHECK_FALSE(outerPipeline.hasErrors());
  CHECK_FALSE(nestedPipeline->hasErrors());
  CHECK_FALSE(outerPipeline.hasCompletionErrors());
  CHECK_FALSE(nestedPipeline->hasCompletionErrors());
  CHECK_FALSE(fixture.firstNode->hasCompletionErrors());
  CHECK(fixture.secondControl->executeCalls == 1);
  CHECK(fixture.flushControl->checkedCalls == 3);
  CheckNodeCompletion(retryTrace, *fixture.firstNode);
  CheckSnapshot(fixture, nestedPipeline->getDataStructure());
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

TEST_CASE("PipelineFlush failed node snapshot retains its prior complete topology", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto snapshotControl = AddSnapshotGroup(fixture);
  bool succeeded = false;
  REQUIRE_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
  REQUIRE(succeeded);
  const auto oldNodeNextId = fixture.firstNode->getDataStructure().getNextId();
  REQUIRE(DataGroup::Create(fixture.dataStructure, "NewGroup") != nullptr);
  fixture.firstControl->onExecute = [snapshotControl]() { snapshotControl->failure = SnapshotFailure::Standard; };
  CompletionTrace trace(fixture.pipeline, fixture.firstNode);
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
  snapshotControl->failure = SnapshotFailure::None;
  CHECK_FALSE(succeeded);
  CHECK(fixture.firstNode->hasErrors());
  CHECK(fixture.pipeline.hasErrors());
  const auto& retained = fixture.firstNode->getDataStructure();
  CHECK(retained.getData(DataPath({"NewGroup"})) == nullptr);
  CHECK(retained.getNextId() == oldNodeNextId);
  const auto* retainedArray = retained.getDataAs<DataArray<int32>>(k_ValuesPath);
  REQUIRE(retainedArray != nullptr);
  CHECK(retainedArray->getDataStructure() == &retained);
  CHECK(&retainedArray->getDataStoreRef() == fixture.numericStore.get());
  const auto* sourceArray = fixture.dataStructure.getDataAs<DataArray<int32>>(k_ValuesPath);
  REQUIRE(sourceArray != nullptr);
  CHECK(sourceArray->getDataStructure() == &fixture.dataStructure);
  CHECK(sourceArray->getDataStoreRef().getValue(0) == 11);
  CHECK(sourceArray->getDataStoreRef().getValue(1) == 13);
  REQUIRE(fixture.dataStructure.getData(DataPath({"NewGroup"})) != nullptr);
  CheckNodeCompletion(trace, *fixture.firstNode);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}

TEST_CASE("PipelineFlush checked virtual preserves invalid-empty extension results and legacy completion", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  Pipeline pipeline;
  auto node = std::make_shared<InterfaceCompletionNode>(std::make_unique<CompletionFilter>(fixture.firstControl), Arguments{});
  node->setIndex(0);
  REQUIRE(pipeline.push_back(node));
  node->overrideCompletion = true;
  node->completionResult.m_Expected = nonstd::make_unexpected(ErrorCollection{});
  node->completionResult.warnings().push_back({k_FlushWarning, "empty-extension-warning"});
  CompletionTrace trace(pipeline, node.get());
  CHECK_FALSE(node->hasCompletionErrors());
  CHECK_FALSE(pipeline.execute(fixture.dataStructure, false));
  CHECK(node->hasCompletionErrors());
  CHECK(pipeline.hasCompletionErrors());
  CHECK(pipeline.hasErrors());
  REQUIRE(node->getErrors().size() == 1);
  CHECK(node->getErrors()[0].code == -6071);
  CHECK(node->getErrors()[0].message.find("checked completion") != std::string::npos);
  REQUIRE(node->getWarnings().size() == 1);
  CHECK(node->getWarnings()[0].message == "empty-extension-warning");
  CHECK(node->checkedCalls == 1);
  CHECK(node->legacyCalls == 0);
  CheckNodeCompletion(trace, *node);

  node->overrideCompletion = false;
  CHECK(pipeline.execute(fixture.dataStructure, false));
  CHECK_FALSE(node->hasCompletionErrors());
  CHECK_FALSE(pipeline.hasCompletionErrors());
  CHECK(node->getErrors().empty());
  CHECK(node->checkedCalls == 2);
  CHECK(node->legacyCalls == 0);
  CHECK(fixture.flushControl->checkedCalls == 1);
  node->completeLegacy(fixture.dataStructure);
  CHECK(node->legacyCalls == 1);
  CHECK(fixture.flushControl->legacyCalls == 1);
  CHECK(fixture.flushControl->checkedCalls == 1);
}

TEST_CASE("PipelineFlush preserves original invalid-empty filter result validity", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric);
  // The IFilter wrapper returns an invalid preflight result without an intervening diagnostic merge.
  fixture.firstControl->invalidEmptyPreflight = true;
  CompletionTrace trace(fixture.pipeline, fixture.firstNode);
  CHECK_FALSE(fixture.pipeline.execute(fixture.dataStructure, false));
  CHECK(fixture.pipeline.hasErrors());
  CHECK(fixture.firstNode->hasErrors());
  CHECK_FALSE(fixture.firstNode->hasCompletionErrors());
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.firstNode->getErrors().empty());
  REQUIRE(fixture.firstNode->getWarnings().size() == 1);
  CHECK(fixture.firstNode->getWarnings()[0].message == "invalid-empty-original-warning");
  CHECK(fixture.firstControl->executeCalls == 0);
  CHECK(fixture.secondControl->executeCalls == 0);
  CHECK(fixture.flushControl->checkedCalls == 1);
  CheckNodeCompletion(trace, *fixture.firstNode);
}

TEST_CASE("PipelineFlush completion outcome follows actual execution lifecycle", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric);
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK_FALSE(fixture.firstNode->hasCompletionErrors());
  fixture.flushControl->result = MakeErrorResult(k_FlushError, "lifecycle-failure");
  REQUIRE_FALSE(fixture.pipeline.execute(fixture.dataStructure, false));
  CHECK(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.firstNode->hasCompletionErrors());
  CHECK_FALSE(fixture.pipeline.executeFrom(fixture.pipeline.size() + 1, fixture.dataStructure, false));
  CHECK(fixture.pipeline.hasCompletionErrors());

  auto copy = fixture.pipeline.deepCopy();
  CHECK_FALSE(copy->hasCompletionErrors());

  fixture.flushControl->result = {};
  REQUIRE(fixture.pipeline.preflight(fixture.dataStructure, false));
  CHECK(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.firstNode->hasCompletionErrors());
  REQUIRE(fixture.pipeline.executeFrom(1, fixture.dataStructure, false));
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.firstNode->hasCompletionErrors());
  fixture.firstNode->setDisabled(true);
  REQUIRE(fixture.pipeline.execute(fixture.dataStructure, false));
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.firstNode->hasCompletionErrors());
  fixture.firstNode->setDisabled(false);
  REQUIRE(fixture.pipeline.execute(fixture.dataStructure, false));
  CHECK_FALSE(fixture.firstNode->hasCompletionErrors());
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());

  fixture.flushControl->result = MakeErrorResult(k_FlushError, "copy-lifecycle-failure");
  REQUIRE_FALSE(fixture.pipeline.execute(fixture.dataStructure, false));
  Pipeline copied(fixture.pipeline);
  CHECK_FALSE(copied.hasCompletionErrors());
  Pipeline moved(std::move(copied));
  CHECK_FALSE(moved.hasCompletionErrors());
}

TEST_CASE("PipelineFlush failed snapshot notification leaves live source topology usable", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto control = AddSnapshotGroup(fixture);
  REQUIRE(fixture.pipeline.execute(fixture.dataStructure, false));
  const auto* sourceArray = fixture.dataStructure.getData(k_ValuesPath);
  REQUIRE(sourceArray != nullptr);
  const auto sourceId = sourceArray->getId();
  const auto parentIds = sourceArray->getParentIds();
  RemovalWitness witness;
  witness.startObservingStructure(&fixture.dataStructure);
  fixture.firstControl->onExecute = [control]() { control->failure = SnapshotFailure::Standard; };
  CompletionTrace trace(fixture.pipeline, fixture.firstNode);
  bool succeeded = true;
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
  control->failure = SnapshotFailure::None;
  CHECK_FALSE(succeeded);
  // Temporary clones can emit source removals before copy construction rebinds their owner.
  CHECK(witness.liveRemovalCount > 0);
  CHECK(fixture.dataStructure.getData(witness.lastLiveId) != nullptr);
  CHECK(fixture.dataStructure.getData(sourceId) == sourceArray);
  CHECK(fixture.dataStructure.getData(k_ValuesPath) == sourceArray);
  CHECK(sourceArray->getParentIds() == parentIds);
  CHECK(sourceArray->getDataStructure() == &fixture.dataStructure);
  CHECK(fixture.numericStore->getValue(0) == 11);
  CHECK(fixture.numericStore->getValue(1) == 13);
  CheckSnapshot(fixture, fixture.firstNode->getDataStructure());
  CheckNodeCompletion(trace, *fixture.firstNode);
  witness.stopObservingStructure();
}

TEST_CASE("PipelineFlush disabled pipeline still reports final snapshot errors", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric);
  auto control = AddSnapshotGroup(fixture);
  for(const auto& node : fixture.pipeline)
  {
    node->setDisabled(true);
  }
  control->failure = SnapshotFailure::Unknown;
  CompletionTrace trace(fixture.pipeline, nullptr);
  bool succeeded = true;
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.dataStructure, false));
  control->failure = SnapshotFailure::None;
  CHECK_FALSE(succeeded);
  CHECK(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.firstControl->executeCalls == 0);
  CHECK(fixture.secondControl->executeCalls == 0);
  CHECK(fixture.flushControl->checkedCalls == 0);
  REQUIRE(trace.rootDetails.size() == 1);
  CHECK(trace.rootDetails[0].index == -1);
  REQUIRE(trace.rootDetails[0].errors.size() == 1);
  CHECK(trace.rootDetails[0].errors[0].code == -6071);
  REQUIRE_FALSE(trace.pipelineStates.empty());
  CHECK(trace.pipelineStates.back() == RunState::Idle);
}
