#include "AlgorithmDispatchTestSupport.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @class DirectDispatchProbe
 * @brief Records direct-probe selection.
 */
class DirectDispatchProbe
{
public:
  explicit DirectDispatchProbe(bool& usedDirect)
  : m_UsedDirect(usedDirect)
  {
  }

  Result<> operator()()
  {
    m_UsedDirect = true;
    return {};
  }

private:
  bool& m_UsedDirect;
};

/**
 * @class ScanlineDispatchProbe
 * @brief Records scanline-probe selection.
 */
class ScanlineDispatchProbe
{
public:
  explicit ScanlineDispatchProbe(bool& usedDirect)
  : m_UsedDirect(usedDirect)
  {
  }

  Result<> operator()()
  {
    m_UsedDirect = false;
    return {};
  }

private:
  bool& m_UsedDirect;
};

/**
 * @class TestOutOfCoreListStore
 * @brief Reports out-of-core residency for dispatch classification tests.
 *
 * The store retains ListStore storage. The override isolates mixed IArray
 * residency classification without a disk-backed test store.
 */
class TestOutOfCoreListStore : public ListStore<int32>
{
public:
  using ListStore<int32>::ListStore;

  bool isOutOfCore() const noexcept override
  {
    return true;
  }
};

/**
 * @brief Dispatches data-array targets to test probes.
 * @param arrays Identifies targets for residency selection.
 * @param usedDirect Receives true when the direct probe runs.
 * @return Result from the selected probe.
 */
Result<> DispatchDataArraysForTest(std::initializer_list<const IDataArray*> arrays, bool& usedDirect)
{
  return DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>(arrays, usedDirect);
}

/**
 * @brief Dispatches an empty target list to test probes.
 * @param usedDirect Receives true when the direct probe runs.
 * @return Result from the selected probe.
 *
 * An empty list verifies the default direct path without a storage target.
 */
Result<> DispatchEmptyForTest(bool& usedDirect)
{
  return DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({}, usedDirect);
}

/**
 * @class AlgorithmScopeStateGuard
 * @brief Restores test state if an assertion interrupts the scope restoration test.
 *
 * This guard does not use the scope or preference sentinel under test.
 */
class AlgorithmScopeStateGuard
{
public:
  /**
   * @brief Saves the preferences and dispatch state before the test changes them.
   * @param preferences Supplies the process preferences that outlive this guard.
   */
  explicit AlgorithmScopeStateGuard(Preferences& preferences)
  : m_Preferences(preferences)
  , m_HadMode(preferences.contains(Preferences::k_DataStorageMode_Key))
  , m_Mode(preferences.value(Preferences::k_DataStorageMode_Key))
  , m_HadThreshold(preferences.contains(Preferences::k_LargeDataSize_Key))
  , m_Threshold(preferences.value(Preferences::k_LargeDataSize_Key))
  , m_ForceInCore(ForceInCoreAlgorithm())
  , m_ForceOutOfCore(ForceOocAlgorithm())
  , m_Counts(GetAlgorithmPathExecutionCounts())
  {
  }

  /**
   * @brief Restores saved state without writing a preferences file.
   */
  ~AlgorithmScopeStateGuard()
  {
    ForceInCoreAlgorithm() = m_ForceInCore;
    ForceOocAlgorithm() = m_ForceOutOfCore;
    SetAlgorithmPathExecutionCounts(m_Counts);
    if(m_HadMode)
    {
      m_Preferences.setValue(Preferences::k_DataStorageMode_Key, m_Mode);
    }
    else
    {
      m_Preferences.removeValue(Preferences::k_DataStorageMode_Key);
    }
    if(m_HadThreshold)
    {
      m_Preferences.setValue(Preferences::k_LargeDataSize_Key, m_Threshold);
    }
    else
    {
      m_Preferences.removeValue(Preferences::k_LargeDataSize_Key);
    }
  }

  AlgorithmScopeStateGuard(const AlgorithmScopeStateGuard&) = delete;
  AlgorithmScopeStateGuard(AlgorithmScopeStateGuard&&) = delete;
  AlgorithmScopeStateGuard& operator=(const AlgorithmScopeStateGuard&) = delete;
  AlgorithmScopeStateGuard& operator=(AlgorithmScopeStateGuard&&) = delete;

private:
  Preferences& m_Preferences;
  bool m_HadMode;
  nlohmann::json m_Mode;
  bool m_HadThreshold;
  nlohmann::json m_Threshold;
  bool m_ForceInCore;
  bool m_ForceOutOfCore;
  AlgorithmPathExecutionCounts m_Counts;
};

