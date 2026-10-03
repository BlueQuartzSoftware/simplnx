#include "IdentifySampleBenchmarkUtilities.hpp"
#include "SimplnxCore/Filters/Algorithms/IdentifySample.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"

#include <array>
#include <atomic>
#include <exception>
#include <new>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace nx::core;

namespace
{
using namespace IdentifySampleBenchmarkTest;
constexpr usize k_X = 19;
constexpr usize k_Y = 11;
constexpr usize k_Z = 13;
constexpr int32 k_PrimaryCode = -77891;
constexpr int32 k_SecondaryCode = -77892;
constexpr int32 k_PrimaryWarning = -77893;
constexpr int32 k_SecondaryWarning = -77894;

/**
 * @enum FaultKind
 * @brief Selects a carrier without changing the actual failing operation.
 */
enum class FaultKind
{
  None,       ///< Leaves the selected operation unchanged.
  Result,     ///< Returns a distinct error and warning.
  Standard,   ///< Throws PlaneFailure with its original text and payload.
  Allocation, ///< Throws AllocationFailure with its original payload.
  Nonstandard ///< Throws a payload that does not derive from std::exception.
};
/**
 * @enum CopyPhase
 * @brief Identifies actual mask-copy phases in the first batch.
 */
enum class CopyPhase
{
  GatherRead, ///< Reads source values before any plane finishes.
  CommitRead, ///< Reads fresh source values before completed planes are written.
  CommitWrite ///< Writes source values with only the completed planes replaced.
};
/**
 * @enum CancelAt
 * @brief Selects independently witnessed cancellation sites.
 */
enum class CancelAt
{
  None,         ///< Leaves the cancellation flag clear.
  Pre,          ///< Sets cancellation before the target call.
  Gather,       ///< Sets cancellation in the Z=1 source read.
  BeforePlane,  ///< Sets cancellation in the selected plane's Slice message.
  RecordWrite,  ///< Sets cancellation after a successful scratch write inside CCL.
  CommitRead,   ///< Sets cancellation in the first completed-prefix read.
  WithPrimary,  ///< Sets cancellation when the record read also fails.
  WithSecondary ///< Sets cancellation when publication also fails.
};
/**
 * @enum HandlerKind
 * @brief Distinguishes valid empty callbacks from Error-only throwing callbacks.
 */
enum class HandlerKind
{
  Empty,     ///< Requests an empty handler unless an Info control is armed.
  Capturing, ///< Counts ordinary messages without adding an Error failure.
  ErrorThrow ///< Throws from the Error-message branch.
};

/**
 * @class PlaneFailure
 * @brief Carries a test payload that must survive a lone standard exception.
 */
class PlaneFailure : public std::runtime_error
{
public:
  /**
   * @brief Stores the payload and chooses the primary or secondary error text.
   * @param value Exception payload; 101 selects the primary message.
   */
  explicit PlaneFailure(int value)
  : std::runtime_error(value == 101 ? "primary record payload 101" : "secondary copy payload 202")
  , payload(value)
  {
  }
  int payload;
};
/**
 * @class AllocationFailure
 * @brief Proves that allocation subclasses retain their payload on a lone failure.
 */
class AllocationFailure : public std::bad_alloc
{
public:
  /**
   * @brief Stores a payload that identifies the original allocation failure.
   * @param value Payload checked after the exception leaves the target call.
   */
  explicit AllocationFailure(int value)
  : payload(value)
  {
  }
  int payload;
};
/**
 * @struct NonstandardFailure
 * @brief Supplies a nonstandard exception with an independently checked payload.
 */
struct NonstandardFailure
{
  int payload;
};

/**
 * @brief Produces the selected error or exception at a witnessed test operation.
 * @param kind Chooses the Result or exception carrier.
 * @param primary True for the plane failure, false for its publication failure.
 * @return A distinct error and warning for a Result failure, otherwise a valid Result.
 * @throws PlaneFailure When kind selects a standard exception.
 * @throws AllocationFailure When kind selects an allocation exception.
 * @throws NonstandardFailure When kind selects a nonstandard exception.
 */
Result<> RaiseFault(FaultKind kind, bool primary)
{
  const int payload = primary ? 101 : 202;
  switch(kind)
  {
  case FaultKind::Result: {
    auto result = MakeErrorResult(primary ? k_PrimaryCode : k_SecondaryCode, primary ? "primary record Result" : "secondary copy Result");
    result.warnings().push_back({primary ? k_PrimaryWarning : k_SecondaryWarning, primary ? "primary warning" : "secondary warning"});
    return result;
  }
  case FaultKind::Standard:
    throw PlaneFailure(payload);
  case FaultKind::Allocation:
    throw AllocationFailure(payload);
  case FaultKind::Nonstandard:
    throw NonstandardFailure{payload};
  case FaultKind::None:
    return {};
  }
  return {};
}

/**
 * @struct FailureState
 * @brief Collects actual record/copy witnesses; all fixtures execute serially.
 * @note A null selected array tests identity filtering. State and any selected array outlive all installed callbacks and decorated stores.
 */
struct FailureState
{
  FaultKind primary = FaultKind::None;
  FaultKind secondary = FaultKind::None;
  CopyPhase secondaryPhase = CopyPhase::CommitWrite;
  CancelAt cancelAt = CancelAt::None;
  usize primaryPlane = 1;
  usize secondaryZ = 1;
  bool messagePrimary = false;
  bool aggregateFailure = false;
  bool diagnosticMatch = true;
  bool armed = false;
  bool primaryFired = false;
  bool secondaryFired = false;
  bool cancelFired = false;
  usize aggregateCalls = 0;
  usize recordFactories = 0;
  usize liveRecords = 0;
  usize recordReads = 0;
  usize recordWrites = 0;
  usize reads = 0;
  usize writes = 0;
  usize successfulWrites = 0;
  usize infoCalls = 0;
  usize errorCalls = 0;
  const IDataArray* selected = nullptr;
  std::atomic_bool cancel = false;
};

/**
 * @class RecordDecorator
 * @brief Injects only from an actual CCL scratch read or successful scratch write.
 */
class RecordDecorator : public ITemporaryRecordStore
{
public:
  /**
   * @brief Takes ownership of the real scratch store and records its plane identity.
   * @param delegate Owns the validated resident record store.
   * @param state Receives operation counts and supplies faults; outlives this decorator.
   * @param plane Zero-based plane selected by actual record-provider creation order.
   */
  RecordDecorator(std::unique_ptr<ITemporaryRecordStore> delegate, FailureState& state, usize plane)
  : m_Delegate(std::move(delegate))
  , m_State(state)
  , m_Plane(plane)
  {
    ++m_State.liveRecords;
  }
  /**
   * @brief Marks the decorator as closed before its owned store is destroyed.
   */
  ~RecordDecorator() noexcept override
  {
    --m_State.liveRecords;
  }
  uint64 recordSize() const override
  {
    return m_Delegate->recordSize();
  }
  uint64 recordCount() const override
  {
    return m_Delegate->recordCount();
  }
  uint64 maxRecordsPerBatch() const override
  {
    return m_Delegate->maxRecordsPerBatch();
  }
  bool isReadOnly() const override
  {
    return m_Delegate->isReadOnly();
  }

  /**
   * @brief Delegates one CCL read or injects the selected primary carrier.
   * @param offset First scratch record.
   * @param count Number of requested records.
   * @param records Receives the record bytes.
   * @param cancel Cancellation flag passed to the real store.
   * @return Actual read result or the deliberate primary Result.
   * @throws PlaneFailure For the selected standard primary fault.
   * @throws AllocationFailure For the selected allocation primary fault.
   * @throws NonstandardFailure For the selected nonstandard primary fault.
   */
  Result<uint64> read(uint64 offset, uint64 count, nonstd::span<std::byte> records, const std::atomic_bool& cancel) const override
  {
    ++m_State.recordReads;
    if(!m_State.messagePrimary && m_Plane == m_State.primaryPlane && m_State.primary != FaultKind::None && !m_State.primaryFired)
    {
      m_State.primaryFired = true;
      if(m_State.cancelAt == CancelAt::WithPrimary)
      {
        m_State.cancel = true;
        m_State.cancelFired = true;
      }
      return ConvertResultTo<uint64>(RaiseFault(m_State.primary, true), 0);
    }
    return m_Delegate->read(offset, count, records, cancel);
  }

