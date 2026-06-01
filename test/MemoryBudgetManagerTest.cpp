#include <catch2/catch.hpp>

#include "simplnx/Utilities/MemoryBudgetManager.hpp"

#include <thread>

using namespace nx::core;

TEST_CASE("MemoryBudgetManager basic allocation and eviction", "[MemoryBudgetManager]")
{
  auto& mgr = MemoryBudgetManager::instance();
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

TEST_CASE("MemoryBudgetManager touch updates LRU order", "[MemoryBudgetManager]")
{
  auto& mgr = MemoryBudgetManager::instance();
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

TEST_CASE("MemoryBudgetManager cross-subsystem eviction", "[MemoryBudgetManager]")
{
  auto& mgr = MemoryBudgetManager::instance();
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
