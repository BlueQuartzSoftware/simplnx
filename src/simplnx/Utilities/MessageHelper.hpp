#pragma once

#include "simplnx/Filter/MessageHandler.hpp"
#include "simplnx/simplnx_export.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>

namespace nx::core
{
/**
 * @brief Calculates the percent complete
 * @tparam T
 * @param currentProgress
 * @param max
 * @return
 */
template <class T = float32>
inline constexpr T CalculatePercentComplete(usize currentProgress, usize max)
{
  return static_cast<T>(static_cast<float32>(currentProgress) / static_cast<float32>(max) * 100.0f);
}

/**
 * @brief IThrottledChannel is the type-erased interface for channels registered
 * with a FilterMessenger. Each channel stores pending message arguments and
 * is periodically flushed by the messenger's background timer thread.
 */
class IThrottledChannel
{
public:
  virtual ~IThrottledChannel() = default;

  /**
   * @brief Called by the timer thread. Checks if new data is pending
   * and if enough time has elapsed since the last send. If so, formats
   * and sends the message. Returns true if a message was sent.
   * @return
   */
  virtual bool tryFlush() = 0;

  /**
   * @brief Called on destruction of the owning handle. Sends the last
   * pending message regardless of the interval, to avoid losing the
   * final progress update.
   */
  virtual void finalFlush() = 0;
};

namespace detail
{
/**
 * @brief Extracts argument types from a callable's operator().
 */
template <typename T>
struct callable_traits;

template <typename R, typename C, typename... Args>
struct callable_traits<R (C::*)(Args...) const>
{
  using args_tuple = std::tuple<std::decay_t<Args>...>;
};

template <typename R, typename C, typename... Args>
struct callable_traits<R (C::*)(Args...)>
{
  using args_tuple = std::tuple<std::decay_t<Args>...>;
};

template <typename T>
struct callable_traits : callable_traits<decltype(&T::operator())>
{
};

/**
 * @brief Specialization for function pointers.
 */
template <typename R, typename... Args>
struct callable_traits<R (*)(Args...)>
{
  using args_tuple = std::tuple<std::decay_t<Args>...>;
};

template <typename R, typename... Args>
struct callable_traits<R (&)(Args...)>
{
  using args_tuple = std::tuple<std::decay_t<Args>...>;
};
} // namespace detail

/**
 * @brief ThrottledChannel is the general-purpose channel implementation.
 * Stores the latest arguments under a mutex and uses an atomic flag to
 * indicate new data is available.
 * @tparam Args The argument types passed to storeArgs()
 */
template <typename... Args>
class ThrottledChannel : public IThrottledChannel
{
public:
  using FormatterFunc = std::function<std::string(Args...)>;

  ThrottledChannel() = delete;

  ThrottledChannel(FormatterFunc formatter, const MessageHandler& handler, std::chrono::milliseconds interval)
  : m_Formatter(std::move(formatter))
  , m_Handler(handler)
  , m_Interval(interval)
  , m_LastSendTime(std::chrono::steady_clock::now())
  {
  }

  ~ThrottledChannel() noexcept override = default;

  ThrottledChannel(const ThrottledChannel&) = delete;
  ThrottledChannel(ThrottledChannel&&) = delete;
  ThrottledChannel& operator=(const ThrottledChannel&) = delete;
  ThrottledChannel& operator=(ThrottledChannel&&) = delete;

  /**
   * @brief Stores the latest arguments. Called from the filter's hot loop.
   * Cost: ~15-25ns (one uncontended mutex lock + one atomic store).
   * @param args The current values to store
   */
  void storeArgs(Args... args)
  {
    {
      std::lock_guard lock(m_ArgsMutex);
      m_LatestArgs = std::make_tuple(args...);
    }
    m_HasNewData.store(true, std::memory_order_release);
  }

  bool tryFlush() override
  {
    if(!m_HasNewData.load(std::memory_order_acquire))
    {
      return false;
    }

    auto now = std::chrono::steady_clock::now();
    if(now - m_LastSendTime < m_Interval)
    {
      return false;
    }

    return doFlush(now);
  }

  void finalFlush() override
  {
    if(m_HasNewData.load(std::memory_order_acquire))
    {
      doFlush(std::chrono::steady_clock::now());
    }
  }

private:
  bool doFlush(std::chrono::steady_clock::time_point now)
  {
    m_LastSendTime = now;
    m_HasNewData.store(false, std::memory_order_relaxed);

    std::tuple<std::decay_t<Args>...> args;
    {
      std::lock_guard lock(m_ArgsMutex);
      args = m_LatestArgs;
    }

    auto message = std::apply(m_Formatter, args);
    m_Handler(message);
    return true;
  }

  FormatterFunc m_Formatter;
  const MessageHandler& m_Handler;
  std::chrono::milliseconds m_Interval;
  std::chrono::steady_clock::time_point m_LastSendTime;