  /**
   * @brief Delegates a scratch write before setting the selected mid-CCL cancellation.
   * @param offset First scratch record.
   * @param count Number of records.
   * @param records Supplies the record bytes.
   * @param cancel Cancellation flag passed to the real store.
   * @return The real store's Result.
   */
  Result<> write(uint64 offset, uint64 count, nonstd::span<const std::byte> records, const std::atomic_bool& cancel) override
  {
    ++m_State.recordWrites;
    auto result = m_Delegate->write(offset, count, records, cancel);
    if(result.valid() && m_Plane == m_State.primaryPlane && m_State.cancelAt == CancelAt::RecordWrite)
    {
      // Cancel after a real flush, while the unchanged per-plane CCL is still active.
      m_State.cancel = true;
      m_State.cancelFired = true;
    }
    return result;
  }
  /**
   * @brief Passes a fill to the real record store without adding a fault.
   * @param offset First record to fill.
   * @param count Number of records to fill.
   * @param record Supplies one complete record to repeat.
   * @param cancel Cancellation flag passed to the real store.
   * @return The real store's fill Result.
   */
  Result<> fill(uint64 offset, uint64 count, nonstd::span<const std::byte> record, const std::atomic_bool& cancel) override
  {
    return m_Delegate->fill(offset, count, record, cancel);
  }
  /**
   * @brief Passes a size change to the real record store without adding a fault.
   * @param count New logical record count.
   * @param cancel Cancellation flag passed to the real store.
   * @return The real store's resize Result.
   */
  Result<> resize(uint64 count, const std::atomic_bool& cancel) override
  {
    return m_Delegate->resize(count, cancel);
  }

private:
  std::unique_ptr<ITemporaryRecordStore> m_Delegate;
  FailureState& m_State;
  usize m_Plane;
};

/**
 * @class RecordManager
 * @brief Supplies real resident records behind a fault decorator for this one execution.
 */
class RecordManager : public IDataIOManager
{
public:
  /**
   * @brief Borrows the execution state used by all created record decorators.
   * @param state Receives provider counts and supplies faults; outlives this manager and its stores.
   */
  explicit RecordManager(FailureState& state)
  : m_State(state)
  {
  }
  std::string formatName() const override
  {
    return "IdentifySample test records";
  }
  bool supportsTemporaryRecordStore() const override
  {
    return true;
  }
  /**
   * @brief Creates the actual scratch owner, then assigns this plane's read/write decorator.
   * @param config Production CCL record configuration.
   * @return Decorated scratch store or its original creation error.
   */
  Result<std::unique_ptr<ITemporaryRecordStore>> createTemporaryRecordStore(const TemporaryRecordStoreConfig& config) const override
  {
    auto result = InMemoryTemporaryRecordStore::Create(config);
    if(result.invalid())
    {
      return ConvertInvalidResult<std::unique_ptr<ITemporaryRecordStore>>(std::move(result));
    }
    // With hole filling disabled, each plane creates exactly one equivalence store.
    const usize plane = m_State.recordFactories++;
    return {std::unique_ptr<ITemporaryRecordStore>(new RecordDecorator(std::move(result.value()), m_State, plane))};
  }

private:
  FailureState& m_State;
};

/**
 * @class ScopedRecordManager
 * @brief Restores the exact first map entry after a serial, fixture-only execution.
 * @note Construct all fixtures and filter/pipeline objects before this scope. No unrelated factories may run inside it.
 */
class ScopedRecordManager
{
public:
  /**
   * @brief Replaces the first existing manager entry during one serial execution.
   * @param state Receives provider calls and supplies record faults; outlives this scope.
   * @pre The manager collection is nonempty and no concurrent factory activity occurs.
   */
  explicit ScopedRecordManager(FailureState& state)
  : m_Collection(DataStoreUtilities::GetIOCollection())
  , m_Entry(m_Collection.begin())
  , m_Original(m_Entry->second)
  , m_Replacement(std::make_shared<RecordManager>(state))
  {
    m_Entry->second = m_Replacement;
  }
  /**
   * @brief Restores the exact saved manager shared pointer.
   */
  ~ScopedRecordManager() noexcept
  {
    m_Entry->second = m_Original;
  }
  ScopedRecordManager(const ScopedRecordManager&) = delete;
  ScopedRecordManager& operator=(const ScopedRecordManager&) = delete;
  /**
   * @brief Checks that capability selection starts with this scope's record manager.
   * @return True when the first collection entry still owns the installed manager.
   */
  bool selected() const
  {
    return m_Collection.begin()->second == m_Replacement;
  }

private:
  DataIOCollection& m_Collection;
  DataIOCollection::iterator m_Entry;
  std::shared_ptr<IDataIOManager> m_Original;
  std::shared_ptr<IDataIOManager> m_Replacement;
};

/**
 * @class MaskStore
 * @brief Adds phase-aware faults to real resident mask copies after setup.
 * @tparam T Bool or UInt8 mask element type.
 */
template <class T>
class MaskStore : public DataStore<T>
{
public:
  /**
   * @brief Creates the small resident mask and borrows its transfer/fault state.
   * @param state Supplies faults only after setup arms the store; outlives this store.
   */
  explicit MaskStore(FailureState& state)
  : DataStore<T>(ShapeType{k_Z, k_Y, k_X}, ShapeType{1}, std::optional<T>{})
  , m_State(state)
  {
  }
  /**
   * @brief Reads the mask or injects a witnessed gather/prefix-read fault.
   * @param start First flat mask value.
   * @param values Receives the requested values.
   * @return Real read Result or deliberate publication Result.
   * @throws PlaneFailure For the selected standard copy fault.
   * @throws AllocationFailure For the selected allocation copy fault.
   * @throws NonstandardFailure For the selected nonstandard copy fault.
   */
  Result<> copyIntoBuffer(usize start, nonstd::span<T> values) const override
  {
    if(m_State.armed)
    {
      ++m_State.reads;
      const usize z = start / (k_X * k_Y);
      const bool gather = m_State.recordFactories == 0 && !m_State.primaryFired;
      const CopyPhase phase = gather ? CopyPhase::GatherRead : CopyPhase::CommitRead;
      if((gather && m_State.cancelAt == CancelAt::Gather && z == 1) || (!gather && m_State.cancelAt == CancelAt::CommitRead && z == 0))
      {
        m_State.cancel = true;
        m_State.cancelFired = true;
      }
      if(m_State.secondary != FaultKind::None && phase == m_State.secondaryPhase && z == m_State.secondaryZ && !m_State.secondaryFired)
      {
        m_State.secondaryFired = true;
        if(m_State.cancelAt == CancelAt::WithSecondary)
        {
          m_State.cancel = true;
          m_State.cancelFired = true;
        }
        return RaiseFault(m_State.secondary, false);
      }
    }
    return DataStore<T>::copyIntoBuffer(start, values);
  }
  /**
   * @brief Writes a mask prefix or fails before the selected Z write.
   * @param start First flat mask value.
   * @param values Supplies the replacement values.
   * @return Real write Result or deliberate publication Result.
   * @throws PlaneFailure For the selected standard copy fault.
   * @throws AllocationFailure For the selected allocation copy fault.
   * @throws NonstandardFailure For the selected nonstandard copy fault.
   */
  Result<> copyFromBuffer(usize start, nonstd::span<const T> values) override
  {
    if(m_State.armed)
    {
      ++m_State.writes;
      const usize z = start / (k_X * k_Y);
      if(m_State.secondary != FaultKind::None && m_State.secondaryPhase == CopyPhase::CommitWrite && z == m_State.secondaryZ && !m_State.secondaryFired)
      {
        m_State.secondaryFired = true;
        if(m_State.cancelAt == CancelAt::WithSecondary)
        {
          m_State.cancel = true;
          m_State.cancelFired = true;
        }
        return RaiseFault(m_State.secondary, false);
      }
    }
    auto result = DataStore<T>::copyFromBuffer(start, values);
    if(m_State.armed && result.valid())
    {
      ++m_State.successfulWrites;
    }
    return result;
  }

private:
  FailureState& m_State;
};

/**
 * @brief Places the removable island at Z=0 so an early successful write is visible.
 * @param x Selects the alternating rectangle position for this YZ plane.
 * @param y First coordinate within the plane.
 * @param z Second coordinate within the plane.
 * @return True for the unfilled rectangle or the separate Z=0 island.
 */
bool FailureInput(usize x, usize y, usize z)
{
  return IdentifySampleBatchTest::ExpectedValue(y, z, x, false) || (y == 0 && z == 0);
}

/**
 * @brief Initializes the independent early-island fixture before faults are armed.
 * @tparam T Bool or UInt8 mask type.
 * @param store Reports real mask transfers during execution.
 * @return Geometry and initialized mask.
 */
template <class T>
DataStructure CreateFailureFixture(const std::shared_ptr<MaskStore<T>>& store)
{
  auto data = IdentifySampleBatchTest::CreateFixture<T>(2, k_X, store);
  std::array<T, k_X * k_Y> xy{};
  for(usize z = 0; z < k_Z; ++z)
  {
    for(usize y = 0; y < k_Y; ++y)
    {
      for(usize x = 0; x < k_X; ++x)
      {
        xy[y * k_X + x] = static_cast<T>(FailureInput(x, y, z));
      }
    }
    const auto result = store->copyFromBuffer(z * xy.size(), nonstd::span<const T>(xy.data(), xy.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  return data;
}

/**
 * @brief Compares all cells against only the completed X and successful Z prefixes.
 * @tparam T Bool or UInt8 mask element type.
 * @param data Contains the output mask after the fault controls are disarmed.
 * @param completedPlanes Number of fully classified X planes, starting at zero.
 * @param successfulZ Number of committed Z slices, starting at zero.
 */
template <class T>
void RequirePrefix(const DataStructure& data, usize completedPlanes, usize successfulZ)
{
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  const auto& store = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath).getDataStoreRef();
  std::array<T, k_X * k_Y> xy{};
  for(usize z = 0; z < k_Z; ++z)
  {
    const auto result = store.copyIntoBuffer(z * xy.size(), nonstd::span<T>(xy.data(), xy.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    for(usize y = 0; y < k_Y; ++y)
    {
      for(usize x = 0; x < k_X; ++x)
      {
        const bool expected = x < completedPlanes && z < successfulZ ? IdentifySampleBatchTest::ExpectedValue(y, z, x, false) : FailureInput(x, y, z);
        INFO("xyz=" << x << ',' << y << ',' << z << " completed=" << completedPlanes << " successful Z=" << successfulZ);
        REQUIRE(xy[y * k_X + x] == static_cast<T>(expected));
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(data);
}

/**
 * @brief Selects unfilled YZ classification for the terminal-failure fixtures.
 * @return Slice-by-slice YZ inputs using the fixture geometry and mask paths, with hole filling disabled.
 */
IdentifySampleInputValues Inputs()
{
  IdentifySampleInputValues inputs{};
  inputs.FillHoles = false;
  inputs.InputImageGeometryPath = IdentifySampleBatchTest::k_ImagePath;
  inputs.MaskArrayPath = IdentifySampleBatchTest::k_MaskPath;
  inputs.SliceBySlice = true;
  inputs.SliceBySlicePlaneIndex = 2;
  return inputs;
}

/**
 * @brief Builds an empty, observing, or deliberately failing message callback.
 * @param state Records Slice calls and the selected message/cancel event.
 * @param handler Chooses how Error messages behave.
 * @return Callback with a lifetime bounded by state.
 */
IFilter::MessageHandler Messages(FailureState& state, HandlerKind handler)
{
  if(handler == HandlerKind::Empty && !state.messagePrimary && state.cancelAt != CancelAt::BeforePlane)
  {
    return {};
  }
  return IFilter::MessageHandler{[&state, handler](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Error)
    {
      ++state.errorCalls;
      if(handler == HandlerKind::ErrorThrow)
      {
        throw PlaneFailure(303);
      }
    }
    if(message.type == IFilter::Message::Type::Info && message.message.rfind("Slice ", 0) == 0)
    {
      ++state.infoCalls;
      if(message.message == fmt::format("Slice {}", state.primaryPlane))
      {
        if(state.cancelAt == CancelAt::BeforePlane)
        {
          state.cancel = true;
          state.cancelFired = true;
        }
        if(state.messagePrimary && state.primary != FaultKind::None)
        {
          state.primaryFired = true;
          (void)RaiseFault(state.primary, true);
        }
      }
    }
  }};
}

/**
 * @brief Fails only the selected mask's guarded diagnostic assembly.
 * @param context Points to its FailureState.
 * @param mask Identifies the algorithm input at the real assembly boundary.
 * @throws AllocationFailure When this exact guard is armed.
 */
void FailDiagnostic(void* context, const IDataArray* mask)
{
  auto& state = *static_cast<FailureState*>(context);
  if(mask == state.selected)
  {
    ++state.aggregateCalls;
    if(state.aggregateFailure)
    {
      throw AllocationFailure(404);
    }
  }
}

/**
 * @struct Outcome
 * @brief Captures a lone exception without losing its concrete payload.
 */
struct Outcome
{
  std::optional<Result<>> result;
  FaultKind thrown = FaultKind::None;
  int payload = 0;
};

/**
 * @brief Captures only the test's concrete failure classes; unrelated exceptions remain visible.
 * @tparam Callable Callable returning the algorithm Result.
 * @param callable Performs one target execution.
 * @return Original Result or the selected exception type and payload.
 */
template <class Callable>
Outcome CaptureOutcome(Callable&& callable)
{
  Outcome outcome;
  try
  {
    outcome.result.emplace(callable());
  } catch(const AllocationFailure& error)
  {
    outcome.thrown = FaultKind::Allocation;
    outcome.payload = error.payload;
  } catch(const PlaneFailure& error)
  {
    outcome.thrown = FaultKind::Standard;
    outcome.payload = error.payload;
  } catch(const NonstandardFailure& error)
  {
    outcome.thrown = FaultKind::Nonstandard;
    outcome.payload = error.payload;
  }
  return outcome;
}

/**
 * @brief Requires every observed owner, transfer, and reservation scope to be closed.
 * @param counts Final observations for the selected mask after the target call ends.
 */
void RequireReleased(const LogicalCounters& counts)
{
  REQUIRE_FALSE(counts.invalid);
  REQUIRE_FALSE(counts.invalidExtraAccounting);
  REQUIRE(counts.active == 0);
  REQUIRE(counts.planeLive == 0);
  REQUIRE(counts.xyLive == 0);
  REQUIRE(counts.extraLiveBytes == 0);
  REQUIRE(counts.extraRetainedBytes == 0);
  REQUIRE(counts.allocationEvents == counts.releaseEvents);
  REQUIRE(counts.extraAllocationCount == counts.extraReleaseCount);
  REQUIRE(counts.extraRetainCount == counts.extraReservationReleaseCount);
}

/**
 * @brief Executes one failure fixture with a full first batch and returns its original carrier.
 * @tparam T Bool or UInt8 mask type.
 * @param state Supplies faults and records actual operation witnesses.
 * @param handler Selects the independent callback condition.
 * @param completedPlanes Expected completed X prefix.
 * @param successfulZ Expected successful publication Z prefix.
 * @return Result or concrete lone exception payload.
 */
template <class T>
Outcome RunFailure(FailureState& state, HandlerKind handler, usize completedPlanes, usize successfulZ)
{
  const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  auto store = std::make_shared<MaskStore<T>>(state);
  auto data = CreateFailureFixture<T>(store);
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
  state.selected = state.diagnosticMatch ? &mask : nullptr;
  auto inputs = Inputs();
  auto messages = Messages(state, handler);
  IdentifySampleCCL algorithm(data, messages, state.cancel, &inputs);
  LogicalCounters counts;
  counts.selected = &mask;
  auto& budget = CacheMemoryBudgetManager::instance();
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  const BudgetRestore restoreBudget;
  REQUIRE_FALSE(budget.setBudgetBytes(4ULL * 1024 * 1024));
  const auto occupied = budget.reserveWorkingMemory(budget.maximumWorkingMemoryBytes() - 7 * k_Y * k_Z * sizeof(T));
  REQUIRE(occupied.sizeBytes() == budget.maximumWorkingMemoryBytes() - 7 * k_Y * k_Z * sizeof(T));
  auto& collection = DataStoreUtilities::GetIOCollection();
  REQUIRE(collection.begin() != collection.end());
  const auto originalManager = collection.begin()->second;
  Outcome outcome;
  {
    const auto oldObserver = SetIdentifySampleSliceObserverForTesting({&counts, &LogicalCounters::Observe});
    const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(oldObserver); });
    REQUIRE(oldObserver.callback == nullptr);
    const auto oldDiagnostic = SetIdentifySampleDiagnosticControlForTesting({&state, &FailDiagnostic});
    const auto restoreDiagnostic = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleDiagnosticControlForTesting(oldDiagnostic); });
    REQUIRE(oldDiagnostic.beforeAggregate == nullptr);
    const ScopedRecordManager overrideManager(state);
    REQUIRE(overrideManager.selected());
    state.armed = true;
    const auto disarm = MakeScopeGuard([&]() noexcept { state.armed = false; });
    if(state.cancelAt == CancelAt::Pre)
    {
      state.cancel = true;
      state.cancelFired = true;
    }
    outcome = CaptureOutcome([&] { return algorithm(); });
  }
  REQUIRE(collection.begin()->second == originalManager);
  REQUIRE(mask.getDataStore() == store.get());
  REQUIRE(budget.reservedWorkingMemoryBytes() == occupied.sizeBytes());
  RequireReleased(counts);
  REQUIRE(state.liveRecords == 0);
  REQUIRE(state.errorCalls == 0);
  REQUIRE(state.successfulWrites == successfulZ);
  if(state.cancelAt == CancelAt::Pre)
  {
    REQUIRE(counts.allocationEvents == 0);
    REQUIRE(counts.extraRequestCount == 0);
    REQUIRE(counts.batchCount == 0);
    REQUIRE(state.reads == 0);
    REQUIRE(state.writes == 0);
  }
  else
  {
    REQUIRE(counts.batchCount == 1);
    REQUIRE(counts.maximumBatchWidth == 8);
    REQUIRE(counts.extraPeakBytes == 7 * k_Y * k_Z * sizeof(T));
  }
  RequirePrefix<T>(data, completedPlanes, successfulZ);
  return outcome;
}

/**
 * @brief Maps the scoped fault carrier to the established filter error code.
 * @param kind Selects a returned, standard, allocation, or nonstandard failure.
 * @param primary True for the plane failure, false for the publication failure.
 * @return Original Result code, -272 for allocation failure, or -2 for another exception.
 * @pre kind is not FaultKind::None.
 */
int32 ExpectedCode(FaultKind kind, bool primary)
{
  return kind == FaultKind::Result ? (primary ? k_PrimaryCode : k_SecondaryCode) : (kind == FaultKind::Allocation ? -272 : -2);
}

/**
 * @brief Checks original returned diagnostics and original standard-exception text.
 * @param error Diagnostic returned by the target call.
 * @param kind Identifies the configured failure carrier.
 * @param primary True for the plane failure, false for the publication failure.
 */
void RequireOriginalError(const Error& error, FaultKind kind, bool primary)
{
  REQUIRE(error.code == ExpectedCode(kind, primary));
  if(kind == FaultKind::Result)
  {
    REQUIRE(error.message == (primary ? "primary record Result" : "secondary copy Result"));
  }
  else
  {
    REQUIRE(error.message.find("Mask") != std::string::npos);
    if(kind == FaultKind::Standard)
    {
      REQUIRE(error.message.find(primary ? "primary record payload 101" : "secondary copy payload 202") != std::string::npos);
    }
  }
}

/**
 * @brief Checks both ordered carriers, unchanged warnings, exact locations, and operation witnesses.
 * @param outcome Holds the returned combined Result.
 * @param state Records the configured faults and the actual operations that reached them.
 */
void RequireDual(const Outcome& outcome, const FailureState& state)
{
  REQUIRE(outcome.thrown == FaultKind::None);
  REQUIRE(outcome.result.has_value());
  const auto& result = outcome.result.value();
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 2);
  RequireOriginalError(result.errors()[0], state.primary, true);
  RequireOriginalError(result.errors()[1], state.secondary, false);
  const usize warningCount = static_cast<usize>(state.primary == FaultKind::Result) + static_cast<usize>(state.secondary == FaultKind::Result);
  REQUIRE(result.warnings().size() == warningCount);
  usize warning = 0;
  if(state.primary == FaultKind::Result)
  {
    REQUIRE(result.warnings()[warning].code == k_PrimaryWarning);
    REQUIRE(result.warnings()[warning++].message == "primary warning");
  }
  else
  {
    const auto& message = result.errors()[0].message;
    REQUIRE(message.find(state.messagePrimary ? "reporting plane" : "classifying plane") != std::string::npos);
    REQUIRE(message.find("plane 1") != std::string::npos);
  }
  if(state.secondary == FaultKind::Result)
  {
    REQUIRE(result.warnings()[warning].code == k_SecondaryWarning);
    REQUIRE(result.warnings()[warning].message == "secondary warning");
  }
  else
  {
    const auto& message = result.errors()[1].message;
    REQUIRE(message.find(state.secondaryPhase == CopyPhase::CommitRead ? "reading completed prefix" : "writing completed prefix") != std::string::npos);
    REQUIRE(message.find(fmt::format("starting at plane 0 with 1 planes at Z {}", state.secondaryZ)) != std::string::npos);
  }
  REQUIRE(state.primaryFired);
  REQUIRE(state.secondaryFired);
  REQUIRE(state.aggregateCalls == static_cast<usize>(state.diagnosticMatch));
  REQUIRE(state.recordFactories == (state.messagePrimary ? 1 : 2));
  REQUIRE(state.recordReads >= 1);
  REQUIRE(state.reads == k_Z + state.secondaryZ + 1);
  REQUIRE(state.writes == state.secondaryZ + static_cast<usize>(state.secondaryPhase == CopyPhase::CommitWrite));
}
} // namespace

TEST_CASE("IdentifySample batch retains both terminal failures in primary order", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto primary = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  const auto secondary = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  const auto phase = GENERATE(CopyPhase::CommitRead, CopyPhase::CommitWrite);
  const auto handler = GENERATE(HandlerKind::Empty, HandlerKind::Capturing, HandlerKind::ErrorThrow);
  const bool useBool = GENERATE(false, true);
  DYNAMIC_SECTION("primary=" << static_cast<int>(primary) << " secondary=" << static_cast<int>(secondary) << " phase=" << static_cast<int>(phase) << " handler=" << static_cast<int>(handler)
                             << " Bool=" << useBool)
  {
    FailureState state;
    state.primary = primary;
    state.secondary = secondary;
    state.secondaryPhase = phase;
    const auto outcome = useBool ? RunFailure<bool>(state, handler, 1, 1) : RunFailure<uint8>(state, handler, 1, 1);
    RequireDual(outcome, state);
  }
}

TEST_CASE("IdentifySample batch captures a Slice callback failure before publication", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto primary = GENERATE(FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  const auto secondary = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation);
  FailureState state;
  state.messagePrimary = true;
  state.primary = primary;
  state.secondary = secondary;
  const auto outcome = RunFailure<uint8>(state, HandlerKind::ErrorThrow, 1, 1);
  RequireDual(outcome, state);
  REQUIRE(state.infoCalls == 2);
}

TEST_CASE("IdentifySample batch preserves lone plane failures and skips an empty prefix", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto kind = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  const usize plane = GENERATE(usize{0}, usize{1});
  const bool useBool = GENERATE(false, true);
  FailureState state;
  state.primary = kind;
  state.primaryPlane = plane;
  const auto outcome = useBool ? RunFailure<bool>(state, HandlerKind::Empty, plane, plane == 0 ? 0 : k_Z) : RunFailure<uint8>(state, HandlerKind::Empty, plane, plane == 0 ? 0 : k_Z);
  REQUIRE(state.primaryFired);
  REQUIRE_FALSE(state.secondaryFired);
  REQUIRE(state.recordFactories == plane + 1);
  REQUIRE(state.aggregateCalls == 0);
  REQUIRE(state.reads == (plane == 0 ? k_Z : 2 * k_Z));
  REQUIRE(state.writes == (plane == 0 ? 0 : k_Z));
  if(kind == FaultKind::Result)
  {
    REQUIRE(outcome.result.has_value());
    const auto& result = outcome.result.value();
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    RequireOriginalError(result.errors()[0], kind, true);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings()[0].code == k_PrimaryWarning);
    REQUIRE(result.warnings()[0].message == "primary warning");
  }
  else
  {
    REQUIRE_FALSE(outcome.result.has_value());
    REQUIRE(outcome.thrown == kind);
    REQUIRE(outcome.payload == 101);
  }
}

