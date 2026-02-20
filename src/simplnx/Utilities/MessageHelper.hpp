#pragma once

#include "simplnx/Filter/IFilter.hpp"
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
 * with the MessageDispatcher. Each channel stores pending message arguments and
 * is periodically flushed by the dispatcher's background timer thread.
 */
class IThrottledChannel
{
public:
  virtual ~IThrottledChannel() = default;

  /**
   * @brief Called by the dispatcher thread. Checks if new data is pending
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

/**
 * @brief MessageDispatcher is a singleton that owns a background timer thread.
 * The thread wakes every 100ms and calls tryFlush() on all registered channels.
 * Channels are held via weak_ptr and auto-cleaned when destroyed.
 */
class SIMPLNX_EXPORT MessageDispatcher
{
public:
  static MessageDispatcher& instance();

  /**
   * @brief Registers a channel with the dispatcher. The channel is stored
   * as a weak_ptr and automatically removed when no longer alive.
   * @param channel
   */
  void registerChannel(std::weak_ptr<IThrottledChannel> channel);

  MessageDispatcher(const MessageDispatcher&) = delete;
  MessageDispatcher(MessageDispatcher&&) = delete;
  MessageDispatcher& operator=(const MessageDispatcher&) = delete;
  MessageDispatcher& operator=(MessageDispatcher&&) = delete;

private:
  MessageDispatcher();
  ~MessageDispatcher() noexcept;

  struct Impl;
  std::unique_ptr<Impl> m_Impl;
};

namespace detail
{
/**
 * @brief Extracts argument types from a callable's operator().
 * Used by MessageHelper::createThrottledMessenger to deduce Args...
 * from the formatter lambda.
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
 * @tparam Args The argument types passed to sendMessage()
 */
template <typename... Args>
class ThrottledChannel : public IThrottledChannel
{
public:
  using FormatterFunc = std::function<std::string(Args...)>;

  ThrottledChannel() = delete;

  ThrottledChannel(FormatterFunc formatter, const IFilter::MessageHandler& handler, std::chrono::milliseconds interval)
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
  const IFilter::MessageHandler& m_Handler;
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

  ThrottledChannel(FormatterFunc formatter, const IFilter::MessageHandler& handler, std::chrono::milliseconds interval)
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
  const IFilter::MessageHandler& m_Handler;
  std::chrono::milliseconds m_Interval;
  std::chrono::steady_clock::time_point m_LastSendTime;

  std::atomic<usize> m_LatestValue{0};
  std::atomic<bool> m_HasNewData{false};
};

/**
 * @brief ThrottledMessenger is the user-facing handle for sending throttled
 * messages. It owns a shared_ptr to an internal ThrottledChannel that is
 * registered with the MessageDispatcher singleton.
 *
 * The sendMessage() method is the hot path -- it just stores the arguments
 * and sets an atomic flag. The background timer thread handles clock checks,
 * string formatting, and message dispatch.
 *
 * @tparam Args The argument types passed to sendMessage()
 */
template <typename... Args>
class ThrottledMessenger
{
public:
  using FormatterFunc = std::function<std::string(Args...)>;

  ThrottledMessenger() = delete;

  /**
   * @brief Constructs a ThrottledMessenger with a formatter and interval.
   * The formatter is called by the background thread to construct the message
   * string from the stored arguments. It is set once and never changes.
   * @param formatter Callable that takes Args... and returns std::string
   * @param handler The message handler callback (must remain valid for lifetime)
   * @param interval Minimum time between message sends
   */
  ThrottledMessenger(FormatterFunc formatter, const IFilter::MessageHandler& handler, std::chrono::milliseconds interval)
  : m_Channel(std::make_shared<ThrottledChannel<Args...>>(std::move(formatter), handler, interval))
  {
    MessageDispatcher::instance().registerChannel(m_Channel);
  }

  ~ThrottledMessenger()
  {
    if(m_Channel)
    {
      m_Channel->finalFlush();
    }
  }

  ThrottledMessenger(ThrottledMessenger&&) noexcept = default;
  ThrottledMessenger& operator=(ThrottledMessenger&&) noexcept = default;

  ThrottledMessenger(const ThrottledMessenger&) = delete;
  ThrottledMessenger& operator=(const ThrottledMessenger&) = delete;

  /**
   * @brief Stores the latest argument values for the background thread.
   * This is the HOT PATH -- no clock check, no string construction.
   * @param args The current values to store
   */
  void sendMessage(Args... args)
  {
    m_Channel->storeArgs(args...);
  }

private:
  std::shared_ptr<ThrottledChannel<Args...>> m_Channel;
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

  ProgressChannel(FormatterFunc formatter, const IFilter::MessageHandler& handler, usize maxProgress, std::chrono::milliseconds interval)
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
  const IFilter::MessageHandler& m_Handler;
  usize m_MaxProgress;
  std::chrono::milliseconds m_Interval;
  std::chrono::steady_clock::time_point m_LastSendTime;

