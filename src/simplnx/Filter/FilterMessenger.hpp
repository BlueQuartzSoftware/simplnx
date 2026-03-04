#pragma once

#include "simplnx/Filter/MessageHandler.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/simplnx_export.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace nx::core
{

/**
 * @brief ProgressWorker is a lightweight, copyable per-thread handle for
 * incrementing shared progress. Multiple ProgressWorkers share the same
 * ProgressChannel via shared_ptr and atomically contribute to the total count.
 * Being copyable allows use inside TBB/parallel-for functors.
 */
class SIMPLNX_EXPORT ProgressWorker
{
public:
  // Copyable: multiple workers share the same ProgressChannel via shared_ptr.
  ProgressWorker(const ProgressWorker&) = default;
  ProgressWorker& operator=(const ProgressWorker&) = default;
  ProgressWorker(ProgressWorker&&) noexcept = default;
  ProgressWorker& operator=(ProgressWorker&&) noexcept = default;
  ~ProgressWorker() = default;

  /**
   * @brief Atomically increments the shared progress counter.
   * This is the HOT PATH for parallel workers.
   * @param amount The amount to increment (default 1)
   */
  void incrementProgress(usize amount = 1)
  {
    m_Channel->incrementAndStore(amount);
  }

private:
  friend class ProgressHelper;

  explicit ProgressWorker(std::shared_ptr<ProgressChannel> channel)
  : m_Channel(std::move(channel))
  {
  }

  std::shared_ptr<ProgressChannel> m_Channel;
};

/**
 * @brief ProgressHelper manages multi-threaded progress tracking using the
 * owning FilterMessenger's background timer thread. Creates ProgressWorker
 * handles that share a single ProgressChannel. Multiple workers can atomically
 * increment the progress counter from different threads.
 */
class SIMPLNX_EXPORT ProgressHelper
{
public:
  using FormatterFunc = std::function<std::string(usize, usize)>;

  ProgressHelper() = delete;

  ~ProgressHelper()
  {
    if(m_Channel)
    {
      m_Channel->finalFlush();
    }
  }

  ProgressHelper(ProgressHelper&&) noexcept = default;
  ProgressHelper& operator=(ProgressHelper&&) noexcept = default;

  ProgressHelper(const ProgressHelper&) = delete;
  ProgressHelper& operator=(const ProgressHelper&) = delete;

  /**
   * @brief Creates a lightweight worker handle that shares the same channel.
   * Each worker thread should have its own ProgressWorker.
   * @return ProgressWorker
   */
  ProgressWorker createWorkerHandle()
  {
    return ProgressWorker(m_Channel);
  }

  /**
   * @brief Resets the progress counter to zero. Should only be called
   * when there are no active ProgressWorkers.
   */
  void resetProgress()
  {
    m_Channel->resetProgress();
  }

private:
  friend class FilterMessenger;

  explicit ProgressHelper(std::shared_ptr<ProgressChannel> channel)
  : m_Channel(std::move(channel))
  {
  }

  std::shared_ptr<ProgressChannel> m_Channel;
};

/**
 * @brief FilterMessenger is the single owner of a background timer thread and
 * provides a clean API for sending filter messages. It replaces the
 * MessageHelper + MessageDispatcher singleton stack with a per-instance,
 * RAII-managed object.
 *
 * Synchronous methods (sendInfo, sendWarning, etc.) deliver messages
 * immediately on the calling thread. The throttled path stores a usize
 * atomically and the background thread handles clock checks, formatting,
 * and delivery at most once per interval.
 *
 * The background thread is started lazily on the first call to
 * sendThrottledMessage() or createProgressHelper().
 */
class SIMPLNX_EXPORT FilterMessenger
{
public:
  explicit FilterMessenger(const MessageHandler& handler);
  ~FilterMessenger();

  FilterMessenger(FilterMessenger&&) noexcept;
  FilterMessenger& operator=(FilterMessenger&&) noexcept;

  FilterMessenger(const FilterMessenger&) = delete;
  FilterMessenger& operator=(const FilterMessenger&) = delete;

  // -------------------------------------------------------------------------
  // Synchronous sends — guaranteed delivery on the calling thread
  // -------------------------------------------------------------------------

  /**
   * @brief Sends an Info message synchronously.
   * @param message The message text
   */
  void sendInfo(std::string message);

  /**
   * @brief Sends a Debug message synchronously.
   * @param message The message text
   */
  void sendDebug(std::string message);

  /**
   * @brief Sends a Warning message synchronously.
   * @param message The message text
   */
  void sendWarning(std::string message);

  /**
   * @brief Sends an Error message synchronously.
   * @param message The message text
   */
  void sendError(std::string message);

  /**
   * @brief Sends a Progress message synchronously.
   * @param message The message text
   * @param progress The progress value (0-100)
   */
  void sendProgress(std::string message, int32 progress = 0);

  // -------------------------------------------------------------------------
  // Throttled hot-loop messaging
  // -------------------------------------------------------------------------

  using ThrottledFormatter = std::function<std::string(usize)>;

  /**
   * @brief Sets the formatter used by sendThrottledMessage(). Must be called
   * once before entering the hot loop. Starts the background timer thread if
   * not already running.
   *
   * Example:
   * @code
   * filterMessenger.setThrottledFormatter(
   *     [total](usize i) { return fmt::format("{:.2f}%", CalculatePercentComplete(i, total)); });
   * for(usize i = 0; i < total; i++) filterMessenger.sendThrottledMessage(i);
   * @endcode
   *
   * @param formatter Callable that takes usize and returns std::string
   * @param interval Minimum time between message sends (default 1000ms)
   */
  void setThrottledFormatter(ThrottledFormatter formatter, std::chrono::milliseconds interval = std::chrono::milliseconds(1000));

  /**
   * @brief Stores the current loop index atomically. This is the HOT PATH —
   * no clock check, no string construction (~10ns).
   * @param current The current loop counter value
   */
  void sendThrottledMessage(usize current);

  // -------------------------------------------------------------------------
  // Multi-threaded progress tracking
  // -------------------------------------------------------------------------

  /**
   * @brief Creates a ProgressHelper that shares this FilterMessenger's
   * background timer thread. No second thread is created.
   *
   * Example:
   * @code
   * auto progressHelper = filterMessenger.createProgressHelper(
   *     total, [](usize cur, usize max) { return fmt::format("{}/{}", cur, max); });
   * auto worker = progressHelper.createWorkerHandle();
   * // In parallel threads: worker.incrementProgress(1);
   * @endcode
   *
   * @param maxProgress The maximum progress value
   * @param formatter Callable that takes (usize current, usize max) and returns std::string
   * @param interval Minimum time between message sends (default 1000ms)
   * @return ProgressHelper
   */
  ProgressHelper createProgressHelper(usize maxProgress, std::function<std::string(usize, usize)> formatter, std::chrono::milliseconds interval = std::chrono::milliseconds(1000));

private:
  struct Impl;
  std::unique_ptr<Impl> m_Impl;
};
} // namespace nx::core