TEST_CASE("IdentifySample batch preserves lone gather and commit exceptions", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto phase = GENERATE(CopyPhase::GatherRead, CopyPhase::CommitRead, CopyPhase::CommitWrite);
  const auto kind = GENERATE(FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  FailureState state;
  state.secondary = kind;
  state.secondaryPhase = phase;
  state.secondaryZ = 0;
  const auto outcome = RunFailure<uint8>(state, HandlerKind::Empty, phase == CopyPhase::GatherRead ? 0 : 8, 0);
  REQUIRE_FALSE(outcome.result.has_value());
  REQUIRE(outcome.thrown == kind);
  REQUIRE(outcome.payload == 202);
  REQUIRE(state.secondaryFired);
  REQUIRE_FALSE(state.primaryFired);
  REQUIRE(state.aggregateCalls == 0);
  REQUIRE(state.recordFactories == (phase == CopyPhase::GatherRead ? 0 : 8));
  REQUIRE(state.reads == (phase == CopyPhase::GatherRead ? 1 : k_Z + 1));
  REQUIRE(state.writes == static_cast<usize>(phase == CopyPhase::CommitWrite));
}

TEST_CASE("IdentifySample batch preserves primary when diagnostic assembly fails", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto primary = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  const auto secondary = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation);
  const auto handler = GENERATE(HandlerKind::Empty, HandlerKind::ErrorThrow);
  FailureState state;
  state.primary = primary;
  state.secondary = secondary;
  state.aggregateFailure = true;
  const auto outcome = RunFailure<uint8>(state, handler, 1, 1);
  REQUIRE(state.primaryFired);
  REQUIRE(state.secondaryFired);
  REQUIRE(state.aggregateCalls == 1);
  REQUIRE(state.recordFactories == 2);
  REQUIRE(state.reads == k_Z + 2);
  REQUIRE(state.writes == 2);
  if(primary == FaultKind::Result)
  {
    REQUIRE(outcome.result.has_value());
    const auto& result = outcome.result.value();
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    RequireOriginalError(result.errors()[0], primary, true);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings()[0].code == k_PrimaryWarning);
    REQUIRE(result.warnings()[0].message == "primary warning");
  }
  else
  {
    REQUIRE_FALSE(outcome.result.has_value());
    REQUIRE(outcome.thrown == primary);
    REQUIRE(outcome.payload == 101);
  }
}

