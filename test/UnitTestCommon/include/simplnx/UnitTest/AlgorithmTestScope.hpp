#pragma once

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <functional>
#include <memory>
#include <ostream>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::UnitTest
{
class PreferencesSentinel;

/**
 * @enum AlgorithmTestScenario
 * @brief Names a supported algorithm path for a unit test whose stores remain in memory.
 */
enum class AlgorithmTestScenario : uint8
{
  InCoreAlgorithmOnInMemoryStore,   ///< Runs the in-core path with in-memory stores.
  OutOfCoreAlgorithmOnInMemoryStore ///< Runs the OOC path with in-memory stores.
};

/**
 * @brief Writes a human-readable algorithm and backing-store scenario name.
 * @param output The destination stream.
 * @param scenario The scenario to write.
 * @return The destination stream.
 */
std::ostream& operator<<(std::ostream& output, AlgorithmTestScenario scenario);

/**
 * @brief Filters explicitly requested scenarios using an algorithm-path setting.
 * @param requestedScenarios The scenarios named by the test.
 * @param algorithmPathSetting 0 for Both, 1 for OocOnly, or 2 for InCoreOnly.
 * @return The requested scenarios compatible with the setting.
 * @throws std::invalid_argument If the setting is unsupported or excludes every requested scenario.
 */
std::vector<AlgorithmTestScenario> SelectAlgorithmTestScenarios(std::vector<AlgorithmTestScenario> requestedScenarios, int algorithmPathSetting);

/**
 * @brief Filters explicitly requested scenarios using SIMPLNX_TEST_ALGORITHM_PATH.
 * @param requestedScenarios The scenarios named by the test.
 * @return The requested scenarios compatible with the current test target.
 */
inline std::vector<AlgorithmTestScenario> SelectAlgorithmTestScenarios(std::vector<AlgorithmTestScenario> requestedScenarios)
{
  return SelectAlgorithmTestScenarios(std::move(requestedScenarios), SIMPLNX_TEST_ALGORITHM_PATH);
}

/**
 * @brief Selects the standard correctness-test scenarios that keep all stores in memory.
 * @param algorithmPathSetting 0 for Both, 1 for OocOnly, or 2 for InCoreOnly.
 * @return The in-memory scenarios compatible with the requested algorithm-path setting.
 */
std::vector<AlgorithmTestScenario> SelectAlgorithmTestScenariosForInMemoryStores(int algorithmPathSetting);

/**
 * @brief Selects the standard correctness-test scenarios using SIMPLNX_TEST_ALGORITHM_PATH.
 * @return The configured subset of in-core/in-memory and out-of-core/in-memory scenarios.
 */
inline std::vector<AlgorithmTestScenario> SelectAlgorithmTestScenariosForInMemoryStores()
{
  return SelectAlgorithmTestScenariosForInMemoryStores(SIMPLNX_TEST_ALGORITHM_PATH);
}

/**
 * @class AlgorithmTestScope
 * @brief Configures a readable unit-test scenario and proves the target algorithm/store combination.
 *
 * The scope controls backing-store preferences for its lifetime. executeFilter()
 * and execute() apply force flags and collect runtime evidence only during the
 * target call. This limit prevents setup filters from satisfying the runtime
 * check.
 */
class AlgorithmTestScope
{
public:
  /**
   * @brief Forces in-memory backing stores for the selected algorithm path.
   * @param scenario The algorithm path to test.
   */
  explicit AlgorithmTestScope(AlgorithmTestScenario scenario);

  /**
   * @brief Restores the prior backing-store preferences.
   */
  ~AlgorithmTestScope();

  AlgorithmTestScope(const AlgorithmTestScope&) = delete;
  AlgorithmTestScope(AlgorithmTestScope&&) = delete;
  AlgorithmTestScope& operator=(const AlgorithmTestScope&) = delete;
  AlgorithmTestScope& operator=(AlgorithmTestScope&&) = delete;

  /**
   * @brief Executes a target filter and verifies that only the requested algorithm path ran.
   * @tparam FilterT Specifies the filter type.
   * @tparam ArgsT Specifies the forwarded argument types.
   * @param filter The target filter.
   * @param args Arguments forwarded to filter.execute().
   * @return The value returned by filter.execute().
   */
  template <typename FilterT, typename... ArgsT>
  decltype(auto) executeFilter(FilterT&& filter, ArgsT&&... args)
  {
    return execute([&filter, &args...]() -> decltype(auto) { return std::forward<FilterT>(filter).execute(std::forward<ArgsT>(args)...); });
  }

  /**
   * @brief Executes a target callable and verifies that only the requested algorithm path ran.
   * @tparam CallableT Specifies the callable type.
   * @param callable A callable containing the target filter execution.
   * @return The value returned by the callable, or void when the callable returns void.
   */
  template <typename CallableT>
  decltype(auto) execute(CallableT&& callable)
  {
    ExecutionStateGuard executionState(m_Scenario);
    using ResultT = std::invoke_result_t<CallableT&&>;
    if constexpr(std::is_void_v<ResultT>)
    {
      std::invoke(std::forward<CallableT>(callable));
      requireExpectedScenario();
    }
    else
    {
      ResultT result = std::invoke(std::forward<CallableT>(callable));
      requireExpectedScenario();
      return result;
    }
  }

  /**
   * @brief Verifies that an array uses the backing store named by this scenario.
   * @param array The array whose concrete store is checked.
   */
  void requireExpectedStore(const IDataArray& array) const;

  /**
   * @brief Verifies that a NeighborList uses the backing store named by this scenario.
   * @param array The NeighborList whose concrete list store is checked.
   */
  void requireExpectedStore(const INeighborList& array) const;

  AlgorithmTestScenario scenario() const noexcept;

private:
  /**
   * @class ExecutionStateGuard
   * @brief Limits algorithm force flags and execution counters to one target call.
   *
   * The guard restores prior global test state so setup and subsequent filters do
   * not inherit the selected algorithm path or its runtime evidence.
   */
  class ExecutionStateGuard
  {
  public:
    /**
     * @brief Selects the requested algorithm path and clears its execution counters.
     * @param scenario Algorithm path to select.
     */
    explicit ExecutionStateGuard(AlgorithmTestScenario scenario);

    /**
     * @brief Restores the algorithm force flags and counters that the constructor saved.
     */
    ~ExecutionStateGuard();

    ExecutionStateGuard(const ExecutionStateGuard&) = delete;
    ExecutionStateGuard(ExecutionStateGuard&&) = delete;
    ExecutionStateGuard& operator=(const ExecutionStateGuard&) = delete;
    ExecutionStateGuard& operator=(ExecutionStateGuard&&) = delete;

  private:
    bool m_OriginalForceOutOfCore = false;
    bool m_OriginalForceInCore = false;
    AlgorithmPathExecutionCounts m_OriginalExecutionCounts;
  };

  void requireExpectedScenario() const;

  AlgorithmTestScenario m_Scenario;
  std::unique_ptr<PreferencesSentinel> m_PreferencesSentinel;
};
} // namespace nx::core::UnitTest
