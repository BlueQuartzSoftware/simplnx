#include <catch2/catch.hpp>

#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace nx::core;

TEST_CASE("CacheMemoryBudgetManager basic allocation and eviction", "[CacheMemoryBudgetManager]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  // Track eviction callbacks
  bool firstEvicted = false;
  bool secondEvicted = false;

  auto [handle1, evicted1] = mgr.allocate("chunk", "key1", 400, [&firstEvicted]() { firstEvicted = true; });
  REQUIRE(evicted1.empty());
  REQUIRE(mgr.usedBytes() == 400);

  auto [handle2, evicted2] = mgr.allocate("chunk", "key2", 400, [&secondEvicted]() { secondEvicted = true; });
  REQUIRE(evicted2.empty());
  REQUIRE(mgr.usedBytes() == 800);

  // Third allocation of 400 bytes exceeds 1000 byte budget (800 + 400 = 1200)
  // Should evict the first (oldest) entry
  bool thirdEvicted = false;
  auto [handle3, evicted3] = mgr.allocate("chunk", "key3", 400, [&thirdEvicted]() { thirdEvicted = true; });

  REQUIRE(firstEvicted);
  REQUIRE_FALSE(secondEvicted);
  REQUIRE(evicted3.size() == 1);
  REQUIRE(evicted3[0] == handle1);
  REQUIRE(mgr.usedBytes() == 800); // 400 (key2) + 400 (key3)

  // Cleanup
  mgr.release(handle2);
  mgr.release(handle3);
  REQUIRE(mgr.usedBytes() == 0);
}

TEST_CASE("CacheMemoryBudgetManager touch updates LRU order", "[CacheMemoryBudgetManager]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  bool firstEvicted = false;
  bool secondEvicted = false;

  auto [handle1, evicted1] = mgr.allocate("stride", "s1", 400, [&firstEvicted]() { firstEvicted = true; });
  REQUIRE(evicted1.empty());

  // Small delay to ensure different timestamps
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  auto [handle2, evicted2] = mgr.allocate("stride", "s2", 400, [&secondEvicted]() { secondEvicted = true; });
  REQUIRE(evicted2.empty());

  // Touch handle1 to make it more recent than handle2
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  mgr.touch(handle1);

  // Third allocation should evict handle2 (the oldest untouched) not handle1
  bool thirdEvicted = false;
  auto [handle3, evicted3] = mgr.allocate("stride", "s3", 400, [&thirdEvicted]() { thirdEvicted = true; });

  REQUIRE_FALSE(firstEvicted);
  REQUIRE(secondEvicted);
  REQUIRE(evicted3.size() == 1);
  REQUIRE(evicted3[0] == handle2);
  REQUIRE(mgr.usedBytes() == 800); // 400 (s1) + 400 (s3)

  // Cleanup
  mgr.release(handle1);
  mgr.release(handle3);
  REQUIRE(mgr.usedBytes() == 0);
}

TEST_CASE("CacheMemoryBudgetManager cross-subsystem eviction", "[CacheMemoryBudgetManager]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  bool chunkEvicted = false;
  bool strideEvicted = false;
  bool partitionEvicted = false;

  auto [handleChunk, evictedChunk] = mgr.allocate("chunk", "c1", 350, [&chunkEvicted]() { chunkEvicted = true; });
  REQUIRE(evictedChunk.empty());

  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  auto [handleStride, evictedStride] = mgr.allocate("stride", "s1", 350, [&strideEvicted]() { strideEvicted = true; });
  REQUIRE(evictedStride.empty());

  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Third allocation: used=700, needed=350, 700+350=1050 > 1000
  // Should evict the oldest entry (chunk, c1) regardless of subsystem
  auto [handlePartition, evictedPartition] = mgr.allocate("partition", "p1", 350, [&partitionEvicted]() { partitionEvicted = true; });

  REQUIRE(chunkEvicted);
  REQUIRE_FALSE(strideEvicted);
  REQUIRE_FALSE(partitionEvicted);
  REQUIRE(evictedPartition.size() == 1);
  REQUIRE(evictedPartition[0] == handleChunk);
  REQUIRE(mgr.usedBytes() == 700); // 350 (stride) + 350 (partition)

  // Cleanup
  mgr.release(handleStride);
  mgr.release(handlePartition);
  REQUIRE(mgr.usedBytes() == 0);
}