TEST_CASE("IdentifySample batch cancellation publishes only completed planes", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto point = GENERATE(CancelAt::Pre, CancelAt::Gather, CancelAt::BeforePlane, CancelAt::RecordWrite, CancelAt::CommitRead);
  const bool useBool = GENERATE(false, true);
  const usize completed = point == CancelAt::CommitRead ? 8 : (point == CancelAt::BeforePlane || point == CancelAt::RecordWrite ? 1 : 0);
  FailureState state;
  state.cancelAt = point;
  const auto outcome =
      useBool ? RunFailure<bool>(state, HandlerKind::Capturing, completed, completed == 0 ? 0 : k_Z) : RunFailure<uint8>(state, HandlerKind::Capturing, completed, completed == 0 ? 0 : k_Z);
  REQUIRE(state.cancelFired);
  REQUIRE(outcome.result.has_value());
  SIMPLNX_RESULT_REQUIRE_VALID(outcome.result.value());
  REQUIRE(state.aggregateCalls == 0);
  REQUIRE_FALSE(state.primaryFired);
  REQUIRE_FALSE(state.secondaryFired);
  REQUIRE(state.recordFactories == (point == CancelAt::RecordWrite ? 2 : completed));
  if(point == CancelAt::Gather)
  {
    // Cancellation set by the Z=1 read must stop before another gather read.
    REQUIRE(state.reads == 2);
    REQUIRE(state.writes == 0);
  }
  if(point == CancelAt::RecordWrite)
  {
    REQUIRE(state.recordWrites == 2);
  }
}

