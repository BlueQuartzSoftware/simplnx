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
  usize checked_calls = 0;
  usize legacy_calls = 0;
  std::atomic_bool* cancel_on_flush = nullptr;
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

  ~NumericCompletionStore() override = default;
  NumericCompletionStore(const NumericCompletionStore&) = default;
  NumericCompletionStore(NumericCompletionStore&&) = default;
  NumericCompletionStore& operator=(const NumericCompletionStore&) = delete;
  NumericCompletionStore& operator=(NumericCompletionStore&&) noexcept = default;

  /**
   * @brief Counts a legacy call without returning checked diagnostics.
   */
  void flush() const override
  {
    ++m_Control->legacy_calls;
    if(m_Control->cancel_on_flush != nullptr)
    {
      m_Control->cancel_on_flush->store(true);
    }
  }

  /**
   * @brief Returns diagnostics and can request cancellation after execute.
   * @return The configured completion result.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    ++m_Control->checked_calls;
    if(m_Control->cancel_on_flush != nullptr)
    {
      m_Control->cancel_on_flush->store(true);
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

  ~ListCompletionStore() override = default;
  ListCompletionStore(const ListCompletionStore&) = default;
  ListCompletionStore(ListCompletionStore&&) = default;
  ListCompletionStore& operator=(const ListCompletionStore&) = delete;
  ListCompletionStore& operator=(ListCompletionStore&&) = delete;

  /**
   * @brief Returns the selected list completion diagnostics.
   * @return The configured completion result.
   */
  [[nodiscard]] Result<> flushChecked() const override
  {
    ++m_Control->checked_calls;
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
  usize execute_calls = 0;
  bool throw_allocation = false;
  bool invalid_empty_preflight = false;
  std::function<void()> on_execute;
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

  ~CompletionFilter() override = default;
  CompletionFilter(const CompletionFilter&) = delete;
  CompletionFilter(CompletionFilter&&) = delete;
  CompletionFilter& operator=(const CompletionFilter&) = delete;
  CompletionFilter& operator=(CompletionFilter&&) = delete;

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
  UniquePointerType clone() const override
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
    result.output_values.push_back({"Completion value", "preserved"});
    if(m_Control->invalid_empty_preflight)
    {
      result.output_actions.expected = nonstd::make_unexpected(ErrorCollectionType{});
      result.output_actions.warnings().push_back({k_FilterWarning, "invalid-empty-original-warning"});
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
    ++m_Control->execute_calls;
    if(m_Control->on_execute)
    {
      m_Control->on_execute();
    }
    if(m_Control->throw_allocation)
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

  ~InterfaceCompletionNode() override = default;
  InterfaceCompletionNode(const InterfaceCompletionNode&) = delete;
  InterfaceCompletionNode(InterfaceCompletionNode&&) = delete;
  InterfaceCompletionNode& operator=(const InterfaceCompletionNode&) = delete;
  InterfaceCompletionNode& operator=(InterfaceCompletionNode&&) = delete;

  bool override_completion = false;
  Result<> completion_result;
  usize checked_calls = 0;
  usize legacy_calls = 0;

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
    ++checked_calls;
    return override_completion ? completion_result : AbstractPipelineNode::endExecutionChecked(dataStructure);
  }

  /**
   * @brief Counts the legacy virtual while retaining its original flush contract.
   * @param dataStructure Supplies data to flush and snapshot.
   */
  void endExecution(DataStructure& dataStructure) override
  {
    ++legacy_calls;
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
  RemovalWitness() = default;
  ~RemovalWitness() override = default;
  RemovalWitness(const RemovalWitness&) = delete;
  RemovalWitness(RemovalWitness&&) = delete;
  RemovalWitness& operator=(const RemovalWitness&) = delete;
  RemovalWitness& operator=(RemovalWitness&&) = delete;

  usize live_removal_count = 0;
  DataObject::IdType last_live_id = 0;

  /**
   * @brief Counts removal messages whose original object remains present.
   * @param target Supplies the source ID lookup.
   * @param message Identifies the destroyed temporary clone.
   */
  void onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& message) noexcept override
  {
    const auto* removedPtr = dynamic_cast<const DataRemovedMessage*>(message.get());
    if(removedPtr != nullptr && target->getData(removedPtr->getId()) != nullptr)
    {
      ++live_removal_count;
      last_live_id = removedPtr->getId();
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
  usize copy_calls = 0;
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

  ~SnapshotGroup() override = default;

  // Rvalues retain the source topology and shared fault control through the copy contract.
  SnapshotGroup(SnapshotGroup&& other) noexcept
  : SnapshotGroup(static_cast<const SnapshotGroup&>(other))
  {
  }

  SnapshotGroup& operator=(const SnapshotGroup&) = delete;
  SnapshotGroup& operator=(SnapshotGroup&&) = delete;

  /**
   * @brief Copies the object or throws the selected synthetic exception.
   * @return A caller-owned shallow copy.
   * @throws std::runtime_error If an ordinary copy fault is armed.
   * @throws std::bad_alloc If an allocation copy fault is armed.
   * @throws int If a nonstandard copy fault is armed.
   */
  DataObject* shallowCopy() override
  {
    ++m_Control->copy_calls;
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
  WarningCollectionType warnings;
  ErrorCollectionType errors;
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
    connections.emplace_back(pipeline.getPipelineRunStateSignal().connect([this](AbstractPipelineNode*, RunState state) { pipeline_states.push_back(state); }));
    connections.emplace_back(pipeline.getCancelledSignal().connect([this]() { ++cancelled_calls; }));
    connections.emplace_back(pipeline.getFilterFaultDetailSignal().connect(
        [this](AbstractPipelineNode*, int32 index, const WarningCollectionType& warnings, const ErrorCollectionType& errors) { root_details.push_back({index, warnings, errors}); }));
    if(firstNode != nullptr)
    {
      connections.emplace_back(firstNode->getFilterRunStateSignal().connect([this](AbstractPipelineNode*, int32, RunState state) { node_states.push_back(state); }));
      connections.emplace_back(firstNode->getFilterUpdateSignal().connect([this](AbstractPipelineNode*, int32, const std::string& message) { updates.push_back(message); }));
      connections.emplace_back(firstNode->getFilterFaultDetailSignal().connect(
          [this](AbstractPipelineNode*, int32 index, const WarningCollectionType& warnings, const ErrorCollectionType& errors) { node_details.push_back({index, warnings, errors}); }));
    }
  }

  ~CompletionTrace() = default;
  CompletionTrace(const CompletionTrace&) = default;
  CompletionTrace(CompletionTrace&&) = default;
  CompletionTrace& operator=(const CompletionTrace&) = default;
  CompletionTrace& operator=(CompletionTrace&&) = default;

  std::vector<RunState> pipeline_states;
  std::vector<RunState> node_states;
  std::vector<std::string> updates;
  std::vector<DetailRecord> node_details;
  std::vector<DetailRecord> root_details;
  usize cancelled_calls = 0;
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
  : store_kind(kind)
  {
    unit_test::LoadPlugins();
    const auto* groupPtr = DataGroup::Create(data_structure, "Group");
    REQUIRE(groupPtr != nullptr);
    if(kind == StoreKind::Numeric)
    {
      numeric_store = std::make_shared<NumericCompletionStore>(flush_control);
      REQUIRE(DataArray<int32>::Create(data_structure, "Values", numeric_store, groupPtr->getId()) != nullptr);
    }
    else
    {
      list_store = std::make_shared<ListCompletionStore>(flush_control);
      REQUIRE(NeighborList<int32>::Create(data_structure, "Neighbors", list_store, groupPtr->getId()) != nullptr);
    }
    REQUIRE(pipeline.push_back(std::make_unique<CompletionFilter>(first_control), Arguments{}));
    first_node = dynamic_cast<PipelineFilter*>(pipeline.at(0));
    REQUIRE(first_node != nullptr);
    first_node->setIndex(0);
    if(includeSecond)
    {
      REQUIRE(pipeline.push_back(std::make_unique<CompletionFilter>(second_control), Arguments{}));
      auto* secondNodePtr = dynamic_cast<PipelineFilter*>(pipeline.at(1));
      REQUIRE(secondNodePtr != nullptr);
      secondNodePtr->setIndex(1);
    }
  }

  ~CompletionFixture() = default;
  CompletionFixture(const CompletionFixture&) = default;
  CompletionFixture(CompletionFixture&&) = default;
  CompletionFixture& operator=(const CompletionFixture&) = default;
  CompletionFixture& operator=(CompletionFixture&&) = default;

  StoreKind store_kind;
  DataStructure data_structure;
  Pipeline pipeline;
  std::shared_ptr<FlushControl> flush_control = std::make_shared<FlushControl>();
  std::shared_ptr<FilterControl> first_control = std::make_shared<FilterControl>();
  std::shared_ptr<FilterControl> second_control = std::make_shared<FilterControl>();
  std::shared_ptr<NumericCompletionStore> numeric_store;
  std::shared_ptr<ListCompletionStore> list_store;
  PipelineFilter* first_node = nullptr;
};

/**
 * @brief Checks literal contents and shared storage in a completed shallow snapshot.
 * @param fixture Owns the source structure and original stores.
 * @param snapshot Supplies the captured object topology.
 */
void CheckSnapshot(const CompletionFixture& fixture, const DataStructure& snapshot)
{
  const DataPath& path = fixture.store_kind == StoreKind::Numeric ? k_ValuesPath : k_ListPath;
  const auto* sourceObjectPtr = fixture.data_structure.getData(path);
  const auto* copiedObjectPtr = snapshot.getData(path);
  REQUIRE(sourceObjectPtr != nullptr);
  REQUIRE(copiedObjectPtr != nullptr);
  CHECK(copiedObjectPtr != sourceObjectPtr);
  CHECK(copiedObjectPtr->getId() == sourceObjectPtr->getId());
  CHECK(copiedObjectPtr->getParentIds() == sourceObjectPtr->getParentIds());
  CHECK(copiedObjectPtr->getDataStructure() == &snapshot);
  CHECK(sourceObjectPtr->getDataStructure() == &fixture.data_structure);
  CHECK(snapshot.getNextId() == fixture.data_structure.getNextId());
  CHECK(&snapshot.formatResolver() == &fixture.data_structure.formatResolver());
  if(fixture.store_kind == StoreKind::Numeric)
  {
    const auto* arrayPtr = snapshot.getDataAs<DataArray<int32>>(path);
    REQUIRE(arrayPtr != nullptr);
    CHECK(&arrayPtr->getDataStoreRef() == fixture.numeric_store.get());
    CHECK(arrayPtr->getDataStoreRef().getValue(0) == 11);
    CHECK(arrayPtr->getDataStoreRef().getValue(1) == 13);
  }
  else
  {
    const auto* listsPtr = snapshot.getDataAs<NeighborList<int32>>(path);
    REQUIRE(listsPtr != nullptr);
    CHECK(listsPtr->getIListStore() == fixture.list_store.get());
    CHECK(listsPtr->getList(0) == std::vector<int32>{11, 13});
    CHECK(listsPtr->getList(1) == std::vector<int32>{17});
  }
}

/**
 * @brief Checks normal terminal signals and the retained output-value sentinel.
 * @param trace Contains events from the completed or failed execution.
 * @param firstNode Owns the execute output values.
 */
void CheckNodeCompletion(const CompletionTrace& trace, const PipelineFilter& firstNode)
{
  REQUIRE_FALSE(trace.node_states.empty());
  CHECK(trace.node_states.back() == RunState::Idle);
  REQUIRE_FALSE(trace.pipeline_states.empty());
  CHECK(trace.pipeline_states.back() == RunState::Idle);
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
  auto group = std::make_shared<SnapshotGroup>(fixture.data_structure, control);
  REQUIRE(fixture.data_structure.insert(group, DataPath{}));
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
    fixture.first_control->result.warnings().push_back({k_FilterWarning, "execute-warning-sentinel"});
    if(originalError)
    {
      fixture.first_control->result.expected = nonstd::make_unexpected(ErrorCollectionType{{.code = k_FilterError, .message = "execute-error-sentinel"}});
    }
    fixture.flush_control->result = MakeErrorResult(k_FlushError, "backing.h5:/values: terminal-flush-sentinel");
    fixture.flush_control->result.warnings().push_back({k_FlushWarning, "flush-warning-sentinel"});
    CompletionTrace trace(fixture.pipeline, fixture.first_node);
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
    CHECK_FALSE(succeeded);
    CHECK(fixture.first_control->execute_calls == 1);
    CHECK(fixture.second_control->execute_calls == 0);
    CHECK(fixture.flush_control->checked_calls == 1);
    CHECK(fixture.flush_control->legacy_calls == 0);
    CHECK(fixture.first_node->hasErrors());
    CHECK_FALSE(fixture.first_node->hasWarnings());
    CHECK(fixture.pipeline.hasErrors());
    CHECK_FALSE(fixture.pipeline.hasWarnings());

    const auto errors = fixture.first_node->getErrors();
    REQUIRE(errors.size() == (originalError ? 2 : 1));
    if(originalError)
    {
      CHECK(errors.front().code == k_FilterError);
      CHECK(errors.front().message == "execute-error-sentinel");
    }
    CHECK(errors.back().code == k_FlushError);
    CHECK(errors.back().message.find("backing.h5:/values") != std::string::npos);
    CHECK(errors.back().message.find(kind == StoreKind::Numeric ? "Values" : "Neighbors") != std::string::npos);
    const auto warnings = fixture.first_node->getWarnings();
    REQUIRE(warnings.size() == 2);
    CHECK(warnings[0].code == k_FilterWarning);
    CHECK(warnings[1].code == k_FlushWarning);
    REQUIRE(trace.node_details.size() == 1);
    CHECK(trace.node_details[0].index == 0);
    REQUIRE(trace.node_details[0].errors.size() == errors.size());
    for(usize index = 0; index < errors.size(); ++index)
    {
      CHECK(trace.node_details[0].errors[index].code == errors[index].code);
      CHECK(trace.node_details[0].errors[index].message == errors[index].message);
    }
    CHECK(trace.root_details.empty());
    CheckNodeCompletion(trace, *fixture.first_node);
    CheckSnapshot(fixture, fixture.first_node->getDataStructure());
    CheckSnapshot(fixture, fixture.pipeline.getDataStructure());
    unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
  }
}

TEST_CASE("PipelineFlush retries the same stores and clears completion faults", "[simplnx][PipelineFlush]")
{
  const auto kind = GENERATE(StoreKind::Numeric, StoreKind::List);
  DYNAMIC_SECTION("Store " << static_cast<int>(kind))
  {
    CompletionFixture fixture(kind);
    fixture.flush_control->result = MakeErrorResult(k_FlushError, "retry-flush-sentinel");
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
    CHECK_FALSE(succeeded);
    CHECK(fixture.second_control->execute_calls == 0);
    fixture.flush_control->result = {};
    CompletionTrace retryTrace(fixture.pipeline, fixture.first_node);
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
    CHECK(succeeded);
    CHECK(fixture.first_control->execute_calls == 2);
    CHECK(fixture.second_control->execute_calls == 1);
    CHECK(fixture.flush_control->checked_calls == 3);
    CHECK(fixture.flush_control->legacy_calls == 0);
    CHECK_FALSE(fixture.first_node->hasErrors());
    CHECK_FALSE(fixture.pipeline.hasErrors());
    CHECK(fixture.first_node->getErrors().empty());
    CHECK(retryTrace.node_details.empty());
    CHECK(retryTrace.root_details.empty());
    CheckNodeCompletion(retryTrace, *fixture.first_node);
    CheckSnapshot(fixture, fixture.first_node->getDataStructure());
    unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
  }
}

TEST_CASE("PipelineFlush warning survives a later clean completion", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric);
  fixture.flush_control->result = MakeWarningVoidResult(k_FlushWarning, "early-warning-sentinel");
  fixture.second_control->on_execute = [&fixture]() { fixture.flush_control->result = {}; };
  CompletionTrace trace(fixture.pipeline, fixture.first_node);
  bool succeeded = false;
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
  CHECK(succeeded);
  CHECK(fixture.second_control->execute_calls == 1);
  CHECK(fixture.first_node->hasWarnings());
  CHECK(fixture.pipeline.hasWarnings());
  CHECK_FALSE(fixture.pipeline.hasErrors());
  REQUIRE(trace.node_details.size() == 1);
  REQUIRE(trace.node_details[0].warnings.size() == 1);
  CHECK(trace.node_details[0].warnings[0].code == k_FlushWarning);
  CHECK(trace.node_details[0].errors.empty());
  CHECK(trace.root_details.empty());
  CheckNodeCompletion(trace, *fixture.first_node);
  unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
}

