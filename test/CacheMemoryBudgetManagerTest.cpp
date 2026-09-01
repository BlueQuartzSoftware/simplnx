#include <catch2/catch.hpp>

#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

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
