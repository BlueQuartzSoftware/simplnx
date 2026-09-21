#include <catch2/catch.hpp>

#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

using namespace nx::core;

namespace
{
constexpr uint64 k_MiB = uint64{1024} * 1024;

class CacheBudgetSentinel
{
public:
  explicit CacheBudgetSentinel(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budgetBytes);
  }

  ~CacheBudgetSentinel()
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  CacheBudgetSentinel(const CacheBudgetSentinel&) = delete;
  CacheBudgetSentinel& operator=(const CacheBudgetSentinel&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget = 0;
};
} // namespace

TEST_CASE("ImageProcessing::WorkingMemory tuning override is scoped and nestable", "[ImageProcessing][WorkingMemory]")
{
  using namespace ImageProcessing;

  REQUIRE(ResolvePreferredWorkingMemoryBytes(64 * k_MiB) == 64 * k_MiB);
  {
    ScopedWorkingMemoryTuningOverride outerOverride(256 * k_MiB);
    REQUIRE(ResolvePreferredWorkingMemoryBytes(64 * k_MiB) == 256 * k_MiB);
    {
      ScopedWorkingMemoryTuningOverride innerOverride(128 * k_MiB);
      REQUIRE(ResolvePreferredWorkingMemoryBytes(64 * k_MiB) == 128 * k_MiB);
    }
    REQUIRE(ResolvePreferredWorkingMemoryBytes(64 * k_MiB) == 256 * k_MiB);
  }
  REQUIRE(ResolvePreferredWorkingMemoryBytes(64 * k_MiB) == 64 * k_MiB);
}

TEST_CASE("ImageProcessing::WorkingMemory request clamps to useful bytes and the shared quarter-budget ceiling", "[ImageProcessing][WorkingMemory]")
{
  using namespace ImageProcessing;

  auto& manager = CacheMemoryBudgetManager::instance();
  const CacheBudgetSentinel budgetSentinel(1000);

  {
    auto usefulClamp = ReserveWorkingMemory(/*preferredBytes=*/400, /*usefulBytes=*/100);
    REQUIRE(usefulClamp.sizeBytes() == 100);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 100);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  {
    ScopedWorkingMemoryTuningOverride override(/*sizeBytes=*/900);
    auto quarterClamp = ReserveWorkingMemory(/*preferredBytes=*/64, /*usefulBytes=*/900);
    REQUIRE(quarterClamp.sizeBytes() == 250);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 250);
    REQUIRE(manager.effectiveCacheBudgetBytes() == 750);
  }

  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
}

TEST_CASE("ImageProcessing::WorkingMemory useful-state fractions scale with the dataset-specific useful size", "[ImageProcessing][WorkingMemory]")
{
  using namespace ImageProcessing;

  REQUIRE(ResolveWorkingMemoryFractionBytes(/*usefulBytes=*/1000, /*numerator=*/1, /*denominator=*/2, /*minimumBytes=*/64) == 500);
  REQUIRE(ResolveWorkingMemoryFractionBytes(/*usefulBytes=*/100, /*numerator=*/1, /*denominator=*/4, /*minimumBytes=*/64) == 64);
  REQUIRE(ResolveWorkingMemoryFractionBytes(/*usefulBytes=*/32, /*numerator=*/1, /*denominator=*/4, /*minimumBytes=*/64) == 32);
  REQUIRE(ResolveWorkingMemoryFractionBytes(/*usefulBytes=*/1000, /*numerator=*/5, /*denominator=*/4, /*minimumBytes=*/0) == 1000);
  REQUIRE(ResolveWorkingMemoryFractionBytes(/*usefulBytes=*/1000, /*numerator=*/1, /*denominator=*/0, /*minimumBytes=*/64) == 0);
}
