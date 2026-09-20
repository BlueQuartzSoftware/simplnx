#include "AlgorithmTestScope.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "simplnx/Core/Preferences.hpp"

#include <algorithm>
#include <stdexcept>

namespace
{
bool UsesOutOfCoreAlgorithm(nx::core::UnitTest::AlgorithmTestScenario scenario)
{
  return scenario == nx::core::UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore;
}
} // namespace

namespace nx::core::UnitTest
{
std::ostream& operator<<(std::ostream& output, AlgorithmTestScenario scenario)
{
  switch(scenario)
  {
  case AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore:
    return output << "InCoreAlgorithmOnInMemoryStore";
  case AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore:
    return output << "OutOfCoreAlgorithmOnInMemoryStore";
  }
  return output << "UnknownAlgorithmTestScenario";
}

std::vector<AlgorithmTestScenario> SelectAlgorithmTestScenarios(std::vector<AlgorithmTestScenario> requestedScenarios, int algorithmPathSetting)
{
  if(algorithmPathSetting < 0 || algorithmPathSetting > 2)
  {
    throw std::invalid_argument("Unsupported SIMPLNX_TEST_ALGORITHM_PATH value " + std::to_string(algorithmPathSetting) + "; expected 0 (Both), 1 (OocOnly), or 2 (InCoreOnly)");
  }

  std::erase_if(requestedScenarios, [algorithmPathSetting](AlgorithmTestScenario scenario) {
    if(algorithmPathSetting == 0)
    {
      return false;
    }
    const bool usesOutOfCoreAlgorithm = UsesOutOfCoreAlgorithm(scenario);
    return algorithmPathSetting == 1 ? !usesOutOfCoreAlgorithm : usesOutOfCoreAlgorithm;
  });

  if(requestedScenarios.empty())
  {
    throw std::invalid_argument("SIMPLNX_TEST_ALGORITHM_PATH=" + std::to_string(algorithmPathSetting) + " excludes every explicitly requested algorithm test scenario");
  }
  return requestedScenarios;
}

std::vector<AlgorithmTestScenario> SelectAlgorithmTestScenariosForInMemoryStores(int algorithmPathSetting)
{
  return SelectAlgorithmTestScenarios(
      {
          AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore,
          AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore,
      },
      algorithmPathSetting);
}

AlgorithmTestScope::AlgorithmTestScope(AlgorithmTestScenario scenario)
: m_Scenario(scenario)
{
  auto* preferences = Application::GetOrCreateInstance()->getPreferences();
  const int64 originalLargeDataSize = preferences->valueAs<int64>(Preferences::k_LargeDataSize_Key);
  m_PreferencesSentinel = std::make_unique<PreferencesSentinel>(DataStorageMode::ForceInCore, originalLargeDataSize);
}

AlgorithmTestScope::~AlgorithmTestScope() = default;

void AlgorithmTestScope::requireExpectedStore(const IDataArray& array) const
{
  const IDataStore::StoreType actualStore = array.getIDataStoreRef().getStoreType();
  INFO("Scenario " << m_Scenario << ": array '" << array.getName() << "' expected an in-memory store; actual store=" << static_cast<int>(actualStore));
  REQUIRE(actualStore == IDataStore::StoreType::InMemory);
}

void AlgorithmTestScope::requireExpectedStore(const INeighborList& array) const
{
  const auto* listStore = array.getIListStore();
  REQUIRE(listStore != nullptr);
  const bool actualOutOfCore = listStore->isOutOfCore();
  INFO("Scenario " << m_Scenario << ": NeighborList '" << array.getName() << "' expected an in-memory list store; actual out-of-core=" << actualOutOfCore);
  REQUIRE_FALSE(actualOutOfCore);
}

AlgorithmTestScenario AlgorithmTestScope::scenario() const noexcept
{
  return m_Scenario;
}

AlgorithmTestScope::ExecutionStateGuard::ExecutionStateGuard(AlgorithmTestScenario scenario)
: m_OriginalForceOutOfCore(ForceOocAlgorithm())
, m_OriginalForceInCore(ForceInCoreAlgorithm())
, m_OriginalExecutionCounts(GetAlgorithmPathExecutionCounts())
{
  const bool useOutOfCoreAlgorithm = UsesOutOfCoreAlgorithm(scenario);
  ResetAlgorithmPathExecutionCounts();
  ForceInCoreAlgorithm() = !useOutOfCoreAlgorithm;
  ForceOocAlgorithm() = useOutOfCoreAlgorithm;
}

AlgorithmTestScope::ExecutionStateGuard::~ExecutionStateGuard()
{
  ForceOocAlgorithm() = m_OriginalForceOutOfCore;
  ForceInCoreAlgorithm() = m_OriginalForceInCore;
  SetAlgorithmPathExecutionCounts(m_OriginalExecutionCounts);
}

void AlgorithmTestScope::requireExpectedScenario() const
{
  const AlgorithmPathExecutionCounts observedCounts = GetAlgorithmPathExecutionCounts();
  const bool expectsOutOfCoreAlgorithm = UsesOutOfCoreAlgorithm(m_Scenario);
  const uint64 expectedCount = expectsOutOfCoreAlgorithm ? observedCounts.OutOfCore : observedCounts.InCore;
  const uint64 unexpectedCount = expectsOutOfCoreAlgorithm ? observedCounts.InCore : observedCounts.OutOfCore;
  const uint64 expectedScenarioCount = expectsOutOfCoreAlgorithm ? observedCounts.OutOfCoreOnInMemoryStore : observedCounts.InCoreOnInMemoryStore;
  const uint64 unexpectedStoreCount = expectsOutOfCoreAlgorithm ? observedCounts.OutOfCoreOnOutOfCoreStore : observedCounts.InCoreOnOutOfCoreStore;

  INFO("Scenario " << m_Scenario << ": observed paths [in-core=" << observedCounts.InCore << ", out-of-core=" << observedCounts.OutOfCore
                   << "] and combinations [in-core/in-memory=" << observedCounts.InCoreOnInMemoryStore << ", in-core/OOC=" << observedCounts.InCoreOnOutOfCoreStore
                   << ", OOC/in-memory=" << observedCounts.OutOfCoreOnInMemoryStore << ", OOC/OOC=" << observedCounts.OutOfCoreOnOutOfCoreStore << "]");
  REQUIRE(expectedCount > 0);
  REQUIRE(unexpectedCount == 0);
  REQUIRE(expectedScenarioCount > 0);
  REQUIRE(unexpectedStoreCount == 0);
}
} // namespace nx::core::UnitTest