void CheckDispatchCounts(const AlgorithmPathExecutionCounts& expected)
{
  const auto actual = GetAlgorithmPathExecutionCounts();
  CHECK(actual.InCore == expected.InCore);
  CHECK(actual.OutOfCore == expected.OutOfCore);
  CHECK(actual.InCoreOnInMemoryStore == expected.InCoreOnInMemoryStore);
  CHECK(actual.InCoreOnOutOfCoreStore == expected.InCoreOnOutOfCoreStore);
  CHECK(actual.OutOfCoreOnInMemoryStore == expected.OutOfCoreOnInMemoryStore);
  CHECK(actual.OutOfCoreOnOutOfCoreStore == expected.OutOfCoreOnOutOfCoreStore);
}
} // namespace

TEST_CASE("OrientationAnalysis::AlgorithmDispatch recognizes list-store residency", "[OrientationAnalysis][AlgorithmDispatch]")
{
  DataStructure dataStructure;
  auto* dataArrayPtr = DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Data", {1}, {1});
  auto* inMemoryListPtr = NeighborList<int32>::Create(dataStructure, "InMemoryList", {1});
  auto outOfCoreStore = std::make_shared<TestOutOfCoreListStore>(ShapeType{1});
  auto* outOfCoreListPtr = NeighborList<int32>::Create(dataStructure, "OutOfCoreList", outOfCoreStore);
  auto* stringArrayPtr = StringArray::CreateWithValues(dataStructure, "Strings", {1}, {"value"});

  REQUIRE(dataArrayPtr != nullptr);
  REQUIRE(inMemoryListPtr != nullptr);
  REQUIRE(outOfCoreListPtr != nullptr);
  REQUIRE(stringArrayPtr != nullptr);

  UnitTest::AlgorithmTestScope inMemoryScope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
  inMemoryScope.requireExpectedStore(*inMemoryListPtr);

  bool usedDirect = false;
  const std::initializer_list<const IDataArray*> namedDataArrays = {dataArrayPtr};
  CHECK_FALSE(AnyOutOfCore(namedDataArrays));
  REQUIRE(DispatchDataArraysForTest(namedDataArrays, usedDirect).valid());
  CHECK(usedDirect);

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({inMemoryListPtr}, usedDirect).valid());
  CHECK(usedDirect);

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({outOfCoreListPtr}, usedDirect).valid());
  CHECK_FALSE(usedDirect);

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({dataArrayPtr, outOfCoreListPtr}, usedDirect).valid());
  CHECK_FALSE(usedDirect);
  CHECK(AnyOutOfCore({dataArrayPtr, outOfCoreListPtr}));

  const AlgorithmArrayTargets namedTargets = {dataArrayPtr, outOfCoreListPtr};
  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>(namedTargets, usedDirect).valid());
  CHECK_FALSE(usedDirect);
  CHECK(AnyOutOfCore(namedTargets));

  std::vector<const IArray*> dynamicTargetPointers = {dataArrayPtr, outOfCoreListPtr};
  const AlgorithmArrayTargets dynamicTargets(std::move(dynamicTargetPointers));
  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>(dynamicTargets, usedDirect).valid());
  CHECK_FALSE(usedDirect);
  CHECK(AnyOutOfCore(dynamicTargets));

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({nullptr, dataArrayPtr, stringArrayPtr}, usedDirect).valid());
  CHECK(usedDirect);

  REQUIRE(DispatchEmptyForTest(usedDirect).valid());
  CHECK(usedDirect);
  CHECK_FALSE(AnyOutOfCore({}));
}

