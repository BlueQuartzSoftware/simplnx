#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief An interval long enough that the owned ticker thread will never open the gate during a
 * test. Tests open it explicitly instead, so no test depends on wall-clock time.
 */
constexpr std::chrono::milliseconds k_NeverFires{3'600'000};

/**
 * @brief Records every message emitted through a MessageHandler so tests can assert on them.
 * Access is synchronized because the throttle may be driven from several threads.
 */
class MessageRecorder
{
public:
  IFilter::MessageHandler createHandler()
  {
    return IFilter::MessageHandler{[this](const IFilter::Message& message) {
      std::lock_guard<std::mutex> guard(m_Mutex);
      m_Messages.push_back(message);
    }};
  }

  usize size() const
  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    return m_Messages.size();
  }

  IFilter::Message at(usize index) const
  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    return m_Messages.at(index);
  }

private:
  mutable std::mutex m_Mutex;
  std::vector<IFilter::Message> m_Messages;
};
} // namespace

TEST_CASE("Simplnx::ThrottledMessageHandler::updateCount reports counts", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);
  throttle.reset(200, "Processing tuples");

  throttle.updateCount(50);

  REQUIRE(recorder.size() == 1);
  IFilter::Message message = recorder.at(0);
  REQUIRE(message.type == IFilter::Message::Type::Progress);
  REQUIRE(message.message == "Processing tuples: 50/200");
  REQUIRE(message.progress == 25);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::updatePercent reports decimals", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);
  throttle.reset(3, "Analyzing voxels");

  throttle.updatePercent(1);

  REQUIRE(recorder.size() == 1);
  IFilter::Message message = recorder.at(0);
  REQUIRE(message.type == IFilter::Message::Type::Progress);
  REQUIRE(message.message == "Analyzing voxels: 33.33%");
  REQUIRE(message.progress == 33);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::Throttles until the gate reopens", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);
  throttle.reset(200, "Analyzing data");

  throttle.updateCount(10);
  REQUIRE(recorder.size() == 1);

  // The gate was consumed by the first send, so these are dropped.
  throttle.updateCount(60);
  throttle.updateCount(70);
  REQUIRE(recorder.size() == 1);

  throttle.setReadyForTesting();
  throttle.updateCount(100);

  REQUIRE(recorder.size() == 2);
  REQUIRE(recorder.at(1).message == "Analyzing data: 100/200");
  REQUIRE(recorder.at(1).progress == 50);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::incrementCount accumulates across calls", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);
  throttle.reset(4, "Working");

  throttle.incrementCount();
  REQUIRE(recorder.size() == 1);
  REQUIRE(recorder.at(0).message == "Working: 1/4");

  // Dropped, but still counted.
  throttle.incrementCount();
  throttle.incrementCount();
  REQUIRE(recorder.size() == 1);

  throttle.setReadyForTesting();
  throttle.incrementCount();

  REQUIRE(recorder.size() == 2);
  REQUIRE(recorder.at(1).message == "Working: 4/4");
  REQUIRE(recorder.at(1).progress == 100);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::incrementPercent accumulates across calls", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);
  throttle.reset(3, "Working");

  throttle.incrementPercent();
  REQUIRE(recorder.at(0).message == "Working: 33.33%");

  throttle.setReadyForTesting();
  throttle.incrementPercent();
  REQUIRE(recorder.at(1).message == "Working: 66.67%");
}