TEST_CASE("IdentifySample batch failures take precedence over cancellation", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto cancelAt = GENERATE(CancelAt::WithPrimary, CancelAt::WithSecondary);
  FailureState state;
  state.primary = FaultKind::Result;
  state.secondary = FaultKind::Standard;
  state.cancelAt = cancelAt;
  const auto outcome = RunFailure<uint8>(state, HandlerKind::Empty, 1, 1);
  REQUIRE(state.cancelFired);
  RequireDual(outcome, state);
}

TEST_CASE("IdentifySample lone Slice failure still publishes the completed prefix", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto kind = GENERATE(FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  const usize plane = GENERATE(usize{0}, usize{1});
  FailureState state;
  state.primary = kind;
  state.messagePrimary = true;
  state.primaryPlane = plane;
  const auto outcome = RunFailure<uint8>(state, HandlerKind::Capturing, plane, plane == 0 ? 0 : k_Z);
  REQUIRE(state.primaryFired);
  REQUIRE_FALSE(outcome.result.has_value());
  REQUIRE(outcome.thrown == kind);
  REQUIRE(outcome.payload == 101);
  REQUIRE(state.recordFactories == plane);
  REQUIRE(state.infoCalls == plane + 1);
  REQUIRE(state.aggregateCalls == 0);
}

TEST_CASE("IdentifySample cancellation cannot hide a lone publication failure", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto kind = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation, FaultKind::Nonstandard);
  FailureState state;
  state.secondary = kind;
  state.secondaryPhase = CopyPhase::CommitWrite;
  state.cancelAt = CancelAt::CommitRead;
  const auto outcome = RunFailure<uint8>(state, HandlerKind::Empty, 8, 1);
  REQUIRE(state.cancelFired);
  REQUIRE(state.secondaryFired);
  REQUIRE_FALSE(state.primaryFired);
  REQUIRE(state.recordFactories == 8);
  REQUIRE(state.aggregateCalls == 0);
  REQUIRE(state.reads == k_Z + 2);
  REQUIRE(state.writes == 2);
  if(kind == FaultKind::Result)
  {
    REQUIRE(outcome.result.has_value());
    const auto& result = outcome.result.value();
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    RequireOriginalError(result.errors()[0], kind, false);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings()[0].code == k_SecondaryWarning);
    REQUIRE(result.warnings()[0].message == "secondary warning");
  }
  else
  {
    REQUIRE_FALSE(outcome.result.has_value());
    REQUIRE(outcome.thrown == kind);
    REQUIRE(outcome.payload == 202);
  }
}

namespace
{
/**
 * @struct AllocationControl
 * @brief Selects the real extra-allocation call to fail, after its reservation exists.
 * @note These fixtures use one-byte mask values, so requested values also give the required byte count.
 */
struct AllocationControl
{
  const IDataArray* selected = nullptr;
  usize calls = 0;
  usize failOn = 1;
  usize lastValues = 0;
  uint64 occupied = 0;
  bool reservationMissing = false;

  /**
   * @brief Checks the real reservation before failing the selected extra allocation.
   * @param context Points to the live AllocationControl for this execution.
   * @param mask Identifies the array passed by the algorithm.
   * @param values Number of extra mask values about to be allocated.
   * @throws AllocationFailure When the matching array reaches failOn.
   */
  static void Before(void* context, const IDataArray* mask, usize values)
  {
    auto& control = *static_cast<AllocationControl*>(context);
    if(mask != control.selected)
    {
      return;
    }
    ++control.calls;
    control.lastValues = values;
    control.reservationMissing = control.reservationMissing || CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() != control.occupied + values;
    if(control.calls == control.failOn)
    {
      throw AllocationFailure(505);
    }
  }
};

/**
 * @struct OwnerObservations
 * @brief Checks actual manager state at both extra-owner and token release observations.
 */
struct OwnerObservations
{
  LogicalCounters counts;
  uint64 occupied = 0;
  bool invalidOrder = false;