TEST_CASE("OrientationAnalysis::AlgorithmDispatch force state crosses the plugin boundary", "[OrientationAnalysis][AlgorithmDispatch]")
{
  UnitTest::LoadPlugins();
  using PathType = UnitTest::AlgorithmDispatchPath;
  struct SelectionCase
  {
    bool force_in_core = false;
    bool force_ooc = false;
    PathType expected_path = PathType::Direct;
    AlgorithmPathExecutionCounts expected_counts{};
  };
  const std::array<SelectionCase, 4> cases = {{
      {.force_in_core = false,
       .force_ooc = false,
       .expected_path = PathType::Direct,
       .expected_counts = {.InCore = 1, .OutOfCore = 0, .InCoreOnInMemoryStore = 1, .InCoreOnOutOfCoreStore = 0, .OutOfCoreOnInMemoryStore = 0, .OutOfCoreOnOutOfCoreStore = 0}},
      {.force_in_core = false,
       .force_ooc = true,
       .expected_path = PathType::Scanline,
       .expected_counts = {.InCore = 0, .OutOfCore = 1, .InCoreOnInMemoryStore = 0, .InCoreOnOutOfCoreStore = 0, .OutOfCoreOnInMemoryStore = 1, .OutOfCoreOnOutOfCoreStore = 0}},
      {.force_in_core = true,
       .force_ooc = false,
       .expected_path = PathType::Direct,
       .expected_counts = {.InCore = 1, .OutOfCore = 0, .InCoreOnInMemoryStore = 1, .InCoreOnOutOfCoreStore = 0, .OutOfCoreOnInMemoryStore = 0, .OutOfCoreOnOutOfCoreStore = 0}},
      {.force_in_core = true,
       .force_ooc = true,
       .expected_path = PathType::Direct,
       .expected_counts = {.InCore = 1, .OutOfCore = 0, .InCoreOnInMemoryStore = 1, .InCoreOnOutOfCoreStore = 0, .OutOfCoreOnInMemoryStore = 0, .OutOfCoreOnOutOfCoreStore = 0}},
  }};
  const bool originalInCore = ForceInCoreAlgorithm();
  const bool originalOoc = ForceOocAlgorithm();
  const auto originalCounts = GetAlgorithmPathExecutionCounts();
  const auto restoreState = MakeScopeGuard([=]() noexcept {
    ForceInCoreAlgorithm() = originalInCore;
    ForceOocAlgorithm() = originalOoc;
    SetAlgorithmPathExecutionCounts(originalCounts);
  });

  for(const auto& selection : cases)
  {
    CAPTURE(selection.force_in_core, selection.force_ooc);
    ForceInCoreAlgorithm() = selection.force_in_core;
    ForceOocAlgorithm() = selection.force_ooc;
    ResetAlgorithmPathExecutionCounts();
    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == selection.expected_path);
    CheckDispatchCounts(selection.expected_counts);
  }
}

TEST_CASE("OrientationAnalysis::AlgorithmDispatch records the selected plugin path", "[OrientationAnalysis][AlgorithmDispatch]")
{
  using PathType = UnitTest::AlgorithmDispatchPath;

  ResetAlgorithmPathExecutionCounts();
  {
    const ForceInCoreAlgorithmGuard forceDirect;
    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == PathType::Direct);
  }
  const auto directCounts = GetAlgorithmPathExecutionCounts();
  CHECK(directCounts.InCore == 1);
  CHECK(directCounts.OutOfCore == 0);
  CHECK(directCounts.InCoreOnInMemoryStore == 1);
  CHECK(directCounts.InCoreOnOutOfCoreStore == 0);
  CHECK(directCounts.OutOfCoreOnInMemoryStore == 0);
  CHECK(directCounts.OutOfCoreOnOutOfCoreStore == 0);

  ResetAlgorithmPathExecutionCounts();
  {
    const ForceOocAlgorithmGuard forceScanline(true);
    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == PathType::Scanline);
  }
  const auto scanlineCounts = GetAlgorithmPathExecutionCounts();
  CHECK(scanlineCounts.InCore == 0);
  CHECK(scanlineCounts.OutOfCore == 1);
  CHECK(scanlineCounts.InCoreOnInMemoryStore == 0);
  CHECK(scanlineCounts.InCoreOnOutOfCoreStore == 0);
  CHECK(scanlineCounts.OutOfCoreOnInMemoryStore == 1);
  CHECK(scanlineCounts.OutOfCoreOnOutOfCoreStore == 0);

  ResetAlgorithmPathExecutionCounts();
}

