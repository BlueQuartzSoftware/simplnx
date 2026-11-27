#pragma once

#include "IdentifySampleBenchmarkUtilities.hpp"
#include "simplnx/Common/ScopeGuard.hpp"

#include <algorithm>
#include <limits>

/**
 * @namespace IdentifySampleBatchControlTest
 * @brief Shares exact reservation and output checks between resident and actual-OOC fixtures.
 */
namespace identify_sample_batch_control_test
{
using namespace nx::core;
using namespace identify_sample_benchmark_test;

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
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(identify_sample_batch_test::k_MaskPath));
  auto& mask = data.getDataRefAs<DataArray<T>>(identify_sample_batch_test::k_MaskPath);
  auto* originalStorePtr = mask.getDataStore();
  scope.requireExpectedStore(mask);
  const auto originalId = mask.getId();
  auto& budget = CacheMemoryBudgetManager::Instance();
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
      const auto prior = SetIdentifySampleSliceObserverForTesting({.context = &counters, .callback = &LogicalCounters::Observe});
      const auto restoreObserver = MakeScopeGuard([&]() noexcept { (void)SetIdentifySampleSliceObserverForTesting(prior); });
      REQUIRE(prior.callback == nullptr);
      IdentifySampleFilter filter;
      const auto args = identify_sample_batch_test::ArgumentsFor(2, fillHoles);
      const auto result = scope.executeFilter(filter, data, args);
      SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    }
    REQUIRE(budget.reservedWorkingMemoryBytes() == occupied.sizeBytes());
  }
  REQUIRE(budget.reservedWorkingMemoryBytes() == 0);
  REQUIRE(budget.budgetBytes() == originalBudget);
  REQUIRE(mask.getDataStore() == originalStorePtr);
  REQUIRE(mask.getId() == originalId);
  scope.requireExpectedStore(mask);
  REQUIRE_FALSE(counters.invalid);
  REQUIRE_FALSE(counters.invalid_extra_accounting);
  REQUIRE(counters.active == 0);
  REQUIRE(counters.extra_live_bytes == 0);
  REQUIRE(counters.extra_retained_bytes == 0);
  REQUIRE(counters.plane_live == 0);
  REQUIRE(counters.xy_live == 0);
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
    REQUIRE(counters.batch_count == batches);
    REQUIRE(counters.batch_planes == fixedCount);
    REQUIRE(counters.minimum_batch_width == minimumWidth);
    REQUIRE(counters.maximum_batch_width == maximumWidth);
    REQUIRE(counters.extra_request_count == batches);
    REQUIRE(counters.extra_grant_count == batches);
    REQUIRE(counters.extra_retain_count == batches);
    REQUIRE(counters.extra_reservation_release_count == batches);
    REQUIRE(counters.extra_allocation_count == owningBatches);
    REQUIRE(counters.extra_release_count == owningBatches);
    REQUIRE(counters.extra_allocated_bytes == allocatedPlanes * k_PlaneBytes);
    REQUIRE(counters.extra_peak_bytes == (maximumWidth - 1) * k_PlaneBytes);
    REQUIRE(counters.extra_retained_max_bytes == counters.extra_peak_bytes);
    REQUIRE(counters.extra_requested_max_bytes == requestedMaximum);
    REQUIRE(counters.extra_granted_max_bytes == grantedMaximum);
    REQUIRE(counters.reads == 2 * batches * 13);
    REQUIRE(counters.writes == batches * 13);
    REQUIRE(counters.read_success == counters.reads);
    REQUIRE(counters.write_success == counters.writes);
    REQUIRE(counters.maximum_span == fixedCount * 11);
    REQUIRE(counters.concurrent == 1);
    REQUIRE(counters.allocation_events == 2);
    REQUIRE(counters.release_events == 2);
    REQUIRE(counters.named_carrier_peak == (11 * 13 + fixedCount * 11) * sizeof(T));
  }
  else
  {
    // The resident BFS route does not own CCL slice carriers or reservations.
    REQUIRE(counters.reads == 0);
    REQUIRE(counters.writes == 0);
    REQUIRE(counters.allocation_events == 0);
    REQUIRE(counters.extra_allocation_count == 0);
  }
  identify_sample_batch_test::RequireOutput<T>(data, 2, fillHoles, fixedCount);
}
} // namespace identify_sample_batch_control_test