TEST_CASE("Simplnx::ThrottledMessageHandler::reset relabels and rearms", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);

  throttle.reset(4, "Phase 1");
  throttle.incrementCount();
  REQUIRE(recorder.size() == 1);
  REQUIRE(recorder.at(0).message == "Phase 1: 1/4");

  // reset() rearms the gate so each phase reports its first message immediately, and restarts
  // the accumulated counter against the new denominator.
  throttle.reset(2, "Phase 2");
  throttle.incrementCount();

  REQUIRE(recorder.size() == 2);
  REQUIRE(recorder.at(1).message == "Phase 2: 1/2");
  REQUIRE(recorder.at(1).progress == 50);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::queueMessage formats free-form status", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);

  throttle.queueMessage("Reading slice {} of {}", 3, 12);

  REQUIRE(recorder.size() == 1);
  IFilter::Message message = recorder.at(0);
  REQUIRE(message.type == IFilter::Message::Type::Info);
  REQUIRE(message.message == "Reading slice 3 of 12");
  REQUIRE(message.progress == -1);

  throttle.queueMessage("Reading slice {} of {}", 4, 12);
  REQUIRE(recorder.size() == 1);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::trySendMessage sends pre-formatted text", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);

  throttle.trySendMessage("Resampling Data Array 'Foo' Complete");

  REQUIRE(recorder.size() == 1);
  REQUIRE(recorder.at(0).type == IFilter::Message::Type::Info);
  REQUIRE(recorder.at(0).message == "Resampling Data Array 'Foo' Complete");

  throttle.trySendMessage("dropped");
  REQUIRE(recorder.size() == 1);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::Zero denominator is safe", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);

  throttle.reset(0, "Nothing to do");
  REQUIRE_NOTHROW(throttle.updateCount(0));
  REQUIRE(recorder.size() == 1);
  REQUIRE(recorder.at(0).message == "Nothing to do: 0/0");
  REQUIRE(recorder.at(0).progress == 0);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::Elects a single sender across threads", "[Simplnx][ThrottledMessageHandler]")
{
  static constexpr usize k_ThreadCount = 8;
  static constexpr usize k_PerThread = 1000;

  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, k_NeverFires);
  throttle.reset(k_ThreadCount * k_PerThread, "Converting");

  std::mutex progressMutex;
  std::vector<std::thread> threads;
  threads.reserve(k_ThreadCount);

  for(usize t = 0; t < k_ThreadCount; t++)
  {
    threads.emplace_back([&throttle, &progressMutex] {
      for(usize i = 0; i < k_PerThread; i++)
      {
        std::lock_guard<std::mutex> guard(progressMutex);
        throttle.incrementCount();
      }
    });
  }
  for(std::thread& thread : threads)
  {
    thread.join();
  }

  // The gate started open, so exactly one of the 8000 calls won it. No test sleeps, so the ticker
  // thread never reopens it.
  REQUIRE(recorder.size() == 1);

  // Every increment was counted even though only one produced a message.
  throttle.setReadyForTesting();
  throttle.incrementCount(0);
  REQUIRE(recorder.size() == 2);
  REQUIRE(recorder.at(1).message == "Converting: 8000/8000");
  REQUIRE(recorder.at(1).progress == 100);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::Destruction does not wait for the interval", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;

  auto start = std::chrono::steady_clock::now();
  {
    IFilter::MessageHandler handler = recorder.createHandler();
    ThrottledMessageHandler throttle(handler, k_NeverFires);
    throttle.reset(10, "Working");
    throttle.updateCount(1);
  }
  auto elapsed = std::chrono::steady_clock::now() - start;

  // The ticker thread must be signalled rather than waited out.
  REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < 1000);
}

TEST_CASE("Simplnx::ThrottledMessageHandler::The ticker thread opens the gate", "[Simplnx][ThrottledMessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();
  ThrottledMessageHandler throttle(handler, std::chrono::milliseconds(50));
  throttle.reset(100, "Working");

  throttle.updateCount(10);
  REQUIRE(recorder.size() == 1);

  throttle.updateCount(20);
  REQUIRE(recorder.size() == 1);

  // This is the one place a test waits on the real thread, because the thread firing at all is
  // the behavior under test.
  std::this_thread::sleep_for(std::chrono::milliseconds(250));

  throttle.updateCount(30);
  REQUIRE(recorder.size() == 2);
  REQUIRE(recorder.at(1).progress == 30);
}
