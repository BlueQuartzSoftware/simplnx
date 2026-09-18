#include "AlgorithmDispatchTestSupport.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

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
Result<> dispatchDataArraysForTest(std::initializer_list<const IDataArray*> arrays, bool& usedDirect)
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
Result<> dispatchEmptyForTest(bool& usedDirect)
{
  return DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({}, usedDirect);
}
} // namespace

TEST_CASE("OrientationAnalysis::AlgorithmDispatch recognizes list-store residency", "[OrientationAnalysis][AlgorithmDispatch]")
{
  DataStructure dataStructure;
  auto* dataArray = DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Data", {1}, {1});
  auto* inMemoryList = NeighborList<int32>::Create(dataStructure, "InMemoryList", {1});
  auto outOfCoreStore = std::make_shared<TestOutOfCoreListStore>(ShapeType{1});
  auto* outOfCoreList = NeighborList<int32>::Create(dataStructure, "OutOfCoreList", outOfCoreStore);
  auto* stringArray = StringArray::CreateWithValues(dataStructure, "Strings", {1}, {"value"});

  REQUIRE(dataArray != nullptr);
  REQUIRE(inMemoryList != nullptr);
  REQUIRE(outOfCoreList != nullptr);
  REQUIRE(stringArray != nullptr);

  UnitTest::AlgorithmTestScope inMemoryScope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
  inMemoryScope.requireExpectedStore(*inMemoryList);

  bool usedDirect = false;
  const std::initializer_list<const IDataArray*> namedDataArrays = {dataArray};
  CHECK_FALSE(AnyOutOfCore(namedDataArrays));
  REQUIRE(dispatchDataArraysForTest(namedDataArrays, usedDirect).valid());
  CHECK(usedDirect);

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({inMemoryList}, usedDirect).valid());
  CHECK(usedDirect);

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({outOfCoreList}, usedDirect).valid());
  CHECK_FALSE(usedDirect);

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({dataArray, outOfCoreList}, usedDirect).valid());
  CHECK_FALSE(usedDirect);
  CHECK(AnyOutOfCore({dataArray, outOfCoreList}));

  const AlgorithmArrayTargets namedTargets = {dataArray, outOfCoreList};
  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>(namedTargets, usedDirect).valid());
  CHECK_FALSE(usedDirect);
  CHECK(AnyOutOfCore(namedTargets));

  std::vector<const IArray*> dynamicTargetPointers = {dataArray, outOfCoreList};
  const AlgorithmArrayTargets dynamicTargets(std::move(dynamicTargetPointers));
  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>(dynamicTargets, usedDirect).valid());
  CHECK_FALSE(usedDirect);
  CHECK(AnyOutOfCore(dynamicTargets));

  REQUIRE(DispatchAlgorithm<DirectDispatchProbe, ScanlineDispatchProbe>({nullptr, dataArray, stringArray}, usedDirect).valid());
  CHECK(usedDirect);

  REQUIRE(dispatchEmptyForTest(usedDirect).valid());
  CHECK(usedDirect);
  CHECK_FALSE(AnyOutOfCore({}));
}

TEST_CASE("OrientationAnalysis::AlgorithmDispatch force state crosses the plugin boundary", "[OrientationAnalysis][AlgorithmDispatch]")
{
  using Path = UnitTest::AlgorithmDispatchPath;

  REQUIRE_FALSE(ForceInCoreAlgorithm());

  {
    const ForceOocAlgorithmGuard forceDirect(false);
    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == Path::Direct);
  }

  {
    const ForceOocAlgorithmGuard forceScanline(true);
    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == Path::Scanline);

    {
      const ForceInCoreAlgorithmGuard forceDirect;
      REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == Path::Direct);
    }

    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == Path::Scanline);
  }

  REQUIRE_FALSE(ForceOocAlgorithm());
  REQUIRE_FALSE(ForceInCoreAlgorithm());
}