  /**
   * @brief Checks release order against live manager bytes, then records the logical event.
   * @param context Points to the live OwnerObservations for this execution.
   * @param mask Identifies the array that produced the event.
   * @param event Identifies a transfer, owner, or reservation boundary.
   * @param values Number of elements, bytes, or planes as defined by event.
   * @param elementBytes Converts values to bytes; admission and width events use one.
   * @param success Reports transfer completion; lifetime events require true.
   */
  static void Observe(void* context, const IDataArray* mask, IdentifySampleSliceEventForTesting event, usize values, usize elementBytes, bool success) noexcept
  {
    auto& observation = *static_cast<OwnerObservations*>(context);
    if(mask != observation.counts.selected)
    {
      return;
    }
    const uint64 reserved = CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes();
    if(event == IdentifySampleSliceEventForTesting::ExtraPlanesReleased)
    {
      observation.invalidOrder = observation.invalidOrder || reserved != observation.occupied + values * elementBytes;
    }
    if(event == IdentifySampleSliceEventForTesting::ExtraReservationReleased)
    {
      observation.invalidOrder = observation.invalidOrder || reserved != observation.occupied || observation.counts.extraLiveBytes != 0;
    }
    LogicalCounters::Observe(&observation.counts, mask, event, values, elementBytes, success);
  }
};

/**
 * @brief Verifies the reservation and actual-owner order on early and later allocation failures.
 * @tparam T Bool or UInt8 mask type.
 * @param failOn One-based real extra-allocation call to fail.
 * @param matchIdentity True to arm the selected mask; false to prove identity filtering.
 * @param hasHeadroom True to admit eight planes; false to prove the hook is skipped.
 */
template <class T>
void CheckAllocationFailure(usize failOn, bool matchIdentity, bool hasHeadroom = true)
{
  static_assert(sizeof(T) == 1);
  const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  FailureState state;
  auto store = std::make_shared<MaskStore<T>>(state);
  auto data = CreateFailureFixture<T>(store);
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
  auto inputs = Inputs();
  const IFilter::MessageHandler messages;
  IdentifySampleCCL algorithm(data, messages, state.cancel, &inputs);
  AllocationControl control;
  control.selected = matchIdentity ? &mask : nullptr;
  control.failOn = failOn;
  OwnerObservations observations;
  observations.counts.selected = &mask;
  auto& budget = CacheMemoryBudgetManager::instance();
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  const BudgetRestore restoreBudget;
  REQUIRE_FALSE(budget.setBudgetBytes(4ULL * 1024 * 1024));
  const uint64 extraBytes = hasHeadroom ? 7 * k_Y * k_Z * sizeof(T) : 0;
  auto occupied = budget.reserveWorkingMemory(budget.maximumWorkingMemoryBytes() - extraBytes);
  REQUIRE(occupied.sizeBytes() == budget.maximumWorkingMemoryBytes() - extraBytes);
  observations.occupied = occupied.sizeBytes();
  control.occupied = occupied.sizeBytes();
  Outcome outcome;
  {
    const auto old = SetIdentifySampleExtraAllocationControlForTesting({&control, &AllocationControl::Before});
    const auto restore = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleExtraAllocationControlForTesting(old); });
    REQUIRE(old.beforeAllocate == nullptr);
    const auto oldObserver = SetIdentifySampleSliceObserverForTesting({&observations, &OwnerObservations::Observe});
    const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(oldObserver); });
    REQUIRE(oldObserver.callback == nullptr);
    state.armed = true;
    const auto disarm = MakeScopeGuard([&]() noexcept { state.armed = false; });
    outcome = CaptureOutcome([&] { return algorithm(); });
  }
  REQUIRE(budget.reservedWorkingMemoryBytes() == occupied.sizeBytes());
  REQUIRE_FALSE(control.reservationMissing);
  REQUIRE_FALSE(observations.invalidOrder);
  RequireReleased(observations.counts);
  if(matchIdentity && hasHeadroom)
  {
    REQUIRE_FALSE(outcome.result.has_value());
    REQUIRE(outcome.thrown == FaultKind::Allocation);
    REQUIRE(outcome.payload == 505);
    REQUIRE(control.calls == failOn);
    REQUIRE(control.lastValues == 7 * k_Y * k_Z);
    REQUIRE(observations.counts.extraRetainCount == failOn);
    REQUIRE(observations.counts.extraAllocationCount == failOn - 1);
    REQUIRE(observations.counts.batchCount == failOn - 1);
    REQUIRE(state.reads == (failOn - 1) * 2 * k_Z);
    REQUIRE(state.writes == (failOn - 1) * k_Z);
    RequirePrefix<T>(data, (failOn - 1) * 8, failOn == 1 ? 0 : k_Z);
  }
  else
  {
    REQUIRE(outcome.result.has_value());
    SIMPLNX_RESULT_REQUIRE_VALID(outcome.result.value());
    REQUIRE(control.calls == 0);
    REQUIRE(observations.counts.extraAllocationCount == (hasHeadroom ? 3 : 0));
    REQUIRE(observations.counts.batchCount == (hasHeadroom ? 3 : k_X));
    RequirePrefix<T>(data, k_X, k_Z);
  }
}
} // namespace

TEST_CASE("IdentifySample extra allocation failure releases owners before reservations", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const usize failOn = GENERATE(usize{1}, usize{2});
  const bool useBool = GENERATE(false, true);
  const bool matchIdentity = GENERATE(false, true);
  if(useBool)
  {
    CheckAllocationFailure<bool>(failOn, matchIdentity);
  }
  else
  {
    CheckAllocationFailure<uint8>(failOn, matchIdentity);
  }
}

TEST_CASE("IdentifySample allocation and diagnostic controls restore nested and thread state", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  AllocationControl outer;
  AllocationControl inner;
  FailureState outerDiagnostic;
  FailureState innerDiagnostic;
  const auto originalAllocation = SetIdentifySampleExtraAllocationControlForTesting({&outer, &AllocationControl::Before});
  const auto restoreAllocation = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleExtraAllocationControlForTesting(originalAllocation); });
  const auto originalDiagnostic = SetIdentifySampleDiagnosticControlForTesting({&outerDiagnostic, &FailDiagnostic});
  const auto restoreDiagnostic = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleDiagnosticControlForTesting(originalDiagnostic); });
  REQUIRE(originalAllocation.context == nullptr);
  REQUIRE(originalAllocation.beforeAllocate == nullptr);
  REQUIRE(originalDiagnostic.context == nullptr);
  REQUIRE(originalDiagnostic.beforeAggregate == nullptr);
  {
    const auto priorAllocation = SetIdentifySampleExtraAllocationControlForTesting({&inner, &AllocationControl::Before});
    const auto restoreInnerAllocation = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleExtraAllocationControlForTesting(priorAllocation); });
    const auto priorDiagnostic = SetIdentifySampleDiagnosticControlForTesting({&innerDiagnostic, &FailDiagnostic});
    const auto restoreInnerDiagnostic = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleDiagnosticControlForTesting(priorDiagnostic); });
    REQUIRE(priorAllocation.context == &outer);
    REQUIRE(priorAllocation.beforeAllocate == &AllocationControl::Before);
    REQUIRE(priorDiagnostic.context == &outerDiagnostic);
    REQUIRE(priorDiagnostic.beforeAggregate == &FailDiagnostic);
  }
  std::atomic_bool childWasNull = false;
  std::thread child([&] {
    const auto allocation = SetIdentifySampleExtraAllocationControlForTesting({});
    const auto diagnostic = SetIdentifySampleDiagnosticControlForTesting({});
    childWasNull = allocation.context == nullptr && allocation.beforeAllocate == nullptr && diagnostic.context == nullptr && diagnostic.beforeAggregate == nullptr;
    (void)SetIdentifySampleExtraAllocationControlForTesting(allocation);
    (void)SetIdentifySampleDiagnosticControlForTesting(diagnostic);
  });
  child.join();
  REQUIRE(childWasNull.load());
  const auto currentAllocation = SetIdentifySampleExtraAllocationControlForTesting({});
  const auto currentDiagnostic = SetIdentifySampleDiagnosticControlForTesting({});
  REQUIRE(currentAllocation.context == &outer);
  REQUIRE(currentAllocation.beforeAllocate == &AllocationControl::Before);
  REQUIRE(currentDiagnostic.context == &outerDiagnostic);
  REQUIRE(currentDiagnostic.beforeAggregate == &FailDiagnostic);
}

