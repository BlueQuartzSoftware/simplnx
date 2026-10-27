#pragma once

#include "IdentifySampleBenchmarkUtilities.hpp"
#include "simplnx/Common/ScopeGuard.hpp"

#include <algorithm>
#include <limits>

/**
 * @namespace IdentifySampleBatchControlTest
 * @brief Shares exact reservation and output checks between resident and actual-OOC fixtures.
 */
namespace IdentifySampleBatchControlTest
{
using namespace nx::core;
using namespace IdentifySampleBenchmarkTest;

/**
 * @brief Runs an actual filter with a real competing reservation and checks every mask value.
 * @tparam T Bool or UInt8 mask element type.
 * @tparam ScopeT Resident or private actual-OOC dispatch witness.
 * @param scope Owns the caller-selected scenario.
 * @param data Initialized mask; fixture creation occurs before budget occupation.
 * @param width Requested stable batch width from one through eight.
 * @param partialBytes Extra headroom below one whole plane.
 * @param fillHoles Selects the independent filled-hole oracle.
 * @param fixedCount Number of YZ planes.
 * @param expectCcl True when the selected dispatch path must use CCL.
 */
template <class T, class ScopeT>
void CheckGrant(ScopeT& scope, DataStructure& data, usize width, usize partialBytes, bool fillHoles, usize fixedCount = 19, bool expectCcl = true)
{
  REQUIRE(width >= 1);
  REQUIRE(width <= 8);
  constexpr uint64 k_PlaneBytes = 11 * 13 * sizeof(T);
  REQUIRE(partialBytes < k_PlaneBytes);
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
  auto* originalStore = mask.getDataStore();
  scope.requireExpectedStore(mask);
  const auto originalId = mask.getId();
  auto& budget = CacheMemoryBudgetManager::instance();
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  const uint64 originalBudget = budget.budgetBytes();
  LogicalCounters counters;
  counters.selected = &mask;
  {
    const BudgetRestore restoreBudget;
    REQUIRE_FALSE(budget.setBudgetBytes(4ULL * 1024 * 1024));
    const uint64 capacity = budget.maximumWorkingMemoryBytes();
    const uint64 headroom = (width - 1) * k_PlaneBytes + partialBytes;
    auto occupied = budget.reserveWorkingMemory(capacity - headroom);
    REQUIRE(occupied.sizeBytes() == capacity - headroom);
    {
      const auto prior = SetIdentifySampleSliceObserverForTesting({&counters, &LogicalCounters::Observe});
      const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
      REQUIRE(prior.callback == nullptr);
      IdentifySampleFilter filter;
      const auto args = IdentifySampleBatchTest::ArgumentsFor(2, fillHoles);
      const auto result = scope.executeFilter(filter, data, args);
      SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    }
    REQUIRE(budget.reservedWorkingMemoryBytes() == occupied.sizeBytes());
  }
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  REQUIRE(budget.budgetBytes() == originalBudget);
  REQUIRE(mask.getDataStore() == originalStore);
  REQUIRE(mask.getId() == originalId);
  scope.requireExpectedStore(mask);
  REQUIRE_FALSE(counters.invalid);
  REQUIRE_FALSE(counters.invalidExtraAccounting);
  REQUIRE(counters.active == 0);
  REQUIRE(counters.extraLiveBytes == 0);
  REQUIRE(counters.extraRetainedBytes == 0);
  REQUIRE(counters.planeLive == 0);
  REQUIRE(counters.xyLive == 0);
  if(expectCcl)
  {
    // Replay the admitted schedule independently. An out-of-core mask publishes its first batch without extra planes.
    const bool firstBatchSingle = mask.getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore;
    const uint64 headroom = (width - 1) * k_PlaneBytes + partialBytes;
    usize batches = 0;
    usize owningBatches = 0;
    usize minimumWidth = std::numeric_limits<usize>::max();
    usize maximumWidth = 0;
    usize allocatedPlanes = 0;
    uint64 requestedMaximum = 0;
    uint64 grantedMaximum = 0;
    for(usize start = 0; start < fixedCount; ++batches)
    {
      const usize maxExtra = firstBatchSingle && batches == 0 ? 0 : std::min(usize{7}, fixedCount - start - 1);
      const uint64 requested = maxExtra * k_PlaneBytes;
      const uint64 granted = std::min(requested, headroom);
      const usize extras = std::min<usize>(maxExtra, static_cast<usize>(granted / k_PlaneBytes));
      requestedMaximum = std::max(requestedMaximum, requested);
      grantedMaximum = std::max(grantedMaximum, granted);
      owningBatches += static_cast<usize>(extras != 0);
      allocatedPlanes += extras;
      minimumWidth = std::min(minimumWidth, extras + 1);
      maximumWidth = std::max(maximumWidth, extras + 1);
      start += extras + 1;
    }
    REQUIRE(counters.batchCount == batches);
    REQUIRE(counters.batchPlanes == fixedCount);
    REQUIRE(counters.minimumBatchWidth == minimumWidth);
    REQUIRE(counters.maximumBatchWidth == maximumWidth);
    REQUIRE(counters.extraRequestCount == batches);
    REQUIRE(counters.extraGrantCount == batches);
    REQUIRE(counters.extraRetainCount == batches);
    REQUIRE(counters.extraReservationReleaseCount == batches);
    REQUIRE(counters.extraAllocationCount == owningBatches);
    REQUIRE(counters.extraReleaseCount == owningBatches);
    REQUIRE(counters.extraAllocatedBytes == allocatedPlanes * k_PlaneBytes);
    REQUIRE(counters.extraPeakBytes == (maximumWidth - 1) * k_PlaneBytes);
    REQUIRE(counters.extraRetainedMaxBytes == counters.extraPeakBytes);
    REQUIRE(counters.extraRequestedMaxBytes == requestedMaximum);
    REQUIRE(counters.extraGrantedMaxBytes == grantedMaximum);
    REQUIRE(counters.reads == 2 * batches * 13);
    REQUIRE(counters.writes == batches * 13);
    REQUIRE(counters.readSuccess == counters.reads);
    REQUIRE(counters.writeSuccess == counters.writes);
    REQUIRE(counters.maximumSpan == fixedCount * 11);
    REQUIRE(counters.concurrent == 1);
    REQUIRE(counters.allocationEvents == 2);
    REQUIRE(counters.releaseEvents == 2);
    REQUIRE(counters.namedCarrierPeak == (11 * 13 + fixedCount * 11) * sizeof(T));
  }
  else
  {
    // The resident BFS route does not own CCL slice carriers or reservations.
    REQUIRE(counters.reads == 0);
    REQUIRE(counters.writes == 0);
    REQUIRE(counters.allocationEvents == 0);
    REQUIRE(counters.extraAllocationCount == 0);
  }
  IdentifySampleBatchTest::RequireOutput<T>(data, 2, fillHoles, fixedCount);
}
} // namespace IdentifySampleBatchControlTest
