#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/format.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace nx::core
{
/**
 * @class ThrottledMessageHandler
 * @brief Rate-limits progress and status messages so a tight loop can report without measurable
 * cost. The instance owns a thread that opens a gate once per interval; the loop body only reads an
 * atomic flag, which is roughly 60x cheaper than reading the clock on every iteration. Message
 * formatting happens only when a message is actually due, so a throttled loop never allocates.
 *
 * This class is NOT internally thread-safe. Use it from a single thread, or serialize access with
 * your own lock. The established pattern for a ParallelDataAlgorithm is a thread-safe seam on the
 * owning algorithm that takes the lock and forwards to this class; workers call the seam and never
 * touch an IFilter::MessageHandler directly.
 *
 * The referenced IFilter::MessageHandler must outlive this instance.
 */
class SIMPLNX_EXPORT ThrottledMessageHandler
{
public:
  static constexpr std::chrono::milliseconds k_DefaultInterval{1000};

  ThrottledMessageHandler() = delete;

  /**
   * @brief Constructs a handler that emits at most one message per interval.
   * @param messageHandler Must outlive this instance
   * @param interval Minimum time between messages
   */
  ThrottledMessageHandler(const IFilter::MessageHandler& messageHandler, std::chrono::milliseconds interval = k_DefaultInterval);

  ~ThrottledMessageHandler() noexcept;

  // Owns a thread that captures `this`, so it is neither copyable nor movable.
  ThrottledMessageHandler(const ThrottledMessageHandler&) = delete;
  ThrottledMessageHandler(ThrottledMessageHandler&&) = delete;
  ThrottledMessageHandler& operator=(const ThrottledMessageHandler&) = delete;
  ThrottledMessageHandler& operator=(ThrottledMessageHandler&&) = delete;

  /**
   * @brief Sets the denominator and label used for percent-based reporting, restarts the
   * accumulated counter, and reopens the gate so the new phase reports immediately. Call once per
   * phase before the loop that reports it.
   * @param maxProgress The denominator. A value of 0 reports 0%.
   * @param label Message text; the percent is delivered as a separate field, not appended here.
   */
  void reset(usize maxProgress, std::string label);

  /**
   * @brief Reports absolute progress as a count, rendered as "<label>: <current>/<max>". Use this
   * when the counts are meaningful to a user, e.g. tuples or slices. Nothing is stored, so this is
   * safe to call with any value.
   * @param currentProgress Items completed so far
   */
  void updateCount(usize currentProgress);

  /**
   * @brief Reports absolute progress as a percentage, rendered as "<label>: <percent>%". Use this
   * when the counts are too large to be readable.
   * @param currentProgress Items completed so far
   * @param decimals Number of decimal places to display
   */
  void updatePercent(usize currentProgress, int32 decimals = 2);

  /**
   * @brief Reports progress as a count, accumulating into a running counter.
   * @param delta Items completed since the previous call
   */
  void incrementCount(usize delta = 1);

  /**
   * @brief Reports progress as a percentage, accumulating into a running counter.
   * @param delta Items completed since the previous call
   * @param decimals Number of decimal places to display
   */
  void incrementPercent(usize delta = 1, int32 decimals = 2);

  /**
   * @brief Sends free-form throttled status text. The format string is checked at compile time and
   * the arguments are only formatted when a message is due.
   * @param format A compile-time checked format string
   * @param args Format arguments
   */
  template <class... Args>
  void queueMessage(fmt::format_string<Args...> format, Args&&... args)
  {
    if(!isReady())
    {
      return;
    }
    m_MessageHandler.sendInfoMessage(fmt::format(format, std::forward<Args>(args)...));
  }

  /**
   * @brief Sends already-formatted throttled status text. Prefer queueMessage() where possible, so
   * the string is not built on iterations that will be dropped.
   * @param message The message text
   */
  void trySendMessage(std::string message);

  /**
   * @brief Opens the gate as though the interval had elapsed. Intended for unit tests, so they can
   * observe a send without sleeping.
   */
  void setReadyForTesting();

private:
  /**
   * @brief Returns true at most once per interval. Written as a relaxed load followed by an
   * exchange so the hot path stays read-only until a send actually occurs.
   * @return
   */
  bool isReady();

  const IFilter::MessageHandler& m_MessageHandler;
  std::chrono::milliseconds m_Interval;
  std::string m_Label;
  usize m_MaxProgress = 0;
  usize m_CurrentProgress = 0;
  std::atomic<bool> m_Ready = true;
  bool m_Stop = false;
  std::mutex m_Mutex;
  std::condition_variable m_ConditionVariable;
  std::thread m_Thread;
};
} // namespace nx::core