  std::atomic<usize> m_CurrentProgress{0};
  std::atomic<bool> m_HasNewData{false};
};

class ProgressWorker;

/**
 * @brief ProgressHelper manages multi-threaded progress tracking.
 * Creates ProgressWorker handles that share a single ProgressChannel.
 * Multiple workers can atomically increment the progress counter from
 * different threads.
 */
class ProgressHelper
{
public:
  using FormatterFunc = std::function<std::string(usize, usize)>;

  ProgressHelper() = delete;

  /**
   * @brief Constructs a ProgressHelper with a formatter, max progress, and interval.
   * @param formatter Callable that takes (usize current, usize max) and returns std::string
   * @param handler The message handler callback (must remain valid for lifetime)
   * @param maxProgress The maximum progress value
   * @param interval Minimum time between message sends
   */
  ProgressHelper(FormatterFunc formatter, const IFilter::MessageHandler& handler, usize maxProgress, std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  : m_Channel(std::make_shared<ProgressChannel>(std::move(formatter), handler, maxProgress, interval))
  {
    MessageDispatcher::instance().registerChannel(m_Channel);
  }

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
   * @return
   */
  ProgressWorker createWorkerHandle();

  /**
   * @brief Resets the progress counter to zero. Should only be called
   * when there are no active ProgressWorkers.
   */
  void resetProgress()
  {
    m_Channel->resetProgress();
  }

private:
  std::shared_ptr<ProgressChannel> m_Channel;
};

/**
 * @brief ProgressWorker is a lightweight per-thread handle for incrementing
 * shared progress. Multiple ProgressWorkers share the same ProgressChannel
 * and atomically contribute to the total progress count.
 */
class ProgressWorker
{
public:
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

inline ProgressWorker ProgressHelper::createWorkerHandle()
{
  return ProgressWorker(m_Channel);
}

/**
 * @brief The MessageHelper class is the top-level factory for creating throttled
 * messengers and progress helpers. It stores a reference to the IFilter::MessageHandler
 * and provides convenience methods for sending messages.
 */
class MessageHelper
{
public:
  MessageHelper() = delete;

  /**
   * @brief Constructs a MessageHelper using a MessageHandler.
   * @param messageHandler The callback for message delivery (must remain valid for lifetime)
   */
  MessageHelper(const IFilter::MessageHandler& messageHandler)
  : m_MessageHandler(messageHandler)
  {
  }

  ~MessageHelper() noexcept = default;

  MessageHelper(const MessageHelper&) = delete;
  MessageHelper(MessageHelper&&) noexcept = default;

  MessageHelper& operator=(const MessageHelper&) = delete;
  MessageHelper& operator=(MessageHelper&&) = delete;

  /**
   * @brief Sends a message synchronously with guaranteed delivery.
   * Calls the MessageHandler directly on the calling thread.
   * Use for infrequent messages (start/end of algorithm, etc.).
   * @param message
   */
  void sendMessage(std::string message)
  {
    m_MessageHandler(std::move(message));
  }

  /**
   * @brief Creates a ThrottledMessenger with a formatter lambda and interval.
   * The formatter's parameter types determine the Args... template parameters.
   *
   * Example:
   * @code
   * auto messenger = messageHelper.createThrottledMessenger(
   *     [total](usize current) {
   *       return fmt::format("{:.2f}% complete", CalculatePercentComplete(current, total));
   *     },
   *     std::chrono::milliseconds(1000));
   * // Deduces: ThrottledMessenger<usize>
   * @endcode
   *
   * @tparam Formatter A callable type whose parameter types determine Args...
   * @param formatter Callable that takes Args... and returns std::string
   * @param interval Minimum time between message sends (default 1000ms)
   * @return ThrottledMessenger<Args...>
   */
  template <typename Formatter>
  auto createThrottledMessenger(Formatter&& formatter, std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  {
    return createThrottledMessengerImpl(std::forward<Formatter>(formatter), interval, typename detail::callable_traits<std::decay_t<Formatter>>::args_tuple{});
  }

  /**
   * @brief Creates a ProgressHelper for multi-threaded progress tracking.
   * @param maxProgress The maximum progress value
   * @param formatter Callable that takes (usize current, usize max) and returns std::string
   * @param interval Minimum time between message sends (default 1000ms)
   * @return ProgressHelper
   */
  ProgressHelper createProgressHelper(usize maxProgress, ProgressHelper::FormatterFunc formatter, std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  {
    return ProgressHelper(std::move(formatter), m_MessageHandler, maxProgress, interval);
  }

private:
  template <typename Formatter, typename... Args>
  ThrottledMessenger<Args...> createThrottledMessengerImpl(Formatter&& formatter, std::chrono::milliseconds interval, std::tuple<Args...>)
  {
    return ThrottledMessenger<Args...>(std::forward<Formatter>(formatter), m_MessageHandler, interval);
  }

  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
