#include "IdentifySampleBenchmarkUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/IdentifySample.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataStore.hpp"

#include <atomic>
#include <thread>

using namespace nx::core;

namespace
{
using namespace IdentifySampleBenchmarkTest;

/**
 * @enum CopyFault
 * @brief Selects the first armed copy operation and its failure carrier.
 */
enum class CopyFault
{
  None,        ///< Completes all mask copies normally.
  ReadResult,  ///< Returns an error from the first armed read.
  WriteResult, ///< Returns an error from the first armed write.
  ReadThrow,   ///< Throws from the first armed read.
  WriteThrow   ///< Throws from the first armed write.
};

/**
 * @class FaultStore
 * @brief Injects a result or exception at the first actual copy after fixture initialization.
 * @tparam T Bool or UInt8 resident mask type.
 */
template <class T>
class FaultStore : public DataStore<T>
{
public:
  /**
   * @brief Creates the fixed resident mask used by the first-copy controls.
   */
  FaultStore()
  : DataStore<T>(ShapeType{13, 11, 19}, ShapeType{1}, std::optional<T>{})
  {
  }

  CopyFault fault = CopyFault::None;
  bool armed = false;
  mutable uint64 reads = 0;
  uint64 writes = 0;

  /**
   * @brief Delegates one read or injects the armed first-read failure.
   * @param start First flat value index.
   * @param values Destination values.
   * @return Original copy Result or the deliberate test error.
   * @throws std::runtime_error For the armed first-read exception.
   */
  Result<> copyIntoBuffer(usize start, nonstd::span<T> values) const override
  {
    if(armed)
    {
      reads++;
      if(reads == 1 && fault == CopyFault::ReadResult)
      {
        return MakeErrorResult(-77881, "Deliberate first-copy read failure in Identify Sample observer control");
      }
      if(reads == 1 && fault == CopyFault::ReadThrow)
      {
        throw std::runtime_error("Deliberate first-copy read exception in Identify Sample observer control");
      }
    }
    return DataStore<T>::copyIntoBuffer(start, values);
  }