TEST_CASE("PipelineFlush preserves allocation classification and earlier errors", "[simplnx][PipelineFlush]")
{
  const bool allocationInExecute = GENERATE(false, true);
  DYNAMIC_SECTION("Allocation in execute " << allocationInExecute)
  {
    CompletionFixture fixture(StoreKind::Numeric);
    fixture.first_control->throw_allocation = allocationInExecute;
    if(!allocationInExecute)
    {
      fixture.first_control->result = MakeErrorResult(k_FilterError, "original-before-allocation");
    }
    fixture.flush_control->result = MakeErrorResult(allocationInExecute ? k_FlushError : -272, "completion-allocation-routing");
    CompletionTrace trace(fixture.pipeline, fixture.first_node);
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
    CHECK_FALSE(succeeded);
    CHECK(fixture.second_control->execute_calls == 0);
    const auto errors = fixture.first_node->getErrors();
    REQUIRE(errors.size() == 2);
    CHECK(errors[0].code == (allocationInExecute ? -272 : k_FilterError));
    CHECK(errors[1].code == (allocationInExecute ? k_FlushError : -272));
    CheckNodeCompletion(trace, *fixture.first_node);
    CheckSnapshot(fixture, fixture.first_node->getDataStructure());
    unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
  }
}

TEST_CASE("PipelineFlush cancellation at the flush boundary retains completion failure", "[simplnx][PipelineFlush]")
{
  const bool failFlush = GENERATE(false, true);
  DYNAMIC_SECTION("Flush fails " << failFlush)
  {
    CompletionFixture fixture(StoreKind::Numeric);
    std::atomic_bool cancelled = false;
    fixture.flush_control->cancel_on_flush = &cancelled;
    if(failFlush)
    {
      fixture.flush_control->result = MakeErrorResult(k_FlushError, "flush-after-cancel-sentinel");
    }
    CompletionTrace trace(fixture.pipeline, fixture.first_node);
    bool succeeded = !failFlush;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, cancelled));
    CHECK(succeeded == !failFlush);
    CHECK(cancelled.load());
    CHECK(trace.cancelled_calls == 1);
    CHECK(fixture.second_control->execute_calls == 0);
    CHECK(fixture.pipeline.hasErrors() == failFlush);
    if(failFlush)
    {
      const auto errors = fixture.first_node->getErrors();
      REQUIRE(errors.size() == 1);
      CHECK(errors[0].code == k_FlushError);
    }
    CheckNodeCompletion(trace, *fixture.first_node);
    fixture.flush_control->cancel_on_flush = nullptr;
    unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
  }
}