TEST_CASE("OrientationAnalysis::AlgorithmDispatch records the selected plugin path", "[OrientationAnalysis][AlgorithmDispatch]")
{
  using Path = UnitTest::AlgorithmDispatchPath;

  ResetAlgorithmPathExecutionCounts();
  {
    const ForceInCoreAlgorithmGuard forceDirect;
    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == Path::Direct);
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
    REQUIRE(UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin() == Path::Scanline);
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
  using Scenario = UnitTest::AlgorithmTestScenario;

  const std::vector allScenarios = {
      Scenario::InCoreAlgorithmOnInMemoryStore,
      Scenario::OutOfCoreAlgorithmOnInMemoryStore,
  };
  const std::vector oocScenarios = {Scenario::OutOfCoreAlgorithmOnInMemoryStore};
  const std::vector inCoreScenarios = {Scenario::InCoreAlgorithmOnInMemoryStore};

  CHECK(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 0) == allScenarios);
  CHECK(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 1) == oocScenarios);
  CHECK(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 2) == inCoreScenarios);
  CHECK_THROWS_WITH(UnitTest::SelectAlgorithmTestScenarios(inCoreScenarios, 1), "SIMPLNX_TEST_ALGORITHM_PATH=1 excludes every explicitly requested algorithm test scenario");
  CHECK_THROWS_WITH(UnitTest::SelectAlgorithmTestScenarios(allScenarios, 3), "Unsupported SIMPLNX_TEST_ALGORITHM_PATH value 3; expected 0 (Both), 1 (OocOnly), or 2 (InCoreOnly)");
}

TEST_CASE("OrientationAnalysis::AlgorithmTestScenario helpers encode the supported test matrices", "[OrientationAnalysis][AlgorithmDispatch][AlgorithmTestScenario]")
{
  using Scenario = UnitTest::AlgorithmTestScenario;

  const std::vector bothInMemoryScenarios = {
      Scenario::InCoreAlgorithmOnInMemoryStore,
      Scenario::OutOfCoreAlgorithmOnInMemoryStore,
  };
  const std::vector inCoreInMemoryScenario = {Scenario::InCoreAlgorithmOnInMemoryStore};
  const std::vector outOfCoreInMemoryScenario = {Scenario::OutOfCoreAlgorithmOnInMemoryStore};

  CHECK(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores(0) == bothInMemoryScenarios);
  CHECK(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores(1) == outOfCoreInMemoryScenario);
  CHECK(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores(2) == inCoreInMemoryScenario);
}

TEST_CASE("OrientationAnalysis::AlgorithmTestScope witnesses only the target execution and restores state", "[OrientationAnalysis][AlgorithmDispatch][AlgorithmTestScenario]")
{
  using Scenario = UnitTest::AlgorithmTestScenario;
  using Path = UnitTest::AlgorithmDispatchPath;

  UnitTest::LoadPlugins();
  ResetAlgorithmPathExecutionCounts();
  RecordAlgorithmPathExecution(AlgorithmPath::InCore, false);

  {
    const ForceOocAlgorithmGuard originalForceState(true);
    UnitTest::AlgorithmTestScope scope(Scenario::InCoreAlgorithmOnInMemoryStore);
    const auto selectedPath = scope.execute([] { return UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin(); });
    CHECK(selectedPath == Path::Direct);
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
  using Scenario = UnitTest::AlgorithmTestScenario;
  using Path = UnitTest::AlgorithmDispatchPath;

  UnitTest::LoadPlugins();
  UnitTest::AlgorithmTestScope scope(Scenario::OutOfCoreAlgorithmOnInMemoryStore);
  DataStructure dataStructure;
  auto* values = DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {1}, {1});
  REQUIRE(values != nullptr);
  scope.requireExpectedStore(*values);
  CHECK(scope.execute([] { return UnitTest::GetAlgorithmDispatchPathFromOrientationAnalysisPlugin(); }) == Path::Scanline);
}