TEST_CASE("IdentifySample checked slice dimensions fail before owners and transfers", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  constexpr usize k_SignedMaximum = static_cast<usize>(std::numeric_limits<int64>::max());
  /**
   * @struct InvalidDimensions
   * @brief Pairs oversized geometry metadata with its first reachable error code.
   */
  struct InvalidDimensions
  {
    SizeVec3 dimensions;
    int32 code;
  };
  const usize labelByteBound = std::numeric_limits<usize>::max() / sizeof(int64);
  std::vector<InvalidDimensions> cases{{{std::numeric_limits<usize>::max(), 1, 1}, -45464}, {{k_SignedMaximum, 3, 1}, -45463},         {{1, k_SignedMaximum / 2 + 1, 1}, -45464},
                                       {{k_SignedMaximum / 2 + 1, 2, 1}, -45465},           {{8, 1, k_SignedMaximum / 7 + 1}, -45465}, {{1, labelByteBound / 2 + 1, 1}, -45463}};
  const usize vectorBound = std::vector<int64>().max_size();
  REQUIRE(vectorBound <= labelByteBound);
  const usize overCapacityRowWidth = vectorBound / 2 + 1;
  const bool vectorCapacityReachable = overCapacityRowWidth * 2 <= labelByteBound;
  if(vectorCapacityReachable)
  {
    // Some standard libraries impose a lower vector bound than byte arithmetic does.
    cases.push_back({{1, overCapacityRowWidth, 1}, -45465});
  }
  else
  {
    INFO("Vector capacity rejection is preceded by checked label-byte overflow on this standard library");
  }
  CAPTURE(vectorBound, labelByteBound, vectorCapacityReachable);
  const auto value = GENERATE_COPY(from_range(cases));
  FailureState state;
  auto store = std::make_shared<MaskStore<uint8>>(state);
  auto data = CreateFailureFixture<uint8>(store);
  REQUIRE_NOTHROW(data.getDataRefAs<ImageGeom>(IdentifySampleBatchTest::k_ImagePath));
  auto& image = data.getDataRefAs<ImageGeom>(IdentifySampleBatchTest::k_ImagePath);
  const auto dimensions = image.getDimensions();
  const auto restoreDimensions = MakeScopeGuard([&]() noexcept { image.setDimensions(dimensions); });
  image.setDimensions(value.dimensions);
  auto inputs = Inputs();
  const IFilter::MessageHandler messages;
  IdentifySampleCCL algorithm(data, messages, state.cancel, &inputs);
  LogicalCounters counts;
  REQUIRE_NOTHROW(data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath));
  counts.selected = &data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath);
  const uint64 reserved = CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes();
  {
    const auto prior = SetIdentifySampleSliceObserverForTesting({&counts, &LogicalCounters::Observe});
    const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
    state.armed = true;
    const auto disarm = MakeScopeGuard([&]() noexcept { state.armed = false; });
    const auto result = algorithm();
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors()[0].code == value.code);
    REQUIRE(result.errors()[0].message.find("Mask") != std::string::npos);
    REQUIRE((result.errors()[0].message.find("bound") != std::string::npos || result.errors()[0].message.find("supported") != std::string::npos ||
             result.errors()[0].message.find("must fit") != std::string::npos));
  }
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == reserved);
  REQUIRE(state.reads == 0);
  REQUIRE(state.writes == 0);
  REQUIRE(counts.allocationEvents == 0);
  REQUIRE(counts.extraRequestCount == 0);
  RequireReleased(counts);
  image.setDimensions(dimensions);
  RequirePrefix<uint8>(data, 0, 0);
}

TEST_CASE("IdentifySample safe zero dimensions retain valid empty work", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const usize zeroAxis = GENERATE(usize{0}, usize{1}, usize{2});
  const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  FailureState state;
  auto store = std::make_shared<MaskStore<uint8>>(state);
  auto data = CreateFailureFixture<uint8>(store);
  REQUIRE_NOTHROW(data.getDataRefAs<ImageGeom>(IdentifySampleBatchTest::k_ImagePath));
  auto& image = data.getDataRefAs<ImageGeom>(IdentifySampleBatchTest::k_ImagePath);
  const auto original = image.getDimensions();
  auto dimensions = original;
  dimensions[zeroAxis] = 0;
  const auto restoreDimensions = MakeScopeGuard([&]() noexcept { image.setDimensions(original); });
  image.setDimensions(dimensions);
  auto inputs = Inputs();
  const IFilter::MessageHandler messages;
  IdentifySampleCCL algorithm(data, messages, state.cancel, &inputs);
  LogicalCounters counts;
  REQUIRE_NOTHROW(data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath));
  counts.selected = &data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath);
  {
    const auto prior = SetIdentifySampleSliceObserverForTesting({&counts, &LogicalCounters::Observe});
    const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
    state.armed = true;
    const auto disarm = MakeScopeGuard([&]() noexcept { state.armed = false; });
    const auto result = algorithm();
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  RequireReleased(counts);
  REQUIRE(counts.extraAllocationCount == 0);
  if(zeroAxis == 0)
  {
    REQUIRE(counts.allocationEvents == 0);
    REQUIRE(counts.batchCount == 0);
    REQUIRE(state.reads == 0);
    REQUIRE(state.writes == 0);
  }
  else
  {
    REQUIRE(counts.batchCount == k_X);
    REQUIRE(counts.minimumBatchWidth == 1);
    REQUIRE(counts.maximumBatchWidth == 1);
  }
  image.setDimensions(original);
  RequirePrefix<uint8>(data, 0, 0);
}

TEST_CASE("IdentifySample diagnostic control ignores another mask identity", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  FailureState state;
  state.primary = FaultKind::Result;
  state.secondary = FaultKind::Standard;
  state.aggregateFailure = true;
  state.diagnosticMatch = false;
  const auto outcome = RunFailure<uint8>(state, HandlerKind::Empty, 1, 1);
  RequireDual(outcome, state);
  REQUIRE(state.aggregateCalls == 0);
}