TEST_CASE("OrientationAnalysis::AlgorithmTestScenario selects configured algorithm paths", "[OrientationAnalysis][AlgorithmDispatch][AlgorithmTestScenario]")
{
  using ScenarioType = UnitTest::AlgorithmTestScenario;

  const std::vector allScenarios = {
      ScenarioType::InCoreAlgorithmOnInMemoryStore,
      ScenarioType::OutOfCoreAlgorithmOnInMemoryStore,
  };
  const std::vector oocScenarios = {ScenarioType::OutOfCoreAlgorithmOnInMemoryStore};
  const std::vector inCoreScenarios = {ScenarioType::InCoreAlgorithmOnInMemoryStore};

  CHECK(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 0) == allScenarios);
  CHECK(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 1) == oocScenarios);
  CHECK(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 2) == inCoreScenarios);
  CHECK_THROWS_WITH(UnitTest::SelectAlgorithmTestScenarios(inCoreScenarios, 1), "SIMPLNX_TEST_ALGORITHM_PATH=1 excludes every explicitly requested algorithm test scenario");
  CHECK_THROWS_WITH(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 3), "Unsupported SIMPLNX_TEST_ALGORITHM_PATH value 3; expected 0 (Both), 1 (OocOnly), or 2 (InCoreOnly)");
}

TEST_CASE("OrientationAnalysis::AlgorithmTestScenario helpers encode the supported test matrices", "[OrientationAnalysis][AlgorithmDispatch][AlgorithmTestScenario]")
{
  using ScenarioType = UnitTest::AlgorithmTestScenario;

  const std::vector bothInMemoryScenarios = {
      ScenarioType::InCoreAlgorithmOnInMemoryStore,
      ScenarioType::OutOfCoreAlgorithmOnInMemoryStore,
  };
  const std::vector inCoreInMemoryScenario = {ScenarioType::InCoreAlgorithmOnInMemoryStore};
  const std::vector outOfCoreInMemoryScenario = {ScenarioType::OutOfCoreAlgorithmOnInMemoryStore};

  CHECK(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores(0) == bothInMemoryScenarios);
  CHECK(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores(1) == outOfCoreInMemoryScenario);
  CHECK(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores(2) == inCoreInMemoryScenario);
}

TEST_CASE("OrientationAnalysis::AlgorithmTestScope witnesses only the target execution and restores state", "[OrientationAnalysis][AlgorithmDispatch][AlgorithmTestScenario]")
{
  using ScenarioType = UnitTest::AlgorithmTestScenario;
  using PathType = UnitTest::AlgorithmDispatchPath;

  UnitTest::LoadPlugins();
  ResetAlgorithmPathExecutionCounts();
  RecordAlgorithmPathExecution(AlgorithmPath::InCore, false);

  {
    const ForceOocAlgorithmGuard originalForceState(true);
    UnitTest::AlgorithmTestScope scope(ScenarioType::InCoreAlgorithmOnInMemoryStore);
    const auto selectedPath = scope.execute([] { return UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin(); });
    CHECK(selectedPath == PathType::Direct);
    CHECK(ForceOocAlgorithm());
    CHECK_FALSE(ForceInCoreAlgorithm());
  }

  const auto restoredCounts = GetAlgorithmPathExecutionCounts();
  CHECK(restoredCounts.InCore == 1);
  CHECK(restoredCounts.OutOfCore == 0);
  CHECK(restoredCounts.InCoreOnInMemoryStore == 1);
  CHECK(restoredCounts.InCoreOnOutOfCoreStore == 0);
  CHECK(restoredCounts.OutOfCoreOnInMemoryStore == 0);
  CHECK(restoredCounts.OutOfCoreOnOutOfCoreStore == 0);
  ResetAlgorithmPathExecutionCounts();
}

TEST_CASE("OrientationAnalysis::AlgorithmTestScope runs OOC algorithm on in-memory storage", "[OrientationAnalysis][AlgorithmDispatch][AlgorithmTestScenario]")
{
  using ScenarioType = UnitTest::AlgorithmTestScenario;
  using PathType = UnitTest::AlgorithmDispatchPath;

  UnitTest::LoadPlugins();
  UnitTest::AlgorithmTestScope scope(ScenarioType::OutOfCoreAlgorithmOnInMemoryStore);
  DataStructure dataStructure;
  auto* valuesPtr = DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {1}, {1});
  REQUIRE(valuesPtr != nullptr);
  scope.requireExpectedStore(*valuesPtr);
  CHECK(scope.execute([] { return UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin(); }) == PathType::Scanline);
}