TEST_CASE("PipelineFlush leaves execute-stage cancellation behavior unchanged", "[simplnx][PipelineFlush]")
{
  const bool originalError = GENERATE(false, true);
  DYNAMIC_SECTION("Original execute error " << originalError)
  {
    CompletionFixture fixture(StoreKind::Numeric);
    std::atomic_bool cancelled = false;
    fixture.first_control->on_execute = [&cancelled]() { cancelled.store(true); };
    if(originalError)
    {
      fixture.first_control->result = MakeErrorResult(k_FilterError, "execute-cancel-error-sentinel");
    }
    CompletionTrace trace(fixture.pipeline, fixture.first_node);
    bool succeeded = false;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, cancelled));
    CHECK(succeeded);
    CHECK_FALSE(fixture.pipeline.hasErrors());
    CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
    CHECK_FALSE(fixture.first_node->hasCompletionErrors());
    CHECK(trace.cancelled_calls == 1);
    CHECK(fixture.second_control->execute_calls == 0);
    const auto errors = fixture.first_node->getErrors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].code == (originalError ? k_FilterError : -1));
    CheckNodeCompletion(trace, *fixture.first_node);
    unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
  }
}

TEST_CASE("PipelineFlush repeated snapshot exceptions retain diagnostics and settle", "[simplnx][PipelineFlush]")
{
  const auto failure = GENERATE(SnapshotFailure::Standard, SnapshotFailure::Allocation, SnapshotFailure::Unknown);
  DYNAMIC_SECTION("Copy failure " << static_cast<int>(failure))
  {
    CompletionFixture fixture(StoreKind::Numeric);
    auto snapshotControl = AddSnapshotGroup(fixture);
    fixture.first_control->result = MakeErrorResult(k_FilterError, "before-copy-error-sentinel");
    fixture.flush_control->result = MakeErrorResult(k_FlushError, "before-copy-flush-sentinel");
    fixture.first_control->on_execute = [snapshotControl, failure]() { snapshotControl->failure = failure; };
    CompletionTrace trace(fixture.pipeline, fixture.first_node);
    bool succeeded = true;
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
    snapshotControl->failure = SnapshotFailure::None;
    CHECK_FALSE(succeeded);
    CHECK(fixture.second_control->execute_calls == 0);
    CHECK(snapshotControl->copy_calls >= 2);
    CHECK(fixture.pipeline.hasErrors());
    const auto errors = fixture.first_node->getErrors();
    REQUIRE(errors.size() == 3);
    CHECK(errors[0].code == k_FilterError);
    CHECK(errors[1].code == k_FlushError);
    CHECK(errors[2].code == (failure == SnapshotFailure::Allocation ? -272 : -6071));
    REQUIRE(trace.root_details.size() == 1);
    CHECK(trace.root_details[0].index == -1);
    REQUIRE(trace.root_details[0].errors.size() == 1);
    CHECK(trace.root_details[0].errors[0].code == (failure == SnapshotFailure::Allocation ? -272 : -6071));
    CHECK(trace.root_details[0].warnings.empty());
    CheckNodeCompletion(trace, *fixture.first_node);
    const auto* sourceGroupPtr = fixture.data_structure.getData(DataPath({"CopyFault"}));
    REQUIRE(sourceGroupPtr != nullptr);
    CHECK(sourceGroupPtr->getDataStructure() == &fixture.data_structure);
    CHECK(fixture.numeric_store->getValue(0) == 11);
    CHECK(fixture.numeric_store->getValue(1) == 13);
    CHECK(fixture.first_node->getDataStructure().getData(DataPath({"CopyFault"})) == nullptr);
    unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
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
    REQUIRE_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
    REQUIRE(succeeded);
    const auto oldRootNextId = fixture.pipeline.getDataStructure().getNextId();
    REQUIRE(DataGroup::Create(fixture.data_structure, "NewGroup") != nullptr);
    CompletionTrace trace(fixture.pipeline, fixture.first_node);
    // End follows the successful node snapshot, so only the root snapshot sees this fault.
    nod::scoped_connection armRoot = fixture.first_node->getFilterUpdateSignal().connect([snapshotControl, failure](AbstractPipelineNode*, int32, const std::string& message) {
      if(message == "End")
      {
        snapshotControl->failure = failure;
      }
    });
    CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
    snapshotControl->failure = SnapshotFailure::None;
    CHECK_FALSE(succeeded);
    CHECK(fixture.pipeline.hasErrors());
    CHECK_FALSE(fixture.first_node->hasErrors());
    CHECK(trace.node_details.empty());
    REQUIRE(trace.root_details.size() == 1);
    CHECK(trace.root_details[0].index == -1);
    REQUIRE(trace.root_details[0].errors.size() == 1);
    CHECK(trace.root_details[0].errors[0].code == (failure == SnapshotFailure::Allocation ? -272 : -6071));
    CHECK(fixture.pipeline.getDataStructure().getData(DataPath({"NewGroup"})) == nullptr);
    CHECK(fixture.pipeline.getDataStructure().getNextId() == oldRootNextId);
    REQUIRE(fixture.first_node->getDataStructure().getData(DataPath({"NewGroup"})) != nullptr);
    CheckSnapshot(fixture, fixture.first_node->getDataStructure());
    CheckNodeCompletion(trace, *fixture.first_node);
    unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
  }
}

