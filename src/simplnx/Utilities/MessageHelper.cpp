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

  ~MessageHandlerSink() noexcept = default;

  MessageHandlerSink(const MessageHandlerSink&) = delete;
  MessageHandlerSink(MessageHandlerSink&&) = delete;

  MessageHandlerSink& operator=(const MessageHandlerSink&) = delete;
  MessageHandlerSink& operator=(MessageHandlerSink&&) = delete;

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    m_MessageHandler({IFilter::ProgressMessage::Type::Info, fmt::to_string(msg.payload)});
  }

  void flush_() override
  {
  }

private:
  const IFilter::MessageHandler& m_MessageHandler;
};

using MessageHandlerSink_mt = MessageHandlerSink<std::mutex>;
using MessageHandlerSink_st = MessageHandlerSink<spdlog::details::null_mutex>;
} // namespace

struct Messenger::Impl
{
  const IFilter::MessageHandler& m_MessageHandler;

  std::shared_ptr<spdlog::logger> m_ThrottledLogger = nullptr;
  std::shared_ptr<spdlog::logger> m_MandatoryLogger = nullptr;

  Impl() = delete;

  Impl(const IFilter::MessageHandler& messageHandler)
  : m_MessageHandler(messageHandler)
  {
    auto sink = std::make_shared<MessageHandlerSink_mt>(m_MessageHandler);
    spdlog::init_thread_pool(spdlog::details::default_async_q_size, 1U);
    auto threadPool = spdlog::thread_pool();
    m_MandatoryLogger = std::make_shared<spdlog::async_logger>("MessageHandlerMandatoryLogger", sink, threadPool, spdlog::async_overflow_policy::block);
    m_ThrottledLogger = std::make_shared<spdlog::async_logger>("MessageHandlerThrottledLogger", sink, threadPool, spdlog::async_overflow_policy::overrun_oldest);
  }

  ~Impl() noexcept
  {
    spdlog::shutdown();
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

Messenger::Messenger(const IFilter::MessageHandler& messageHandler)
: m_Impl(std::make_unique<Messenger::Impl>(messageHandler))
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