TEST_CASE("OrientationAnalysis::AlgorithmTestScope restores state when the target throws", "[OrientationAnalysis][AlgorithmDispatch][AlgorithmTestScenario]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool hadMode = GENERATE(false, true);
  const bool hadThreshold = GENERATE(false, true);
  const bool originalForceInCore = GENERATE(false, true);
  const bool originalForceOutOfCore = GENERATE(false, true);
  CAPTURE(scenario, hadMode, hadThreshold, originalForceInCore, originalForceOutOfCore);

  UnitTest::LoadPlugins();
  auto* preferencesPtr = Application::GetOrCreateInstance()->getPreferences();
  const AlgorithmScopeStateGuard restoreTestState(*preferencesPtr);

  if(hadMode)
  {
    preferencesPtr->setDataStorageMode(DataStorageMode::Adaptive);
  }
  else
  {
    preferencesPtr->removeValue(Preferences::k_DataStorageMode_Key);
  }
  if(hadThreshold)
  {
    preferencesPtr->setValue(Preferences::k_LargeDataSize_Key, std::numeric_limits<uint64>::max());
  }
  else
  {
    preferencesPtr->removeValue(Preferences::k_LargeDataSize_Key);
  }
  const auto originalMode = preferencesPtr->value(Preferences::k_DataStorageMode_Key);
  const auto originalThreshold = preferencesPtr->value(Preferences::k_LargeDataSize_Key);
  const AlgorithmPathExecutionCounts originalCounts{
      .InCore = 11, .OutOfCore = 13, .InCoreOnInMemoryStore = 17, .InCoreOnOutOfCoreStore = 19, .OutOfCoreOnInMemoryStore = 23, .OutOfCoreOnOutOfCoreStore = 29};
  ForceInCoreAlgorithm() = originalForceInCore;
  ForceOocAlgorithm() = originalForceOutOfCore;
  SetAlgorithmPathExecutionCounts(originalCounts);

  {
    UnitTest::AlgorithmTestScope scope(scenario);
    const bool expectsOutOfCore = scenario == UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore;
    bool caughtExpectedException = false;
    // The outer handler keeps the original exception alive for the identity check.
    try
    {
      throw std::runtime_error("public algorithm scope target failure");
    } catch(const std::runtime_error& expected)
    {
      try
      {
        scope.execute([&] {
          CHECK(ForceInCoreAlgorithm() == !expectsOutOfCore);
          CHECK(ForceOocAlgorithm() == expectsOutOfCore);
          CheckDispatchCounts({});
          REQUIRE(preferencesPtr->dataStorageMode() == DataStorageMode::ForceInCore);
          REQUIRE(preferencesPtr->value(Preferences::k_LargeDataSize_Key).type() == originalThreshold.type());
          REQUIRE(preferencesPtr->value(Preferences::k_LargeDataSize_Key) == originalThreshold);
          // The synthetic event tests scope cleanup; it does not represent a filter execution.
          RecordAlgorithmPathExecution(expectsOutOfCore ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, false);
          throw;
        });
      } catch(const std::runtime_error& actual)
      {
        caughtExpectedException = true;
        CHECK(&actual == &expected);
        CHECK(std::string(actual.what()) == "public algorithm scope target failure");
      }
    }
    REQUIRE(caughtExpectedException);
    CHECK(ForceInCoreAlgorithm() == originalForceInCore);
    CHECK(ForceOocAlgorithm() == originalForceOutOfCore);
    CheckDispatchCounts(originalCounts);
    CHECK(preferencesPtr->dataStorageMode() == DataStorageMode::ForceInCore);
  }

  CHECK(preferencesPtr->contains(Preferences::k_DataStorageMode_Key) == hadMode);
  CHECK(preferencesPtr->value(Preferences::k_DataStorageMode_Key).type() == originalMode.type());
  CHECK(preferencesPtr->value(Preferences::k_DataStorageMode_Key) == originalMode);
  CHECK(preferencesPtr->contains(Preferences::k_LargeDataSize_Key) == hadThreshold);
  CHECK(preferencesPtr->value(Preferences::k_LargeDataSize_Key).type() == originalThreshold.type());
  CHECK(preferencesPtr->value(Preferences::k_LargeDataSize_Key) == originalThreshold);
}