TEST_CASE("PipelineFlush empty pipeline reports a root snapshot exception", "[simplnx][PipelineFlush]")
{
  unit_test::LoadPlugins();
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
  REQUIRE(trace.root_details.size() == 1);
  CHECK(trace.root_details[0].index == -1);
  REQUIRE(trace.root_details[0].errors.size() == 1);
  CHECK(trace.root_details[0].errors[0].code == -6071);
  REQUIRE_FALSE(trace.pipeline_states.empty());
  CHECK(trace.pipeline_states.back() == RunState::Idle);
  CHECK(dataStructure.getData(group->getId()) == group.get());
  CHECK(group->getDataStructure() == &dataStructure);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("PipelineFlush snapshot preserves multiply parented object identity", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto* secondParentPtr = DataGroup::Create(fixture.data_structure, "Second");
  REQUIRE(secondParentPtr != nullptr);
  auto* sourceArrayPtr = fixture.data_structure.getDataAs<DataArray<int32>>(k_ValuesPath);
  REQUIRE(sourceArrayPtr != nullptr);
  REQUIRE(fixture.data_structure.setAdditionalParent(sourceArrayPtr->getId(), secondParentPtr->getId()));
  const auto sourceId = sourceArrayPtr->getId();
  CompletionTrace trace(fixture.pipeline, fixture.first_node);
  bool succeeded = false;
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
  CHECK(succeeded);
  const auto& snapshot = fixture.first_node->getDataStructure();
  const auto* firstCopyPtr = snapshot.getData(k_ValuesPath);
  const auto* secondCopyPtr = snapshot.getData(DataPath({"Second", "Values"}));
  REQUIRE(firstCopyPtr != nullptr);
  CHECK(secondCopyPtr == firstCopyPtr);
  CHECK(firstCopyPtr->getId() == sourceId);
  CHECK(fixture.flush_control->checked_calls == 1);
  CHECK(fixture.data_structure.getData(sourceId) == sourceArrayPtr);
  CheckSnapshot(fixture, snapshot);
  CheckNodeCompletion(trace, *fixture.first_node);
  unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
}

TEST_CASE("PipelineFlush nested cancellation preserves completion failure and clears it on retry", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto nestedPipeline = std::make_shared<Pipeline>(std::move(fixture.pipeline));
  Pipeline outerPipeline;
  REQUIRE(outerPipeline.push_back(nestedPipeline));
  REQUIRE(outerPipeline.push_back(std::make_unique<CompletionFilter>(fixture.second_control), Arguments{}));
  std::atomic_bool cancelled = false;
  fixture.flush_control->cancel_on_flush = &cancelled;
  fixture.flush_control->result = MakeErrorResult(k_FlushError, "nested-completion-sentinel");
  CompletionTrace failureTrace(outerPipeline, fixture.first_node);
  bool succeeded = true;
  CHECK_NOTHROW(succeeded = outerPipeline.execute(fixture.data_structure, cancelled));
  CHECK_FALSE(succeeded);
  CHECK(outerPipeline.hasErrors());
  CHECK(nestedPipeline->hasErrors());
  CHECK(outerPipeline.hasCompletionErrors());
  CHECK(nestedPipeline->hasCompletionErrors());
  CHECK(fixture.first_node->hasCompletionErrors());
  CHECK(fixture.second_control->execute_calls == 0);
  CheckNodeCompletion(failureTrace, *fixture.first_node);

  fixture.flush_control->result = {};
  fixture.flush_control->cancel_on_flush = nullptr;
  cancelled.store(false);
  CompletionTrace retryTrace(outerPipeline, fixture.first_node);
  CHECK_NOTHROW(succeeded = outerPipeline.execute(fixture.data_structure, cancelled));
  CHECK(succeeded);
  CHECK_FALSE(outerPipeline.hasErrors());
  CHECK_FALSE(nestedPipeline->hasErrors());
  CHECK_FALSE(outerPipeline.hasCompletionErrors());
  CHECK_FALSE(nestedPipeline->hasCompletionErrors());
  CHECK_FALSE(fixture.first_node->hasCompletionErrors());
  CHECK(fixture.second_control->execute_calls == 1);
  CHECK(fixture.flush_control->checked_calls == 3);
  CheckNodeCompletion(retryTrace, *fixture.first_node);
  CheckSnapshot(fixture, nestedPipeline->getDataStructure());
  unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
}

TEST_CASE("PipelineFlush failed node snapshot retains its prior complete topology", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto snapshotControl = AddSnapshotGroup(fixture);
  bool succeeded = false;
  REQUIRE_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
  REQUIRE(succeeded);
  const auto oldNodeNextId = fixture.first_node->getDataStructure().getNextId();
  REQUIRE(DataGroup::Create(fixture.data_structure, "NewGroup") != nullptr);
  fixture.first_control->on_execute = [snapshotControl]() { snapshotControl->failure = SnapshotFailure::Standard; };
  CompletionTrace trace(fixture.pipeline, fixture.first_node);
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
  snapshotControl->failure = SnapshotFailure::None;
  CHECK_FALSE(succeeded);
  CHECK(fixture.first_node->hasErrors());
  CHECK(fixture.pipeline.hasErrors());
  const auto& retained = fixture.first_node->getDataStructure();
  CHECK(retained.getData(DataPath({"NewGroup"})) == nullptr);
  CHECK(retained.getNextId() == oldNodeNextId);
  const auto* retainedArrayPtr = retained.getDataAs<DataArray<int32>>(k_ValuesPath);
  REQUIRE(retainedArrayPtr != nullptr);
  CHECK(retainedArrayPtr->getDataStructure() == &retained);
  CHECK(&retainedArrayPtr->getDataStoreRef() == fixture.numeric_store.get());
  const auto* sourceArrayPtr = fixture.data_structure.getDataAs<DataArray<int32>>(k_ValuesPath);
  REQUIRE(sourceArrayPtr != nullptr);
  CHECK(sourceArrayPtr->getDataStructure() == &fixture.data_structure);
  CHECK(sourceArrayPtr->getDataStoreRef().getValue(0) == 11);
  CHECK(sourceArrayPtr->getDataStoreRef().getValue(1) == 13);
  REQUIRE(fixture.data_structure.getData(DataPath({"NewGroup"})) != nullptr);
  CheckNodeCompletion(trace, *fixture.first_node);
  unit_test::CheckArraysInheritTupleDims(fixture.data_structure);
}

TEST_CASE("PipelineFlush checked virtual preserves invalid-empty extension results and legacy completion", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  Pipeline pipeline;
  auto node = std::make_shared<InterfaceCompletionNode>(std::make_unique<CompletionFilter>(fixture.first_control), Arguments{});
  node->setIndex(0);
  REQUIRE(pipeline.push_back(node));
  node->override_completion = true;
  node->completion_result.expected = nonstd::make_unexpected(ErrorCollectionType{});
  node->completion_result.warnings().push_back({k_FlushWarning, "empty-extension-warning"});
  CompletionTrace trace(pipeline, node.get());
  CHECK_FALSE(node->hasCompletionErrors());
  CHECK_FALSE(pipeline.execute(fixture.data_structure, false));
  CHECK(node->hasCompletionErrors());
  CHECK(pipeline.hasCompletionErrors());
  CHECK(pipeline.hasErrors());
  REQUIRE(node->getErrors().size() == 1);
  CHECK(node->getErrors()[0].code == -6071);
  CHECK(node->getErrors()[0].message.find("checked completion") != std::string::npos);
  REQUIRE(node->getWarnings().size() == 1);
  CHECK(node->getWarnings()[0].message == "empty-extension-warning");
  CHECK(node->checked_calls == 1);
  CHECK(node->legacy_calls == 0);
  CheckNodeCompletion(trace, *node);

  node->override_completion = false;
  CHECK(pipeline.execute(fixture.data_structure, false));
  CHECK_FALSE(node->hasCompletionErrors());
  CHECK_FALSE(pipeline.hasCompletionErrors());
  CHECK(node->getErrors().empty());
  CHECK(node->checked_calls == 2);
  CHECK(node->legacy_calls == 0);
  CHECK(fixture.flush_control->checked_calls == 1);
  node->completeLegacy(fixture.data_structure);
  CHECK(node->legacy_calls == 1);
  CHECK(fixture.flush_control->legacy_calls == 1);
  CHECK(fixture.flush_control->checked_calls == 1);
}

TEST_CASE("PipelineFlush preserves original invalid-empty filter result validity", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric);
  // The IFilter wrapper returns an invalid preflight result without an intervening diagnostic merge.
  fixture.first_control->invalid_empty_preflight = true;
  CompletionTrace trace(fixture.pipeline, fixture.first_node);
  CHECK_FALSE(fixture.pipeline.execute(fixture.data_structure, false));
  CHECK(fixture.pipeline.hasErrors());
  CHECK(fixture.first_node->hasErrors());
  CHECK_FALSE(fixture.first_node->hasCompletionErrors());
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.first_node->getErrors().empty());
  REQUIRE(fixture.first_node->getWarnings().size() == 1);
  CHECK(fixture.first_node->getWarnings()[0].message == "invalid-empty-original-warning");
  CHECK(fixture.first_control->execute_calls == 0);
  CHECK(fixture.second_control->execute_calls == 0);
  CHECK(fixture.flush_control->checked_calls == 1);
  CheckNodeCompletion(trace, *fixture.first_node);
}

