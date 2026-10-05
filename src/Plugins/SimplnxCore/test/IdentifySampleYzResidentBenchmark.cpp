#include "IdentifySampleBenchmarkUtilities.hpp"

#include "simplnx/Common/ScopeGuard.hpp"

#include <cstdio>
#include <type_traits>

using namespace nx::core;

namespace
{
using namespace IdentifySampleBenchmarkTest;

/**
 * @brief Measures one supported resident algorithm scenario on the same large predicate as the actual-OOC fixture.
 * @tparam T Bool or UInt8 mask element type.
 * @param fillHoles Selects hole filling.
 * @param requestedScenario Selects BFS or CCL while the store remains resident.
 */
template <class T>
void MeasureResident(bool fillHoles, UnitTest::AlgorithmTestScenario requestedScenario)
{
  UnitTest::LoadPlugins();
  const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
  REQUIRE(std::find(scenarios.begin(), scenarios.end(), requestedScenario) != scenarios.end());
  const IdentifySampleBatchTest::ScopedPreferenceRestore preferenceRestore;
  UnitTest::AlgorithmTestScope scope(requestedScenario);
  const uint64 requestedBudget = ReadPositiveEnvironment("SIMPLNX_IDENTIFY_BENCHMARK_CACHE_BYTES", k_DefaultBudget);
  const uint64 repetition = ReadPositiveEnvironment("SIMPLNX_IDENTIFY_BENCHMARK_REPETITION", 1);
  auto& budget = CacheMemoryBudgetManager::instance();
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  const uint64 originalBudget = budget.budgetBytes();
  {
    const BudgetRestore restoreBudget;
    const bool clamped = budget.setBudgetBytes(requestedBudget);
#ifdef SIMPLNX_ENABLE_MULTICORE
    const tbb::global_control threadLimit(tbb::global_control::max_allowed_parallelism, k_RequestedThreads);
    const usize effectiveThreads = tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
#else
    const usize effectiveThreads = 1;
#endif
    DataStructure data = CreateBenchmarkFixture<T>();
    REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
    auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
    scope.requireExpectedStore(mask);
    REQUIRE(dynamic_cast<const DataStore<T>*>(mask.getDataStore()) != nullptr);
    IdentifySampleFilter filter;
    const auto args = IdentifySampleBatchTest::ArgumentsFor(2, fillHoles);
    const auto preflight = filter.preflight(data, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const bool useCcl = requestedScenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore;
    Json metadata = {{"version", 1},
                     {"phase", "after"},
                     {"algorithm", useCcl ? "CCL" : "BFS"},
                     {"store", "resident"},
                     {"type", std::is_same_v<T, bool> ? "bool" : "uint8"},
                     {"fill_holes", fillHoles},
                     {"repetition", repetition},
                     {"pid", ProcessId()},
                     {"requested_cache_bytes", requestedBudget},
                     {"effective_cache_bytes", budget.budgetBytes()},
                     {"cache_budget_clamped", clamped},
                     {"working_capacity_bytes", budget.maximumWorkingMemoryBytes()},
                     {"requested_tbb_thread_cap", k_RequestedThreads},
                     {"effective_tbb_thread_cap", effectiveThreads}};
    fmt::print("P2_IDENTIFY_RESIDENT_SETUP {}\n", metadata.dump());
    REQUIRE(std::fflush(stdout) == 0);
    LogicalCounters logical;
    logical.selected = &mask;
    Clock::time_point start;
    Clock::time_point end;
    int64 utcStart = 0;
    int64 utcEnd = 0;
    ProcessIo ioBefore;
    ProcessIo ioAfter;
    {
      const auto previous = SetIdentifySampleSliceObserverForTesting({&logical, &LogicalCounters::Observe});
      const auto restore = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(previous); });
      REQUIRE(previous.callback == nullptr);
      const auto result = scope.execute([&] {
        ioBefore = ReadProcessIo();
        utcStart = UtcMilliseconds();
        start = Clock::now();
        auto executed = filter.execute(data, args);
        end = Clock::now();
        utcEnd = UtcMilliseconds();
        ioAfter = ReadProcessIo();
        return executed;
      });
      SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    }
    REQUIRE_FALSE(logical.invalid);
    REQUIRE(logical.active == 0);
    REQUIRE(logical.planeLive == 0);
    REQUIRE(logical.xyLive == 0);
    if(useCcl)
    {
      REQUIRE(logical.reads == 10000);
      REQUIRE(logical.writes == 5000);
      REQUIRE(logical.readSuccess == logical.reads);
      REQUIRE(logical.writeSuccess == logical.writes);
      REQUIRE(logical.maximumSpan == k_XyValues);
      REQUIRE(logical.concurrent == 1);
      REQUIRE(logical.allocationEvents == 2);
      REQUIRE(logical.releaseEvents == 2);
      REQUIRE(logical.namedCarrierPeak == 2 * k_XyValues * sizeof(T));
      RequireFullWidthYzBatchObservations<T>(logical);
      metadata["logical_ccl"] = logical.describe();
    }
    else
    {
      REQUIRE(logical.reads == 0);
      REQUIRE(logical.writes == 0);
      REQUIRE(logical.allocationEvents == 0);
      metadata["logical_ccl"] = "not applicable; selected BFS does not use the CCL observer";
    }
    REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
    scope.requireExpectedStore(mask);
    RequireResidentOutput<T>(data, fillHoles);
    metadata["execute_ms"] = std::chrono::duration<double, std::milli>(end - start).count();
    metadata["utc_epoch_ms"] = {{"execute_start", utcStart}, {"execute_end", utcEnd}};
    metadata["process_io"] = ProcessIoDelta(ioBefore, ioAfter);
    metadata["persistence"] = "resident mask; no separate mask-file flush";
    metadata["correctness"] = "all 8000000 values match the independent predicate";
    fmt::print("P2_IDENTIFY_RESIDENT_MEASUREMENT {}\n", metadata.dump());
  }
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  REQUIRE(budget.budgetBytes() == originalBudget);
}
} // namespace

TEST_CASE("IdentifySample YZ resident BFS bool noholes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<bool>(false, UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
}
TEST_CASE("IdentifySample YZ resident BFS bool holes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<bool>(true, UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
}
TEST_CASE("IdentifySample YZ resident BFS uint8 noholes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<uint8>(false, UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
}
TEST_CASE("IdentifySample YZ resident BFS uint8 holes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<uint8>(true, UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
}
TEST_CASE("IdentifySample YZ resident CCL bool noholes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<bool>(false, UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
}
TEST_CASE("IdentifySample YZ resident CCL bool holes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<bool>(true, UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
}
TEST_CASE("IdentifySample YZ resident CCL uint8 noholes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<uint8>(false, UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
}
TEST_CASE("IdentifySample YZ resident CCL uint8 holes", "[.IdentifySampleYzResidentBenchmark]")
{
  MeasureResident<uint8>(true, UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
}