namespace
{
/**
 * @brief Checks actual filter and pipeline Result storage with a dispatch witness.
 * @tparam T Bool or UInt8 mask type.
 * @param scenario Standard resident algorithm/store scenario.
 * @param pipeline True to inspect a PipelineFilter's stored error collection.
 * @param primary Selects a returned, standard, or allocation primary failure.
 * @param dual True to also fail publication after one successful Z write.
 */
template <class T>
void CheckCaller(UnitTest::AlgorithmTestScenario scenario, bool pipeline, FaultKind primary, bool dual)
{
  const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool expectCcl = scenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore;
  FailureState state;
  state.primary = expectCcl ? primary : FaultKind::None;
  state.secondary = expectCcl && dual ? FaultKind::Standard : FaultKind::None;
  if(expectCcl && dual)
  {
    state.cancelAt = CancelAt::WithPrimary;
  }
  auto store = std::make_shared<MaskStore<T>>(state);
  auto data = CreateFailureFixture<T>(store);
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
  scope.requireExpectedStore(mask);
  IdentifySampleFilter filter;
  const auto args = IdentifySampleBatchTest::ArgumentsFor(2, false);
  Pipeline owningPipeline;
  auto node = std::make_shared<PipelineFilter>(std::make_unique<IdentifySampleFilter>(), args);
  REQUIRE(owningPipeline.push_back(node));
  REQUIRE(node->getParentPipeline() == &owningPipeline);
  LogicalCounters counts;
  counts.selected = &mask;
  auto& budget = CacheMemoryBudgetManager::instance();
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  const BudgetRestore restoreBudget;
  REQUIRE_FALSE(budget.setBudgetBytes(4ULL * 1024 * 1024));
  auto occupied = budget.reserveWorkingMemory(budget.maximumWorkingMemoryBytes() - 7 * k_Y * k_Z * sizeof(T));
  REQUIRE(occupied.sizeBytes() == budget.maximumWorkingMemoryBytes() - 7 * k_Y * k_Z * sizeof(T));
  auto& collection = DataStoreUtilities::GetIOCollection();
  REQUIRE(collection.begin() != collection.end());
  const auto originalManager = collection.begin()->second;
  ErrorCollection errors;
  WarningCollection warnings;
  {
    const auto prior = SetIdentifySampleSliceObserverForTesting({&counts, &LogicalCounters::Observe});
    const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
    const ScopedRecordManager overrideManager(state);
    REQUIRE(overrideManager.selected());
    state.armed = true;
    const auto disarm = MakeScopeGuard([&]() noexcept { state.armed = false; });
    if(pipeline)
    {
      const bool success = scope.execute([&] { return node->execute(data, state.cancel); });
      REQUIRE(success == !expectCcl);
      errors = node->getErrors();
      warnings = node->getWarnings();
    }
    else
    {
      const auto result = scope.executeFilter(filter, data, args, nullptr, IFilter::MessageHandler{}, state.cancel);
      REQUIRE(result.result.valid() == !expectCcl);
      if(result.result.invalid())
      {
        errors = result.result.errors();
      }
      warnings = result.result.warnings();
    }
  }
  REQUIRE(collection.begin()->second == originalManager);
  REQUIRE(mask.getDataStore() == store.get());
  REQUIRE(budget.reservedWorkingMemoryBytes() == occupied.sizeBytes());
  RequireReleased(counts);
  REQUIRE(state.liveRecords == 0);
  if(expectCcl)
  {
    REQUIRE(state.primaryFired);
    REQUIRE(state.secondaryFired == dual);
    REQUIRE(state.recordFactories == 2);
    REQUIRE(errors.size() == (dual ? 2 : 1));
    REQUIRE(errors[0].code == ExpectedCode(primary, true));
    if(primary == FaultKind::Result)
    {
      REQUIRE(errors[0].message == "primary record Result");
      REQUIRE(warnings.size() == 1);
      REQUIRE(warnings[0].code == k_PrimaryWarning);
      REQUIRE(warnings[0].message == "primary warning");
    }
    else
    {
      REQUIRE(warnings.empty());
      if(primary == FaultKind::Standard)
      {
        REQUIRE(errors[0].message.find("primary record payload 101") != std::string::npos);
      }
    }
    if(dual)
    {
      REQUIRE(state.cancelFired);
      REQUIRE(errors[1].code == -2);
      REQUIRE(errors[1].message.find("secondary copy payload 202") != std::string::npos);
      REQUIRE(errors[1].message.find("writing completed prefix starting at plane 0 with 1 planes at Z 1") != std::string::npos);
    }
    REQUIRE(state.writes == (dual ? 2 : k_Z));
    RequirePrefix<T>(data, 1, dual ? 1 : k_Z);
  }
  else
  {
    REQUIRE(errors.empty());
    REQUIRE(warnings.empty());
    REQUIRE_FALSE(state.primaryFired);
    REQUIRE_FALSE(state.secondaryFired);
    REQUIRE(state.recordFactories == 0);
    RequirePrefix<T>(data, k_X, k_Z);
  }
}
} // namespace

TEST_CASE("IdentifySample filter and pipeline retain failure Results without Output callbacks", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool pipeline = GENERATE(false, true);
  const auto primary = GENERATE(FaultKind::Result, FaultKind::Standard, FaultKind::Allocation);
  const bool dual = GENERATE(false, true);
  const bool useBool = GENERATE(false, true);
  DYNAMIC_SECTION("scenario=" << scenario << " pipeline=" << pipeline << " primary=" << static_cast<int>(primary) << " dual=" << dual << " Bool=" << useBool)
  {
    if(useBool)
    {
      CheckCaller<bool>(scenario, pipeline, primary, dual);
    }
    else
    {
      CheckCaller<uint8>(scenario, pipeline, primary, dual);
    }
  }
}

TEST_CASE("IdentifySample zero extra grant does not call its allocation control", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  CheckAllocationFailure<uint8>(1, true, false);
}

TEST_CASE("IdentifySample orthogonal CCL owns only its used plane carrier", "[.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const auto plane = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  const bool useBool = GENERATE(false, true);
  const bool fillHoles = GENERATE(false, true);
  const auto check = [&]<class T>() {
    const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
    const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
    auto data = IdentifySampleBatchTest::CreateFixture<T>(plane);
    REQUIRE_NOTHROW(data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath));
    LogicalCounters counts;
    counts.selected = &data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath);
    auto inputs = Inputs();
    inputs.SliceBySlicePlaneIndex = plane;
    inputs.FillHoles = fillHoles;
    const IFilter::MessageHandler messages;
    const std::atomic_bool cancel = false;
    IdentifySampleCCL algorithm(data, messages, cancel, &inputs);
    {
      const auto prior = SetIdentifySampleSliceObserverForTesting({&counts, &LogicalCounters::Observe});
      const auto restore = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
      const auto result = algorithm();
      SIMPLNX_RESULT_REQUIRE_VALID(result);
    }
    RequireReleased(counts);
    REQUIRE(counts.allocationEvents == 1);
    REQUIRE(counts.namedCarrierPeak == 11 * 13 * sizeof(T));
    REQUIRE(counts.extraRequestCount == 0);
    REQUIRE(counts.batchCount == 0);
    REQUIRE(counts.reads == (plane == 0 ? 19 : 19 * 13));
    REQUIRE(counts.writes == counts.reads);
    REQUIRE(counts.readSuccess == counts.reads);
    REQUIRE(counts.writeSuccess == counts.writes);
    IdentifySampleBatchTest::RequireOutput<T>(data, plane, fillHoles);
  };
  if(useBool)
  {
    check.template operator()<bool>();
  }
  else
  {
    check.template operator()<uint8>();
  }
}

TEST_CASE("IdentifySample filter pre-cancellation stops before algorithm dispatch", "[SimplnxCore][IdentifySampleFilter][.IdentifySampleObservation][.IdentifySampleBatchFailures]")
{
  UnitTest::LoadPlugins();
  const bool useBool = GENERATE(false, true);
  const auto check = [&]<class T>() {
    const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
    const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
    FailureState state;
    auto store = std::make_shared<MaskStore<T>>(state);
    auto data = CreateFailureFixture<T>(store);
    REQUIRE_NOTHROW(data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath));
    auto& mask = data.getDataRefAs<IDataArray>(IdentifySampleBatchTest::k_MaskPath);
    LogicalCounters counts;
    counts.selected = &mask;
    IdentifySampleFilter filter;
    const auto args = IdentifySampleBatchTest::ArgumentsFor(2, false);
    const uint64 reserved = CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes();
    {
      const auto prior = SetIdentifySampleSliceObserverForTesting({&counts, &LogicalCounters::Observe});
      const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
      const auto priorDispatch = GetAlgorithmPathExecutionCounts();
      const auto restoreDispatch = MakeScopeGuard([&]() noexcept { SetAlgorithmPathExecutionCounts(priorDispatch); });
      ResetAlgorithmPathExecutionCounts();
      state.armed = true;
      state.cancel = true;
      const auto disarm = MakeScopeGuard([&]() noexcept { state.armed = false; });
      // Preflight rejects this call before the selector, so no execution witness is required.
      const auto result = filter.execute(data, args, nullptr, IFilter::MessageHandler{}, state.cancel);
      SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
      REQUIRE(result.result.errors().size() == 1);
      REQUIRE(result.result.errors()[0].code == -1);
      REQUIRE(result.result.warnings().empty());
      const auto dispatch = GetAlgorithmPathExecutionCounts();
      REQUIRE(dispatch.InCore == 0);
      REQUIRE(dispatch.OutOfCore == 0);
      REQUIRE(dispatch.InCoreOnInMemoryStore == 0);
      REQUIRE(dispatch.InCoreOnOutOfCoreStore == 0);
      REQUIRE(dispatch.OutOfCoreOnInMemoryStore == 0);
      REQUIRE(dispatch.OutOfCoreOnOutOfCoreStore == 0);
    }
    REQUIRE(mask.getIDataStore() == store.get());
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == reserved);
    REQUIRE(state.reads == 0);
    REQUIRE(state.writes == 0);
    REQUIRE(counts.allocationEvents == 0);
    REQUIRE(counts.extraRequestCount == 0);
    RequireReleased(counts);
    RequirePrefix<T>(data, 0, 0);
  };
  if(useBool)
  {
    check.template operator()<bool>();
  }
  else
  {
    check.template operator()<uint8>();
  }
}