TEST_CASE("PipelineFlush completion outcome follows actual execution lifecycle", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric);
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK_FALSE(fixture.first_node->hasCompletionErrors());
  fixture.flush_control->result = MakeErrorResult(k_FlushError, "lifecycle-failure");
  REQUIRE_FALSE(fixture.pipeline.execute(fixture.data_structure, false));
  CHECK(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.first_node->hasCompletionErrors());
  CHECK_FALSE(fixture.pipeline.executeFrom(fixture.pipeline.size() + 1, fixture.data_structure, false));
  CHECK(fixture.pipeline.hasCompletionErrors());

  auto copy = fixture.pipeline.deepCopy();
  CHECK_FALSE(copy->hasCompletionErrors());

  fixture.flush_control->result = {};
  REQUIRE(fixture.pipeline.preflight(fixture.data_structure, false));
  CHECK(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.first_node->hasCompletionErrors());
  REQUIRE(fixture.pipeline.executeFrom(1, fixture.data_structure, false));
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.first_node->hasCompletionErrors());
  fixture.first_node->setDisabled(true);
  REQUIRE(fixture.pipeline.execute(fixture.data_structure, false));
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.first_node->hasCompletionErrors());
  fixture.first_node->setDisabled(false);
  REQUIRE(fixture.pipeline.execute(fixture.data_structure, false));
  CHECK_FALSE(fixture.first_node->hasCompletionErrors());
  CHECK_FALSE(fixture.pipeline.hasCompletionErrors());

  fixture.flush_control->result = MakeErrorResult(k_FlushError, "copy-lifecycle-failure");
  REQUIRE_FALSE(fixture.pipeline.execute(fixture.data_structure, false));
  Pipeline copied(fixture.pipeline);
  CHECK_FALSE(copied.hasCompletionErrors());
  Pipeline moved(std::move(copied));
  CHECK_FALSE(moved.hasCompletionErrors());
}