  /**
   * @brief Delegates one write or injects the armed first-write failure.
   * @param start First flat value index.
   * @param values Source values.
   * @return Original copy Result or the deliberate test error.
   * @throws std::runtime_error For the armed first-write exception.
   */
  Result<> copyFromBuffer(usize start, nonstd::span<const T> values) override
  {
    if(armed)
    {
      writes++;
      if(writes == 1 && fault == CopyFault::WriteResult)
      {
        return MakeErrorResult(-77882, "Deliberate first-copy write failure in Identify Sample observer control");
      }
      if(writes == 1 && fault == CopyFault::WriteThrow)
      {
        throw std::runtime_error("Deliberate first-copy write exception in Identify Sample observer control");
      }
    }
    return DataStore<T>::copyFromBuffer(start, values);
  }
};

/**
 * @brief Checks every mask value against the independent unmodified input predicate.
 * @tparam T Bool or UInt8 mask element type.
 * @param data Owns the mask after its copy faults are disarmed.
 */
template <class T>
void RequireOriginal(const DataStructure& data)
{
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  const auto& store = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath).getDataStoreRef();
  auto xy = std::make_unique<T[]>(19 * 11);
  for(usize z = 0; z < 13; z++)
  {
    const auto read = store.copyIntoBuffer(z * 19 * 11, nonstd::span<T>(xy.get(), 19 * 11));
    SIMPLNX_RESULT_REQUIRE_VALID(read);
    for(usize y = 0; y < 11; y++)
    {
      for(usize x = 0; x < 19; x++)
      {
        REQUIRE(xy[y * 19 + x] == static_cast<T>(IdentifySampleBatchTest::InputValue(y, z, x)));
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(data);
}

/**
 * @brief Checks exact one-plane observations with no extra working-memory headroom.
 * @tparam T Bool or UInt8 mask element type.
 * @param fault Selects a first-copy failure or normal completion.
 * @param matchIdentity True to observe this mask; false to test identity filtering.
 * @param installObserver True to install the callback; false to test the null-observer path.
 */
template <class T>
void CheckControl(CopyFault fault, bool matchIdentity, bool installObserver)
{
  const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
  const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
  const BudgetRestore restoreBudget;
  auto& budget = CacheMemoryBudgetManager::instance();
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  REQUIRE_FALSE(budget.setBudgetBytes(4ULL * 1024 * 1024));
  // Keep first-copy controls on their exact one-plane schedule.
  const auto occupied = budget.reserveWorkingMemory(budget.maximumWorkingMemoryBytes());
  REQUIRE(occupied.sizeBytes() == budget.maximumWorkingMemoryBytes());
  auto store = std::make_shared<FaultStore<T>>();
  DataStructure data = IdentifySampleBatchTest::CreateFixture<T>(2, 19, store);
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
  REQUIRE(mask.getDataStore()->getStoreType() == IDataStore::StoreType::InMemory);
  LogicalCounters counters;
  counters.selected = matchIdentity ? &mask : nullptr;
  store->fault = fault;
  IdentifySampleInputValues inputs{};
  inputs.FillHoles = false;
  inputs.InputImageGeometryPath = IdentifySampleBatchTest::k_ImagePath;
  inputs.MaskArrayPath = IdentifySampleBatchTest::k_MaskPath;
  inputs.SliceBySlice = true;
  inputs.SliceBySlicePlaneIndex = 2;
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler messages;
  IdentifySampleCCL algorithm(data, messages, cancel, &inputs);

  const auto initial = SetIdentifySampleSliceObserverForTesting({});
  const auto restoreInitial = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(initial); });
  REQUIRE(initial.callback == nullptr);
  {
    const IdentifySampleSliceObserverForTesting installed = installObserver ? IdentifySampleSliceObserverForTesting{&counters, &LogicalCounters::Observe} : IdentifySampleSliceObserverForTesting{};
    const auto previous = SetIdentifySampleSliceObserverForTesting(installed);
    const auto restore = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(previous); });
    REQUIRE(previous.callback == nullptr);
    store->armed = true;
    const auto disarm = MakeScopeGuard([&]() noexcept { store->armed = false; });
    if(fault == CopyFault::ReadThrow || fault == CopyFault::WriteThrow)
    {
      REQUIRE_THROWS_AS(algorithm(), std::runtime_error);
    }
    else
    {
      const auto result = algorithm();
      if(fault == CopyFault::None)
      {
        SIMPLNX_RESULT_REQUIRE_VALID(result);
      }
      else
      {
        SIMPLNX_RESULT_REQUIRE_INVALID(result);
        REQUIRE(result.errors().size() == 1);
        REQUIRE(result.errors().front().code == (fault == CopyFault::ReadResult ? -77881 : -77882));
      }
    }
  }
  const auto restored = SetIdentifySampleSliceObserverForTesting({});
  REQUIRE(restored.callback == nullptr);
  REQUIRE(restored.context == nullptr);
  REQUIRE_FALSE(counters.invalid);
  REQUIRE(counters.active == 0);
  REQUIRE(counters.planeLive == 0);
  REQUIRE(counters.xyLive == 0);
  if(matchIdentity && installObserver)
  {
    REQUIRE(counters.reads == store->reads);
    REQUIRE(counters.writes == store->writes);
    REQUIRE(counters.concurrent == 1);
    REQUIRE(counters.allocationEvents == 2);
    REQUIRE(counters.releaseEvents == 2);
    REQUIRE(counters.namedCarrierPeak == (11 * 13 + 19 * 11) * sizeof(T));
    if(fault == CopyFault::None)
    {
      REQUIRE(counters.reads == 494);
      REQUIRE(counters.writes == 247);
      REQUIRE(counters.readSuccess == 494);
      REQUIRE(counters.writeSuccess == 247);
    }
    else if(fault == CopyFault::ReadResult || fault == CopyFault::ReadThrow)
    {
      REQUIRE(counters.reads == 1);
      REQUIRE(counters.writes == 0);
      REQUIRE(counters.readSuccess == 0);
      REQUIRE(counters.writeSuccess == 0);
    }
    else
    {
      REQUIRE(counters.reads == 14);
      REQUIRE(counters.writes == 1);
      REQUIRE(counters.readSuccess == 14);
      REQUIRE(counters.writeSuccess == 0);
    }
  }
  else
  {
    REQUIRE(counters.reads == 0);
    REQUIRE(counters.writes == 0);
    REQUIRE(counters.allocationEvents == 0);
    REQUIRE(counters.releaseEvents == 0);
  }
  if(fault == CopyFault::None)
  {
    IdentifySampleBatchTest::RequireOutput<T>(data, 2, false);
  }
  else
  {
    RequireOriginal<T>(data);
  }
}
} // namespace

TEST_CASE("IdentifySample observation preserves results through success and copy failures", "[.IdentifySampleObservation]")
{
  UnitTest::LoadPlugins();
  const auto fault = GENERATE(CopyFault::None, CopyFault::ReadResult, CopyFault::WriteResult, CopyFault::ReadThrow, CopyFault::WriteThrow);
  const bool useBool = GENERATE(false, true);
  CAPTURE(static_cast<int>(fault), useBool);
  if(useBool)
  {
    CheckControl<bool>(fault, true, true);
  }
  else
  {
    CheckControl<uint8>(fault, true, true);
  }
}

TEST_CASE("IdentifySample observation ignores null and unrelated identities", "[.IdentifySampleObservation]")
{
  UnitTest::LoadPlugins();
  const bool installObserver = GENERATE(false, true);
  CAPTURE(installObserver);
  CheckControl<uint8>(CopyFault::None, false, installObserver);
}