  std::mutex m_ArgsMutex;
  std::tuple<std::decay_t<Args>...> m_LatestArgs{};
  std::atomic<bool> m_HasNewData{false};
};

/**
 * @brief Atomic specialization of ThrottledChannel for a single usize argument.
 * Uses std::atomic<usize> instead of mutex for maximum hot-path performance.
 * Cost: ~10ns (two atomic stores, no mutex).
 */
template <>
class ThrottledChannel<usize> : public IThrottledChannel
{
public:
  using FormatterFunc = std::function<std::string(usize)>;

  ThrottledChannel() = delete;

  ThrottledChannel(FormatterFunc formatter, const MessageHandler& handler, std::chrono::milliseconds interval)
  : m_Formatter(std::move(formatter))
  , m_Handler(handler)
  , m_Interval(interval)
  , m_LastSendTime(std::chrono::steady_clock::now())
  {
  }

  ~ThrottledChannel() noexcept override = default;

  ThrottledChannel(const ThrottledChannel&) = delete;
  ThrottledChannel(ThrottledChannel&&) = delete;
  ThrottledChannel& operator=(const ThrottledChannel&) = delete;
  ThrottledChannel& operator=(ThrottledChannel&&) = delete;

  void storeArgs(usize value)
  {
    m_LatestValue.store(value, std::memory_order_relaxed);
    m_HasNewData.store(true, std::memory_order_release);
  }

  bool tryFlush() override
  {
    if(!m_HasNewData.load(std::memory_order_acquire))
    {
      return false;
    }

    auto now = std::chrono::steady_clock::now();
    if(now - m_LastSendTime < m_Interval)
    {
      return false;
    }

    return doFlush(now);
  }

  void finalFlush() override
  {
    if(m_HasNewData.load(std::memory_order_acquire))
    {
      doFlush(std::chrono::steady_clock::now());
    }
  }

private:
  bool doFlush(std::chrono::steady_clock::time_point now)
  {
    m_LastSendTime = now;
    m_HasNewData.store(false, std::memory_order_relaxed);
    auto value = m_LatestValue.load(std::memory_order_relaxed);
    auto message = m_Formatter(value);
    m_Handler(message);
    return true;
  }

  FormatterFunc m_Formatter;
  const MessageHandler& m_Handler;
  std::chrono::milliseconds m_Interval;
  std::chrono::steady_clock::time_point m_LastSendTime;

  std::atomic<usize> m_LatestValue{0};
  std::atomic<bool> m_HasNewData{false};
};

/**
 * @brief ProgressChannel is a channel for multi-threaded progress tracking.
 * Uses an atomic counter for thread-safe progress accumulation from multiple
 * worker threads. The formatter always takes (usize current, usize max).
 */
class ProgressChannel : public IThrottledChannel
{
public:
  using FormatterFunc = std::function<std::string(usize, usize)>;

  ProgressChannel() = delete;

  ProgressChannel(FormatterFunc formatter, const MessageHandler& handler, usize maxProgress, std::chrono::milliseconds interval)
  : m_Formatter(std::move(formatter))
  , m_Handler(handler)
  , m_MaxProgress(maxProgress)
  , m_Interval(interval)
  , m_LastSendTime(std::chrono::steady_clock::now())
  {
  }

  ~ProgressChannel() noexcept override = default;

  ProgressChannel(const ProgressChannel&) = delete;
  ProgressChannel(ProgressChannel&&) = delete;
  ProgressChannel& operator=(const ProgressChannel&) = delete;
  ProgressChannel& operator=(ProgressChannel&&) = delete;

  /**
   * @brief Atomically increments the progress counter and flags new data.
   * Called from worker threads' hot loops.
   * @param amount The amount to increment
   */
  void incrementAndStore(usize amount)
  {
    m_CurrentProgress.fetch_add(amount, std::memory_order_relaxed);
    m_HasNewData.store(true, std::memory_order_release);
  }

  /**
   * @brief Resets the progress counter to zero. Should only be called
   * when there are no active workers.
   */
  void resetProgress()
  {
    m_CurrentProgress.store(0, std::memory_order_relaxed);
    m_HasNewData.store(false, std::memory_order_relaxed);
  }

  bool tryFlush() override
  {
    if(!m_HasNewData.load(std::memory_order_acquire))
    {
      return false;
    }

    auto now = std::chrono::steady_clock::now();
    if(now - m_LastSendTime < m_Interval)
    {
      return false;
    }

    return doFlush(now);
  }

  void finalFlush() override
  {
    if(m_HasNewData.load(std::memory_order_acquire))
    {
      doFlush(std::chrono::steady_clock::now());
    }
  }

private:
  bool doFlush(std::chrono::steady_clock::time_point now)
  {
    m_LastSendTime = now;
    m_HasNewData.store(false, std::memory_order_relaxed);
    auto current = m_CurrentProgress.load(std::memory_order_relaxed);
    auto message = m_Formatter(current, m_MaxProgress);
    m_Handler(message);
    return true;
  }

  FormatterFunc m_Formatter;
  const MessageHandler& m_Handler;
  usize m_MaxProgress;
  std::chrono::milliseconds m_Interval;
  std::chrono::steady_clock::time_point m_LastSendTime;

  std::atomic<usize> m_CurrentProgress{0};
  std::atomic<bool> m_HasNewData{false};
};

} // namespace nx::core
