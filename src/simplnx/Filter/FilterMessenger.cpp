#include "simplnx/Filter/FilterMessenger.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace nx::core
{

// ---------------------------------------------------------------------------
// FilterMessenger::Impl
// ---------------------------------------------------------------------------

struct FilterMessenger::Impl
{
  explicit Impl(const MessageHandler& handler)
  : m_Handler(handler)
  {
  }

  ~Impl() noexcept
  {
    shutdown();
  }

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  // -------------------------------------------------------------------------
  // Synchronous helpers
  // -------------------------------------------------------------------------

  void sendSync(Message::Type type, std::string message)
  {
    m_Handler(Message{type, std::move(message)});
  }

  void sendSyncProgress(std::string message, int32 progress)
  {
    m_Handler(ProgressMessage{Message{Message::Type::Progress, std::move(message)}, progress});
  }

  // -------------------------------------------------------------------------
  // Throttled channel management
  // -------------------------------------------------------------------------

  void setThrottledFormatter(FilterMessenger::ThrottledFormatter formatter, std::chrono::milliseconds interval)
  {
    m_ThrottledChannel = std::make_shared<ThrottledChannel<usize>>(std::move(formatter), m_Handler, interval);
    {
      std::lock_guard lock(m_ChannelsMutex);
      m_Channels.push_back(m_ThrottledChannel);
    }
    ensureTimerRunning();
  }

  void sendThrottledMessage(usize current)
  {
    if(m_ThrottledChannel)
    {
      m_ThrottledChannel->storeArgs(current);
    }
  }

  // -------------------------------------------------------------------------
  // Progress channel management
  // -------------------------------------------------------------------------

  std::shared_ptr<ProgressChannel> createProgressChannel(usize maxProgress, std::function<std::string(usize, usize)> formatter, std::chrono::milliseconds interval)
  {
    auto channel = std::make_shared<ProgressChannel>(std::move(formatter), m_Handler, maxProgress, interval);
    {
      std::lock_guard lock(m_ChannelsMutex);
      m_Channels.push_back(channel);
    }
    ensureTimerRunning();
    return channel;
  }

  // -------------------------------------------------------------------------
  // Timer thread
  // -------------------------------------------------------------------------

  void ensureTimerRunning()
  {
    std::lock_guard lock(m_StartMutex);
    if(!m_ThreadStarted)
    {
      m_Running.store(true, std::memory_order_release);
      m_TimerThread = std::thread([this]() { timerLoop(); });
      m_ThreadStarted = true;
    }
  }

  void shutdown()
  {
    {
      std::lock_guard lock(m_ShutdownMutex);
      m_Running.store(false, std::memory_order_release);
    }
    m_ShutdownCv.notify_one();

    if(m_TimerThread.joinable())
    {
      m_TimerThread.join();
    }

    // Final flush for all live channels
    std::vector<std::shared_ptr<IThrottledChannel>> activeChannels;
    {
      std::lock_guard lock(m_ChannelsMutex);
      for(auto& weakPtr : m_Channels)
      {
        if(auto channel = weakPtr.lock())
        {
          activeChannels.push_back(std::move(channel));
        }
      }
      m_Channels.clear();
    }
    for(auto& channel : activeChannels)
    {
      channel->finalFlush();
    }
  }

  void timerLoop()
  {
    while(true)
    {
      {
        std::unique_lock lock(m_ShutdownMutex);
        m_ShutdownCv.wait_for(lock, k_TickInterval, [this]() { return !m_Running.load(std::memory_order_acquire); });
      }

      if(!m_Running.load(std::memory_order_acquire))
      {
        break;
      }

      std::vector<std::shared_ptr<IThrottledChannel>> activeChannels;
      {
        std::lock_guard lock(m_ChannelsMutex);
        auto it = m_Channels.begin();
        while(it != m_Channels.end())
        {
          if(auto channel = it->lock())
          {
            activeChannels.push_back(std::move(channel));
            ++it;
          }
          else
          {
            it = m_Channels.erase(it);
          }
        }
      }

      for(auto& channel : activeChannels)
      {
        channel->tryFlush();
      }
    }
  }

  static constexpr auto k_TickInterval = std::chrono::milliseconds(100);

  const MessageHandler& m_Handler;

  // Throttled single-usize channel (optional, created by setThrottledFormatter)
  std::shared_ptr<ThrottledChannel<usize>> m_ThrottledChannel;

  // All registered channels (throttled + progress)
  std::mutex m_ChannelsMutex;
  std::vector<std::weak_ptr<IThrottledChannel>> m_Channels;

  // Timer thread management
  std::thread m_TimerThread;
  std::atomic<bool> m_Running{false};
  bool m_ThreadStarted{false};

  std::mutex m_StartMutex;
  std::mutex m_ShutdownMutex;
  std::condition_variable m_ShutdownCv;
};

// ---------------------------------------------------------------------------
// FilterMessenger
// ---------------------------------------------------------------------------

FilterMessenger::FilterMessenger(const MessageHandler& handler)
: m_Impl(std::make_unique<Impl>(handler))
{
}

FilterMessenger::~FilterMessenger() = default;

FilterMessenger::FilterMessenger(FilterMessenger&&) noexcept = default;
FilterMessenger& FilterMessenger::operator=(FilterMessenger&&) noexcept = default;

void FilterMessenger::sendInfo(std::string message)
{
  m_Impl->sendSync(Message::Type::Info, std::move(message));
}

void FilterMessenger::sendDebug(std::string message)
{
  m_Impl->sendSync(Message::Type::Debug, std::move(message));
}

void FilterMessenger::sendWarning(std::string message)
{
  m_Impl->sendSync(Message::Type::Warning, std::move(message));
}

void FilterMessenger::sendError(std::string message)
{
  m_Impl->sendSync(Message::Type::Error, std::move(message));
}

void FilterMessenger::sendProgress(std::string message, int32 progress)
{
  m_Impl->sendSyncProgress(std::move(message), progress);
}

void FilterMessenger::setThrottledFormatter(ThrottledFormatter formatter, std::chrono::milliseconds interval)
{
  m_Impl->setThrottledFormatter(std::move(formatter), interval);
}

void FilterMessenger::sendThrottledMessage(usize current)
{
  m_Impl->sendThrottledMessage(current);
}

ProgressHelper FilterMessenger::createProgressHelper(usize maxProgress, std::function<std::string(usize, usize)> formatter, std::chrono::milliseconds interval)
{
  return ProgressHelper(m_Impl->createProgressChannel(maxProgress, std::move(formatter), interval));
}

} // namespace nx::core