TEST_CASE("CacheMemoryBudgetManager cap and clamping", "[CacheMemoryBudgetManager]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();

  const uint64 oneGiB = uint64{1} * 1024 * 1024 * 1024;
  const uint64 maxAllowed = CacheMemoryBudgetManager::maxBudgetBytes();

  // The cap is always at least the 1 GiB floor.
  REQUIRE(maxAllowed >= oneGiB);

  // The 50%-of-RAM default never exceeds the cap.
  REQUIRE(CacheMemoryBudgetManager::defaultBudgetBytes() <= maxAllowed);

  // An over-cap request is clamped to the cap and reported as clamped.
  const bool clampedHigh = mgr.setBudgetBytes(maxAllowed + oneGiB);
  REQUIRE(clampedHigh);
  REQUIRE(mgr.budgetBytes() == maxAllowed);

  // A tiny budget is accepted verbatim (NOT raised to any floor), so the
  // existing eviction tests that set 1000-byte / 16-KiB budgets keep working.
  const bool clampedLow = mgr.setBudgetBytes(1000);
  REQUIRE_FALSE(clampedLow);
  REQUIRE(mgr.budgetBytes() == 1000);

  // Restore a sane budget for any later test that shares this singleton.
  mgr.setBudgetBytes(CacheMemoryBudgetManager::defaultBudgetBytes());
}

