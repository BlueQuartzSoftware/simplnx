#include "simplnx/Filter/FilterMessenger.hpp"

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Creates a MessageHandler that appends messages to a vector.
 */
nx::core::MessageHandler createCollector(std::vector<std::string>& messages, std::mutex& mutex)
{
  return nx::core::MessageHandler{[&messages, &mutex](const nx::core::Message& msg) {
    std::lock_guard lock(mutex);
    messages.push_back(msg.message);
  }};
}
} // namespace

TEST_CASE("FilterMessenger: Guaranteed synchronous send", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  FilterMessenger filterMessenger(handler);
  filterMessenger.sendInfo("Hello");
  filterMessenger.sendInfo("World");

  // Synchronous - messages should be immediately available
  REQUIRE(messages.size() == 2);
  REQUIRE(messages[0] == "Hello");
  REQUIRE(messages[1] == "World");
}

TEST_CASE("FilterMessenger: ThrottledMessenger single usize argument", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    filterMessenger.setThrottledFormatter([](usize current) { return fmt::format("Progress: {}", current); }, std::chrono::milliseconds(50));

    // Send multiple values rapidly
    for(usize i = 0; i < 1000; i++)
    {
      filterMessenger.sendThrottledMessage(i);
    }

    // Wait for at least one dispatch cycle
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  // After destruction, finalFlush should have fired

  std::lock_guard lock(mutex);
  // Should have received at least 1 message (from dispatch or finalFlush)
  REQUIRE(messages.size() >= 1);

  // All messages should match the format
  for(const auto& msg : messages)
  {
    REQUIRE(msg.find("Progress: ") == 0);
  }
}

TEST_CASE("FilterMessenger: ThrottledMessenger single-usize with captured secondary values", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    // Secondary values (iteration, error) are captured in the closure; only primary usize is passed
    usize capturedIteration = 5;
    float32 capturedError = 0.42f;
    filterMessenger.setThrottledFormatter([capturedIteration, capturedError](usize voxel) { return fmt::format("Iter {}: voxel {} err {:.2f}", capturedIteration, voxel, capturedError); },
                                          std::chrono::milliseconds(50));

    filterMessenger.sendThrottledMessage(static_cast<usize>(100));

    // Wait for dispatch
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  std::lock_guard lock(mutex);
  REQUIRE(messages.size() >= 1);

  // Check last message contains expected content
  bool foundExpected = false;
  for(const auto& msg : messages)
  {
    if(msg.find("Iter ") != std::string::npos && msg.find("voxel ") != std::string::npos)
    {
      foundExpected = true;
    }
  }
  REQUIRE(foundExpected);
}

TEST_CASE("FilterMessenger: ThrottledMessenger no messages if sendThrottledMessage never called", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    filterMessenger.setThrottledFormatter([](usize current) { return fmt::format("{}", current); }, std::chrono::milliseconds(50));

    // Don't call sendThrottledMessage at all
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  std::lock_guard lock(mutex);
  REQUIRE(messages.empty());
}

TEST_CASE("FilterMessenger: ThrottledMessenger finalFlush sends last value on destruction", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    // Use a very long interval so the dispatcher won't fire during the test
    filterMessenger.setThrottledFormatter([](usize current) { return fmt::format("Final: {}", current); }, std::chrono::milliseconds(60000));

    filterMessenger.sendThrottledMessage(static_cast<usize>(999));
    // Don't wait -- destroy immediately
  }
  // finalFlush should have sent the last value

  std::lock_guard lock(mutex);
  REQUIRE(messages.size() == 1);
  REQUIRE(messages[0] == "Final: 999");
}

TEST_CASE("FilterMessenger: ThrottledMessenger throttles to interval", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    filterMessenger.setThrottledFormatter([](usize current) { return fmt::format("{}", current); }, std::chrono::milliseconds(200));

    // Send continuously for ~600ms
    auto start = std::chrono::steady_clock::now();
    usize counter = 0;
    while(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(600))
    {
      filterMessenger.sendThrottledMessage(counter);
      counter++;
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }

  std::lock_guard lock(mutex);
  // With 200ms interval over 600ms, expect ~3-4 messages (plus finalFlush)
  // Allow some tolerance for timing
  REQUIRE(messages.size() >= 2);
  REQUIRE(messages.size() <= 6);
}

TEST_CASE("FilterMessenger: ProgressHelper single worker", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    usize maxProgress = 1000;
    auto progressHelper = filterMessenger.createProgressHelper(
        maxProgress, [](usize current, usize max) { return fmt::format("{}/{}", current, max); }, std::chrono::milliseconds(50));

    auto worker = progressHelper.createWorkerHandle();

    for(usize i = 0; i < maxProgress; i++)
    {
      worker.incrementProgress(1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  std::lock_guard lock(mutex);
  // Should have received messages
  REQUIRE(messages.size() >= 1);

  // The last message (from finalFlush) should report max progress
  REQUIRE(messages.back() == "1000/1000");
}

TEST_CASE("FilterMessenger: ProgressHelper multiple workers", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    usize maxProgress = 4000;
    auto progressHelper = filterMessenger.createProgressHelper(
        maxProgress, [](usize current, usize max) { return fmt::format("{}/{}", current, max); }, std::chrono::milliseconds(50));

    // Spawn 4 worker threads each incrementing 1000 times
    std::vector<std::thread> threads;
    for(int t = 0; t < 4; t++)
    {
      auto worker = progressHelper.createWorkerHandle();
      threads.emplace_back([w = std::move(worker)]() mutable {
        for(usize i = 0; i < 1000; i++)
        {
          w.incrementProgress(1);
        }
      });
    }

    for(auto& th : threads)
    {
      th.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  std::lock_guard lock(mutex);
  REQUIRE(messages.size() >= 1);

  // The last message should report total progress = 4000
  REQUIRE(messages.back() == "4000/4000");
}

TEST_CASE("FilterMessenger: ProgressHelper resetProgress", "[FilterMessenger]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);
    auto progressHelper = filterMessenger.createProgressHelper(
        100, [](usize current, usize max) { return fmt::format("{}/{}", current, max); }, std::chrono::milliseconds(50));

    auto worker = progressHelper.createWorkerHandle();
    for(usize i = 0; i < 50; i++)
    {
      worker.incrementProgress(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Reset and do another batch
    progressHelper.resetProgress();
    for(usize i = 0; i < 30; i++)
    {
      worker.incrementProgress(1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  std::lock_guard lock(mutex);
  REQUIRE(messages.size() >= 2);
  // Last message should show 30 (after reset)
  REQUIRE(messages.back() == "30/100");
}

TEST_CASE("FilterMessenger: setThrottledFormatter with different interval values", "[FilterMessenger]")
{
  // This test verifies that setThrottledFormatter works correctly with different interval values
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    FilterMessenger filterMessenger(handler);

    // Set formatter with a short interval
    filterMessenger.setThrottledFormatter([](usize v) { return fmt::format("short: {}", v); }, std::chrono::milliseconds(50));
    filterMessenger.sendThrottledMessage(static_cast<usize>(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Re-set formatter with a long interval before sending the next value
    filterMessenger.setThrottledFormatter([](usize v) { return fmt::format("long: {}", v); }, std::chrono::milliseconds(60000));
    filterMessenger.sendThrottledMessage(static_cast<usize>(42));
    // finalFlush fires on destruction
  }

  std::lock_guard lock(mutex);
  // Should have received at least one message from each formatter phase
  REQUIRE(messages.size() >= 1);
  // Last message should come from the long-interval formatter (finalFlush)
  REQUIRE(messages.back() == "long: 42");
}
