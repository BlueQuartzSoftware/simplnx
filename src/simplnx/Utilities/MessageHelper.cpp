#include "MessageHelper.hpp"

#include <fmt/format.h>

#include <spdlog/async.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include <mutex>

namespace nx::core
{
namespace
{
template <class Mutex>
class MessageHandlerSink : public spdlog::sinks::base_sink<Mutex>
{
public:
  MessageHandlerSink(const IFilter::MessageHandler& messageHandler)
  : spdlog::sinks::base_sink<Mutex>()
  , m_MessageHandler(messageHandler)
  {
  }

  ~MessageHandlerSink() noexcept override = default;

  MessageHandlerSink(const MessageHandlerSink&) = delete;
  MessageHandlerSink(MessageHandlerSink&&) = delete;

  MessageHandlerSink& operator=(const MessageHandlerSink&) = delete;
  MessageHandlerSink& operator=(MessageHandlerSink&&) = delete;

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    m_MessageHandler.sendInfoMessage(fmt::to_string(msg.payload));
  }

  void flush_() override
  {
  }

private:
  const IFilter::MessageHandler& m_MessageHandler;
};

using MessageHandlerSink_mt = MessageHandlerSink<std::mutex>;
using MessageHandlerSink_st = MessageHandlerSink<spdlog::details::null_mutex>;

template <class Mutex, class BaseDuration = std::chrono::milliseconds>
class ThrottleSink : public spdlog::sinks::base_sink<Mutex>
{
public:
  ThrottleSink() = delete;

  explicit ThrottleSink(BaseDuration rate)
  : m_Rate(std::move(rate))
  , m_Sinks()
  , m_LastTime(spdlog::log_clock::time_point(BaseDuration::min()))
  {
  }

  ThrottleSink(BaseDuration rate, std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks)
  : m_Rate(std::move(rate))
  , m_Sinks(std::move(sinks))
  , m_LastTime(spdlog::log_clock::time_point(BaseDuration::min()))
  {
  }

  ~ThrottleSink() noexcept override = default;

  ThrottleSink(const ThrottleSink&) = delete;
  ThrottleSink(ThrottleSink&&) = delete;

  ThrottleSink& operator=(const ThrottleSink&) = delete;
  ThrottleSink& operator=(ThrottleSink&&) = delete;

  void add_sink(std::shared_ptr<spdlog::sinks::sink> sink)
  {
    std::lock_guard<Mutex> lock(BaseSink::mutex_);
    m_Sinks.push_back(sink);
  }

  void remove_sink(std::shared_ptr<spdlog::sinks::sink> sink)
  {
    std::lock_guard<Mutex> lock(BaseSink::mutex_);
    m_Sinks.erase(std::remove(m_Sinks.begin(), m_Sinks.end(), sink), m_Sinks.end());
  }

  void set_sinks(std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks)
  {
    std::lock_guard<Mutex> lock(BaseSink::mutex_);
    m_Sinks = std::move(sinks);
  }

  std::vector<std::shared_ptr<spdlog::sinks::sink>>& sinks()
  {
    return m_Sinks;
  }

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    auto diff = msg.time - m_LastTime;
    if(diff >= m_Rate)
    {
      m_LastTime = msg.time;
    }
    else
    {
      return;
    }

    for(auto& childSink : m_Sinks)
    {
      if(childSink->should_log(msg.level))
      {
        childSink->log(msg);
      }
    }
  }

  void flush_() override
  {
    for(auto& childSink : m_Sinks)
    {
      childSink->flush();
    }
  }

  void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override
  {
    BaseSink::formatter_ = std::move(sink_formatter);
    for(auto& childSink : m_Sinks)
    {
      childSink->set_formatter(BaseSink::formatter_->clone());
    }
  }

private:
  using BaseSink = spdlog::sinks::base_sink<Mutex>;

  BaseDuration m_Rate;
  std::vector<std::shared_ptr<spdlog::sinks::sink>> m_Sinks;
  spdlog::log_clock::time_point m_LastTime;
};