TEST_CASE("CacheMemoryBudgetManager registered handler receives free request on over-budget", "[CacheMemoryBudgetManager]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  uint64 lastBytesAsked = 0;
  int handlerCalls = 0;
  mgr.registerSubsystem("withHandler", [&](uint64 bytesToFree) {
    ++handlerCalls;
    lastBytesAsked = bytesToFree;
  });

  bool onEvictFired = false;
  auto [handle1, evicted1] = mgr.allocate("withHandler", "key1", 600, [&]() { onEvictFired = true; });
  REQUIRE(evicted1.empty());
  auto [handle2, evicted2] = mgr.allocate("withHandler", "key2", 600, [&]() { onEvictFired = true; });

  REQUIRE(handlerCalls == 1);
  REQUIRE(lastBytesAsked == 200);
  REQUIRE_FALSE(onEvictFired);
  REQUIRE(mgr.usedBytes() == 1200);

  mgr.registerSubsystem("withHandler", [](uint64) {});
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager unregistered subsystem still uses per-entry eviction", "[CacheMemoryBudgetManager]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  bool firstEvicted = false;
  auto [handle1, evicted1] = mgr.allocate("legacy", "key1", 400, [&]() { firstEvicted = true; });
  auto [handle2, evicted2] = mgr.allocate("legacy", "key2", 400, []() {});
  auto [handle3, evicted3] = mgr.allocate("legacy", "key3", 400, []() {});

  REQUIRE(firstEvicted);
  REQUIRE(evicted3.size() == 1);
  REQUIRE(evicted3[0] == handle1);
  REQUIRE(mgr.usedBytes() == 800);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager deferred handler drain reconciles used bytes", "[CacheMemoryBudgetManager]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  std::vector<CacheMemoryBudgetManager::AllocationHandle> handles;
  uint64 pendingFreeBytes = 0;
  mgr.registerSubsystem("deferred", [&](uint64 bytesToFree) { pendingFreeBytes += bytesToFree; });

  auto [handle1, evicted1] = mgr.allocate("deferred", "key1", 600, []() {});
  handles.push_back(handle1);
  auto [handle2, evicted2] = mgr.allocate("deferred", "key2", 600, []() {});
  handles.push_back(handle2);

  REQUIRE(pendingFreeBytes == 200);
  REQUIRE(mgr.usedBytes() == 1200);

  uint64 freedBytes = 0;
  for(const auto handle : handles)
  {
    if(freedBytes >= pendingFreeBytes)
    {
      break;
    }
    mgr.release(handle);
    freedBytes += 600;
  }

  REQUIRE(mgr.usedBytes() == 600);

  mgr.registerSubsystem("deferred", [](uint64) {});
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager pinned allocation balances nested pins", "[CacheMemoryBudgetManager][pin]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  CacheMemoryBudgetManager::AllocationOptions options;
  options.initiallyPinned = true;

  bool pinnedEntryEvicted = false;
  auto [pinnedHandle, initialEvictions] = mgr.allocate("pin-nesting", "pinned", 800, [&pinnedEntryEvicted]() { pinnedEntryEvicted = true; }, options);
  REQUIRE(initialEvictions.empty());
  REQUIRE(mgr.pin(pinnedHandle));

  auto [firstOverageHandle, firstOverageEvictions] = mgr.allocate("pin-nesting", "overage-1", 400, []() {});
  CHECK(firstOverageEvictions.empty());
  CHECK_FALSE(pinnedEntryEvicted);
  CHECK(mgr.usedBytes() == 1200);

  REQUIRE(mgr.unpin(pinnedHandle));
  mgr.release(firstOverageHandle);
  CHECK(mgr.usedBytes() == 800);

  auto [secondOverageHandle, secondOverageEvictions] = mgr.allocate("pin-nesting", "overage-2", 400, []() {});
  CHECK(secondOverageEvictions.empty());
  CHECK_FALSE(pinnedEntryEvicted);
  mgr.release(secondOverageHandle);

  REQUIRE(mgr.unpin(pinnedHandle));
  auto [replacementHandle, replacementEvictions] = mgr.allocate("pin-nesting", "replacement", 400, []() {});
  REQUIRE(replacementEvictions == std::vector{pinnedHandle});
  CHECK(pinnedEntryEvicted);
  CHECK_FALSE(mgr.pin(pinnedHandle));
  CHECK_FALSE(mgr.unpin(pinnedHandle));

  mgr.release(replacementHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager evicts the oldest unpinned allocation", "[CacheMemoryBudgetManager][pin][lru]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  CacheMemoryBudgetManager::AllocationOptions options;
  options.initiallyPinned = true;

  bool pinnedEntryEvicted = false;
  bool unpinnedEntryEvicted = false;
  auto [pinnedHandle, pinnedEvictions] = mgr.allocate("pin-lru", "oldest-pinned", 400, [&pinnedEntryEvicted]() { pinnedEntryEvicted = true; }, options);
  REQUIRE(pinnedEvictions.empty());

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  auto [unpinnedHandle, unpinnedEvictions] = mgr.allocate("pin-lru", "newer-unpinned", 400, [&unpinnedEntryEvicted]() { unpinnedEntryEvicted = true; });
  REQUIRE(unpinnedEvictions.empty());

  auto [replacementHandle, replacementEvictions] = mgr.allocate("pin-lru", "replacement", 400, []() {});
  REQUIRE(replacementEvictions == std::vector{unpinnedHandle});
  CHECK_FALSE(pinnedEntryEvicted);
  CHECK(unpinnedEntryEvicted);

  mgr.release(pinnedHandle);
  mgr.release(replacementHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager pin transitions update recency", "[CacheMemoryBudgetManager][pin][lru]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  bool firstEntryEvicted = false;
  bool secondEntryEvicted = false;
  auto [firstHandle, firstEvictions] = mgr.allocate("pin-recency", "first", 400, [&firstEntryEvicted]() { firstEntryEvicted = true; });
  REQUIRE(firstEvictions.empty());

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  auto [secondHandle, secondEvictions] = mgr.allocate("pin-recency", "second", 400, [&secondEntryEvicted]() { secondEntryEvicted = true; });
  REQUIRE(secondEvictions.empty());

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  REQUIRE(mgr.pin(firstHandle));
  REQUIRE(mgr.unpin(firstHandle));

  auto [replacementHandle, replacementEvictions] = mgr.allocate("pin-recency", "replacement", 400, []() {});
  REQUIRE(replacementEvictions == std::vector{secondHandle});
  CHECK_FALSE(firstEntryEvicted);
  CHECK(secondEntryEvicted);
  CHECK(mgr.unpin(firstHandle));

  mgr.release(firstHandle);
  mgr.release(replacementHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager reports unique pinned bytes", "[CacheMemoryBudgetManager][pin][accounting]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  CacheMemoryBudgetManager::AllocationOptions options;
  options.initiallyPinned = true;
  auto [initiallyPinnedHandle, initialEvictions] = mgr.allocate("pin-bytes", "initial", 300, []() {}, options);
  REQUIRE(initialEvictions.empty());
  auto [nestedHandle, nestedEvictions] = mgr.allocate("pin-bytes", "nested", 400, []() {});
  REQUIRE(nestedEvictions.empty());

  CHECK(mgr.pinnedBytes() == 300);
  REQUIRE(mgr.pin(nestedHandle));
  CHECK(mgr.pinnedBytes() == 700);
  REQUIRE(mgr.pin(nestedHandle));
  CHECK(mgr.pinnedBytes() == 700);
  REQUIRE(mgr.unpin(nestedHandle));
  CHECK(mgr.pinnedBytes() == 700);
  REQUIRE(mgr.unpin(nestedHandle));
  CHECK(mgr.pinnedBytes() == 300);

  mgr.release(initiallyPinnedHandle);
  CHECK(mgr.pinnedBytes() == 0);
  mgr.release(nestedHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager admits pins against unique pinned residency", "[CacheMemoryBudgetManager][pin][admission]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  CacheMemoryBudgetManager::AllocationOptions options;
  options.initiallyPinned = true;
  auto [visibleHandle, visibleEvictions] = mgr.allocate("pin-admission", "visible", 600, []() {}, options);
  REQUIRE(visibleEvictions.empty());
  auto [exactFitHandle, exactFitEvictions] = mgr.allocate("pin-admission", "exact-fit", 400, []() {});
  REQUIRE(exactFitEvictions.empty());

  REQUIRE(mgr.pinWithinBudget(exactFitHandle) == CacheMemoryBudgetManager::PinResult::Success);
  CHECK(mgr.pinnedBytes() == 1000);
  REQUIRE(mgr.pinWithinBudget(exactFitHandle) == CacheMemoryBudgetManager::PinResult::Success);
  CHECK(mgr.pinnedBytes() == 1000);

  auto [overageHandle, overageEvictions] = mgr.allocate("pin-admission", "overage", 100, []() {});
  REQUIRE(overageEvictions.empty());
  CHECK(mgr.pinWithinBudget(overageHandle) == CacheMemoryBudgetManager::PinResult::BudgetExceeded);
  CHECK(mgr.pinnedBytes() == 1000);

  mgr.release(visibleHandle);
  mgr.release(exactFitHandle);
  mgr.release(overageHandle);
  CHECK(mgr.pinWithinBudget(exactFitHandle) == CacheMemoryBudgetManager::PinResult::UnknownHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager reserves bounded temporary working bytes", "[CacheMemoryBudgetManager][pin][reservation]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  bool reusableEntryEvicted = false;
  auto [reusableHandle, reusableEvictions] = mgr.allocate("reservation-pressure", "reusable", 700, [&reusableEntryEvicted]() { reusableEntryEvicted = true; });
  REQUIRE(reusableEvictions.empty());

  const auto reservation = mgr.reservePinned("working-reservation", "request", 400);
  REQUIRE(reservation.has_value());
  CHECK(reusableEntryEvicted);
  CHECK(mgr.usedBytes() == 400);
  CHECK(mgr.pinnedBytes() == 400);

  CHECK_FALSE(mgr.reservePinned("working-reservation", "too-large", 601).has_value());
  CHECK(mgr.usedBytes() == 400);
  CHECK(mgr.pinnedBytes() == 400);

  mgr.release(*reservation);
  CHECK(mgr.usedBytes() == 0);
  CHECK(mgr.pinnedBytes() == 0);
  mgr.release(reusableHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager skips pinned delegated entries", "[CacheMemoryBudgetManager][pin][delegated]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  int handlerCalls = 0;
  mgr.registerSubsystem("pin-delegated", [&handlerCalls](uint64) { handlerCalls++; });

  CacheMemoryBudgetManager::AllocationOptions options;
  options.initiallyPinned = true;
  auto [pinnedHandle, pinnedEvictions] = mgr.allocate("pin-delegated", "pinned", 800, []() {}, options);
  REQUIRE(pinnedEvictions.empty());

  auto [overageHandle, overageEvictions] = mgr.allocate("pin-overage", "overage", 400, []() {});
  CHECK(overageEvictions.empty());
  CHECK(handlerCalls == 0);
  CHECK(mgr.usedBytes() == 1200);

  mgr.release(pinnedHandle);
  mgr.release(overageHandle);
  mgr.registerSubsystem("pin-delegated", [](uint64) {});
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager rejects handles after clear", "[CacheMemoryBudgetManager][pin][lifecycle]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1000);

  auto [handle, evictions] = mgr.allocate("pin-lifecycle", "entry", 400, []() {});
  REQUIRE(evictions.empty());
  CHECK(mgr.unpin(handle));
  REQUIRE(mgr.pin(handle));

  mgr.clear();
  CHECK_FALSE(mgr.pin(handle));
  CHECK_FALSE(mgr.unpin(handle));
  CHECK(mgr.usedBytes() == 0);
}

TEST_CASE("CacheMemoryBudgetManager preserves pinned accounting after budget reduction", "[CacheMemoryBudgetManager][pin][budget]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(1200);

  CacheMemoryBudgetManager::AllocationOptions options;
  options.initiallyPinned = true;

  bool firstEntryEvicted = false;
  bool secondEntryEvicted = false;
  auto [firstHandle, firstEvictions] = mgr.allocate("pin-budget", "first", 400, [&firstEntryEvicted]() { firstEntryEvicted = true; }, options);
  REQUIRE(firstEvictions.empty());
  auto [secondHandle, secondEvictions] = mgr.allocate("pin-budget", "second", 400, [&secondEntryEvicted]() { secondEntryEvicted = true; }, options);
  REQUIRE(secondEvictions.empty());

  mgr.setBudgetBytes(500);
  auto [overageHandle, overageEvictions] = mgr.allocate("pin-budget", "overage", 100, []() {});
  CHECK(overageEvictions.empty());
  CHECK_FALSE(firstEntryEvicted);
  CHECK_FALSE(secondEntryEvicted);
  CHECK(mgr.usedBytes() == 900);
  mgr.release(overageHandle);

  REQUIRE(mgr.unpin(firstHandle));
  auto [replacementHandle, replacementEvictions] = mgr.allocate("pin-budget", "replacement", 100, []() {});
  REQUIRE(replacementEvictions == std::vector{firstHandle});
  CHECK(firstEntryEvicted);
  CHECK_FALSE(secondEntryEvicted);
  CHECK(mgr.usedBytes() == 500);

  mgr.release(secondHandle);
  mgr.release(replacementHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager concurrent pins do not accumulate overage", "[CacheMemoryBudgetManager][pin][concurrency]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(500);

  constexpr int k_ThreadCount = 4;
  constexpr int k_Iterations = 64;

  bool pinnedEntryEvicted = false;
  auto [pinnedHandle, initialEvictions] = mgr.allocate("pin-concurrency", "visible", 400, [&pinnedEntryEvicted]() { pinnedEntryEvicted = true; });
  REQUIRE(initialEvictions.empty());

  std::barrier phaseBarrier(k_ThreadCount + 1);
  std::atomic<int> failedPins = 0;
  std::vector<std::thread> workers;
  workers.reserve(k_ThreadCount);
  for(int threadIndex = 0; threadIndex < k_ThreadCount; threadIndex++)
  {
    workers.emplace_back([&]() {
      for(int iteration = 0; iteration < k_Iterations; iteration++)
      {
        phaseBarrier.arrive_and_wait();
        if(!mgr.pin(pinnedHandle))
        {
          failedPins.fetch_add(1, std::memory_order_relaxed);
        }
        phaseBarrier.arrive_and_wait();
        phaseBarrier.arrive_and_wait();
        if(!mgr.unpin(pinnedHandle))
        {
          failedPins.fetch_add(1, std::memory_order_relaxed);
        }
        phaseBarrier.arrive_and_wait();
      }
    });
  }

  for(int iteration = 0; iteration < k_Iterations; iteration++)
  {
    phaseBarrier.arrive_and_wait();
    phaseBarrier.arrive_and_wait();

    auto [overageHandle, overageEvictions] = mgr.allocate("pin-concurrency", "temporary", 200, []() {});
    CHECK(overageEvictions.empty());
    CHECK_FALSE(pinnedEntryEvicted);
    CHECK(mgr.usedBytes() == 600);
    mgr.release(overageHandle);
    CHECK(mgr.usedBytes() == 400);

    phaseBarrier.arrive_and_wait();
    phaseBarrier.arrive_and_wait();
  }

  for(auto& worker : workers)
  {
    worker.join();
  }
  REQUIRE(failedPins.load(std::memory_order_relaxed) == 0);

  auto [replacementHandle, replacementEvictions] = mgr.allocate("pin-concurrency", "replacement", 200, []() {});
  REQUIRE(replacementEvictions == std::vector{pinnedHandle});
  CHECK(pinnedEntryEvicted);
  CHECK(mgr.usedBytes() == 200);

  mgr.release(replacementHandle);
  mgr.clear();
}

TEST_CASE("CacheMemoryBudgetManager pin overhead benchmark", "[.CacheMemoryBudgetManagerPinBenchmark]")
{
  auto& mgr = CacheMemoryBudgetManager::instance();
  mgr.clear();
  mgr.setBudgetBytes(CacheMemoryBudgetManager::defaultBudgetBytes());

  auto [handle, evictions] = mgr.allocate("pin-benchmark", "entry", 1, []() {});
  REQUIRE(evictions.empty());

  constexpr int k_Iterations = 100000;
  constexpr int k_Repetitions = 5;
  std::array<double, k_Repetitions> touchNanoseconds = {};
  std::array<double, k_Repetitions> pinNanoseconds = {};

  for(int repetition = 0; repetition < k_Repetitions; repetition++)
  {
    const auto touchStart = std::chrono::steady_clock::now();
    for(int iteration = 0; iteration < k_Iterations; iteration++)
    {
      mgr.touch(handle);
    }
    const auto touchEnd = std::chrono::steady_clock::now();
    touchNanoseconds[repetition] = std::chrono::duration<double, std::nano>(touchEnd - touchStart).count() / k_Iterations;

    bool pinOperationsSucceeded = true;
    const auto pinStart = std::chrono::steady_clock::now();
    for(int iteration = 0; iteration < k_Iterations; iteration++)
    {
      pinOperationsSucceeded = mgr.pin(handle) && pinOperationsSucceeded;
      pinOperationsSucceeded = mgr.unpin(handle) && pinOperationsSucceeded;
    }
    const auto pinEnd = std::chrono::steady_clock::now();
    pinNanoseconds[repetition] = std::chrono::duration<double, std::nano>(pinEnd - pinStart).count() / (2 * k_Iterations);
    REQUIRE(pinOperationsSucceeded);
  }

  std::sort(touchNanoseconds.begin(), touchNanoseconds.end());
  std::sort(pinNanoseconds.begin(), pinNanoseconds.end());
  const double medianTouchNanoseconds = touchNanoseconds[k_Repetitions / 2];
  const double medianPinNanoseconds = pinNanoseconds[k_Repetitions / 2];
  REQUIRE(medianTouchNanoseconds > 0.0);
  std::cout << "CACHE_BUDGET_PIN_BENCHMARK iterations=" << k_Iterations << " repetitions=" << k_Repetitions << " median_touch_ns=" << medianTouchNanoseconds
            << " median_pin_or_unpin_ns=" << medianPinNanoseconds << " ratio=" << medianPinNanoseconds / medianTouchNanoseconds << '\n';

  CHECK(medianPinNanoseconds <= medianTouchNanoseconds * 4.0);
  mgr.release(handle);
  mgr.clear();
}