TEST_CASE("IdentifySample observation restores nested observers and isolates threads", "[.IdentifySampleObservation]")
{
  LogicalCounters outer;
  LogicalCounters inner;
  const auto initial = SetIdentifySampleSliceObserverForTesting({&outer, &LogicalCounters::Observe});
  const auto restoreInitial = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(initial); });
  REQUIRE(initial.callback == nullptr);
  {
    const auto prior = SetIdentifySampleSliceObserverForTesting({&inner, &LogicalCounters::Observe});
    const auto restorePrior = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
    REQUIRE(prior.context == &outer);
    REQUIRE(prior.callback == &LogicalCounters::Observe);
  }
  std::atomic_bool childWasNull = false;
  std::thread worker([&] {
    const auto prior = SetIdentifySampleSliceObserverForTesting({});
    childWasNull = prior.context == nullptr && prior.callback == nullptr;
    (void)SetIdentifySampleSliceObserverForTesting(prior);
  });
  worker.join();
  REQUIRE(childWasNull.load());
  const auto current = SetIdentifySampleSliceObserverForTesting({&outer, &LogicalCounters::Observe});
  REQUIRE(current.context == &outer);
  REQUIRE(current.callback == &LogicalCounters::Observe);
}

TEST_CASE("IdentifySample benchmark decimal fields reject malformed values", "[.IdentifySampleObservation]")
{
  const auto invalid = GENERATE(std::string{}, std::string{"0"}, std::string{"-1"}, std::string{"+1"}, std::string{" 1"}, std::string{"1 "}, std::string{"1x"}, std::string{"1.0"},
                                std::string{"18446744073709551616"}, std::string{"111111111111111111111"});
  CAPTURE(invalid);
  REQUIRE_THROWS_AS(ParsePositiveDecimal(invalid, "fixture"), std::invalid_argument);
  REQUIRE(ParsePositiveDecimal("1", "fixture") == 1);
  REQUIRE(ParsePositiveDecimal("4194304", "fixture") == 4194304);
  REQUIRE(ParsePositiveDecimal("536870912", "fixture") == 536870912);
  REQUIRE(ParsePositiveDecimal("18446744073709551615", "fixture") == std::numeric_limits<uint64>::max());
}

TEST_CASE("IdentifySample positive headroom batches YZ transfers", "[.IdentifySampleObservation][.IdentifySampleBatchScheduleRed]")
{
  UnitTest::LoadPlugins();
  const bool useBool = GENERATE(false, true);
  const bool fillHoles = GENERATE(false, true);
  DYNAMIC_SECTION("Bool=" << useBool << " fill holes=" << fillHoles)
  {
    const auto check = [&]<class T>() {
      const IdentifySampleBatchTest::ScopedPreferenceRestore restorePreferences;
      const UnitTest::PreferencesSentinel resident(DataStorageMode::ForceInCore, 1);
      DataStructure data = IdentifySampleBatchTest::CreateFixture<T>(2);
      REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
      auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
      REQUIRE(mask.getDataStoreRef().getStoreType() == IDataStore::StoreType::InMemory);
      auto& budget = CacheMemoryBudgetManager::instance();
      REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
      const BudgetRestore restoreBudget;
      REQUIRE_FALSE(budget.setBudgetBytes(4ULL * 1024 * 1024));
      const uint64 capacity = budget.maximumWorkingMemoryBytes();
      constexpr uint64 k_ExtraBytes = 7 * 11 * 13 * sizeof(T);
      auto occupied = budget.reserveWorkingMemory(capacity - k_ExtraBytes);
      REQUIRE(occupied.sizeBytes() == capacity - k_ExtraBytes);
      LogicalCounters counters;
      counters.selected = &mask;
      IdentifySampleInputValues inputs{};
      inputs.FillHoles = fillHoles;
      inputs.InputImageGeometryPath = IdentifySampleBatchTest::k_ImagePath;
      inputs.MaskArrayPath = IdentifySampleBatchTest::k_MaskPath;
      inputs.SliceBySlice = true;
      inputs.SliceBySlicePlaneIndex = 2;
      const std::atomic_bool cancel = false;
      const IFilter::MessageHandler messages;
      IdentifySampleCCL algorithm(data, messages, cancel, &inputs);
      {
        const auto prior = SetIdentifySampleSliceObserverForTesting({&counters, &LogicalCounters::Observe});
        const auto restore = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
        REQUIRE(prior.callback == nullptr);
        const auto result = algorithm();
        SIMPLNX_RESULT_REQUIRE_VALID(result);
      }
      REQUIRE(budget.reservedWorkingMemoryBytes() == occupied.sizeBytes());
      REQUIRE_FALSE(counters.invalid);
      REQUIRE(counters.active == 0);
      REQUIRE(counters.planeLive == 0);
      REQUIRE(counters.xyLive == 0);
      REQUIRE(counters.concurrent == 1);
      REQUIRE(counters.maximumSpan == 19 * 11);
      IdentifySampleBatchTest::RequireOutput<T>(data, 2, fillHoles);
      // Nineteen columns require three batches of at most eight columns.
      CHECK(counters.reads == 2 * 3 * 13);
      CHECK(counters.writes == 3 * 13);
      CHECK(counters.readSuccess == counters.reads);
      CHECK(counters.writeSuccess == counters.writes);
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
}