using ThrottleSink_mt = ThrottleSink<std::mutex>;
using ThrottleSink_st = ThrottleSink<spdlog::details::null_mutex>;

class ThreadPool
{
public:
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;

  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  static ThreadPool& GetInstance()
  {
    static ThreadPool threadPool;
    return threadPool;
  }

  std::shared_ptr<spdlog::details::thread_pool> GetOrCreateThreadPool()
  {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);

    if(m_ThreadPool == nullptr)
    {
      m_ThreadPool = std::make_shared<spdlog::details::thread_pool>(spdlog::details::default_async_q_size, 1U);
    }

    return m_ThreadPool;
  }

  void tryReset()
  {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    if(m_ThreadPool.use_count() == 1)
    {
      m_ThreadPool = nullptr;
    }
  }

private:
  ThreadPool() = default;
  ~ThreadPool() noexcept = default;

  std::shared_ptr<spdlog::details::thread_pool> m_ThreadPool = nullptr;
  std::recursive_mutex m_Mutex;
};
} // namespace

struct Messenger::Impl
{
  const IFilter::MessageHandler& m_MessageHandler;
  std::chrono::milliseconds m_ThrottleRate;

  std::shared_ptr<spdlog::logger> m_ThrottledLogger = nullptr;
  std::shared_ptr<spdlog::logger> m_MandatoryLogger = nullptr;
  std::shared_ptr<spdlog::details::thread_pool> m_ThreadPool = nullptr;

  static constexpr StringLiteral k_MandatoryLoggerName = "MessageHandlerMandatoryLogger";
  static constexpr StringLiteral k_ThrottledLoggerName = "MessageHandlerThrottledLogger";

  Impl() = delete;

  Impl(const IFilter::MessageHandler& messageHandler, std::chrono::milliseconds throttleRate)
  : m_MessageHandler(messageHandler)
  , m_ThrottleRate(throttleRate)
  {
    auto sink = std::make_shared<MessageHandlerSink_mt>(m_MessageHandler);
    auto throttledSink = std::make_shared<ThrottleSink_mt>(m_ThrottleRate, std::vector<std::shared_ptr<spdlog::sinks::sink>>{sink});
    m_ThreadPool = ThreadPool::GetInstance().GetOrCreateThreadPool();
    m_MandatoryLogger = std::make_shared<spdlog::async_logger>(k_MandatoryLoggerName, sink, m_ThreadPool, spdlog::async_overflow_policy::block);
    m_ThrottledLogger = std::make_shared<spdlog::async_logger>(k_ThrottledLoggerName, throttledSink, m_ThreadPool, spdlog::async_overflow_policy::overrun_oldest);
  }

  ~Impl() noexcept
  {
    // The thread pool can be shared between filters i.e. one filter calls another.
    // If this is the last instance we can safely destruct the thread pool
    m_ThreadPool = nullptr;
    ThreadPool::GetInstance().tryReset();
  }

  Impl(const Impl&) = delete;
  Impl(Impl&&) noexcept = delete;

  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) noexcept = delete;

  void sendMessage(std::string message)
  {
    m_MandatoryLogger->info(message);
  }

  void trySendMessage(std::string message)
  {
    m_ThrottledLogger->info(message);
  }
};

Messenger::Messenger(const IFilter::MessageHandler& messageHandler, std::chrono::milliseconds throttleRate)
: m_Impl(std::make_unique<Messenger::Impl>(messageHandler, throttleRate))
{
}

Messenger::~Messenger() noexcept = default;

Messenger::Messenger(Messenger&&) noexcept = default;

Messenger& Messenger::operator=(Messenger&&) noexcept = default;

void Messenger::sendMessage(std::string message)
{
  m_Impl->sendMessage(std::move(message));
}

void Messenger::trySendMessage(std::string message)
{
  m_Impl->trySendMessage(std::move(message));
}
} // namespace nx::core