TEST_CASE("PipelineFlush failed snapshot notification leaves live source topology usable", "[simplnx][PipelineFlush]")
{
  CompletionFixture fixture(StoreKind::Numeric, false);
  auto control = AddSnapshotGroup(fixture);
  REQUIRE(fixture.pipeline.execute(fixture.data_structure, false));
  const auto* sourceArrayPtr = fixture.data_structure.getData(k_ValuesPath);
  REQUIRE(sourceArrayPtr != nullptr);
  const auto sourceId = sourceArrayPtr->getId();
  const auto parentIds = sourceArrayPtr->getParentIds();
  RemovalWitness witness;
  witness.startObservingStructure(&fixture.data_structure);
  fixture.first_control->on_execute = [control]() { control->failure = SnapshotFailure::Standard; };
  CompletionTrace trace(fixture.pipeline, fixture.first_node);
  bool succeeded = true;
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
  control->failure = SnapshotFailure::None;
  CHECK_FALSE(succeeded);
  // Temporary clones can emit source removals before copy construction rebinds their owner.
  CHECK(witness.live_removal_count > 0);
  CHECK(fixture.data_structure.getData(witness.last_live_id) != nullptr);
  CHECK(fixture.data_structure.getData(sourceId) == sourceArrayPtr);
  CHECK(fixture.data_structure.getData(k_ValuesPath) == sourceArrayPtr);
  CHECK(sourceArrayPtr->getParentIds() == parentIds);
  CHECK(sourceArrayPtr->getDataStructure() == &fixture.data_structure);
  CHECK(fixture.numeric_store->getValue(0) == 11);
  CHECK(fixture.numeric_store->getValue(1) == 13);
  CheckSnapshot(fixture, fixture.first_node->getDataStructure());
  CheckNodeCompletion(trace, *fixture.first_node);
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
  CHECK_NOTHROW(succeeded = fixture.pipeline.execute(fixture.data_structure, false));
  control->failure = SnapshotFailure::None;
  CHECK_FALSE(succeeded);
  CHECK(fixture.pipeline.hasCompletionErrors());
  CHECK(fixture.first_control->execute_calls == 0);
  CHECK(fixture.second_control->execute_calls == 0);
  CHECK(fixture.flush_control->checked_calls == 0);
  REQUIRE(trace.root_details.size() == 1);
  CHECK(trace.root_details[0].index == -1);
  REQUIRE(trace.root_details[0].errors.size() == 1);
  CHECK(trace.root_details[0].errors[0].code == -6071);
  REQUIRE_FALSE(trace.pipeline_states.empty());
  CHECK(trace.pipeline_states.back() == RunState::Idle);
}
