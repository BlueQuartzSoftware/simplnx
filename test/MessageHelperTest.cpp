#include "simplnx/Utilities/MessageHelper.hpp"

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Creates a MessageHandler that appends messages to a vector.
 */
IFilter::MessageHandler createCollector(std::vector<std::string>& messages, std::mutex& mutex)
{
  return IFilter::MessageHandler{[&messages, &mutex](const IFilter::Message& msg) {
    std::lock_guard lock(mutex);
    messages.push_back(msg.message);
  }};
}
} // namespace

TEST_CASE("MessageHelper: Guaranteed synchronous send", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  MessageHelper messageHelper(handler);
  messageHelper.sendMessage("Hello");
  messageHelper.sendMessage("World");

  // Synchronous - messages should be immediately available
  REQUIRE(messages.size() == 2);
  REQUIRE(messages[0] == "Hello");
  REQUIRE(messages[1] == "World");
}

TEST_CASE("MessageHelper: ThrottledMessenger single usize argument (atomic specialization)", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    auto messenger = messageHelper.createThrottledMessenger([](usize current) { return fmt::format("Progress: {}", current); }, std::chrono::milliseconds(50));

    // Send multiple values rapidly
    for(usize i = 0; i < 1000; i++)
    {
      messenger.sendMessage(i);
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

TEST_CASE("MessageHelper: ThrottledMessenger multi-argument", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    auto messenger = messageHelper.createThrottledMessenger([](usize iteration, usize voxel, float32 error) { return fmt::format("Iter {}: voxel {} err {:.2f}", iteration, voxel, error); },
                                                            std::chrono::milliseconds(50));

    messenger.sendMessage(static_cast<usize>(5), static_cast<usize>(100), 0.42f);

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

TEST_CASE("MessageHelper: ThrottledMessenger no messages if sendMessage never called", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    auto messenger = messageHelper.createThrottledMessenger([](usize current) { return fmt::format("{}", current); }, std::chrono::milliseconds(50));

    // Don't call sendMessage at all
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  std::lock_guard lock(mutex);
  REQUIRE(messages.empty());
}

TEST_CASE("MessageHelper: ThrottledMessenger finalFlush sends last value on destruction", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    // Use a very long interval so the dispatcher won't fire during the test
    auto messenger = messageHelper.createThrottledMessenger([](usize current) { return fmt::format("Final: {}", current); }, std::chrono::milliseconds(60000));

    messenger.sendMessage(static_cast<usize>(999));
    // Don't wait -- destroy immediately
  }
  // finalFlush should have sent the last value

  std::lock_guard lock(mutex);
  REQUIRE(messages.size() == 1);
  REQUIRE(messages[0] == "Final: 999");
}

TEST_CASE("MessageHelper: ThrottledMessenger throttles to interval", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    auto messenger = messageHelper.createThrottledMessenger([](usize current) { return fmt::format("{}", current); }, std::chrono::milliseconds(200));

    // Send continuously for ~600ms
    auto start = std::chrono::steady_clock::now();
    usize counter = 0;
    while(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(600))
    {
      messenger.sendMessage(counter);
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

TEST_CASE("MessageHelper: ProgressHelper single worker", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    usize maxProgress = 1000;
    auto progressHelper = messageHelper.createProgressHelper(
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

TEST_CASE("MessageHelper: ProgressHelper multiple workers", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    usize maxProgress = 4000;
    auto progressHelper = messageHelper.createProgressHelper(
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

TEST_CASE("MessageHelper: ProgressHelper resetProgress", "[MessageHelper]")
{
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  {
    MessageHelper messageHelper(handler);
    auto progressHelper = messageHelper.createProgressHelper(
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

TEST_CASE("MessageHelper: callable_traits deduction", "[MessageHelper]")
{
  // This test verifies compile-time type deduction works correctly
  std::vector<std::string> messages;
  std::mutex mutex;
  auto handler = createCollector(messages, mutex);

  MessageHelper messageHelper(handler);

  // Lambda with usize -> should deduce ThrottledMessenger<usize>
  auto m1 = messageHelper.createThrottledMessenger([](usize v) { return fmt::format("{}", v); });
  m1.sendMessage(static_cast<usize>(42));

  // Lambda with two args -> should deduce ThrottledMessenger<usize, float32>
  auto m2 = messageHelper.createThrottledMessenger([](usize a, float32 b) { return fmt::format("{} {}", a, b); });
  m2.sendMessage(static_cast<usize>(1), 2.5f);

  // Lambda with string arg -> should deduce ThrottledMessenger<std::string>
  auto m3 = messageHelper.createThrottledMessenger([](std::string s) { return fmt::format("msg: {}", s); });
  m3.sendMessage(std::string("hello"));

  // If this compiles, the deduction is working
  REQUIRE(true);
}
