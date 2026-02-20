#include "MessageHelper.hpp"

#include <condition_variable>
#include <thread>
#include <vector>

namespace nx::core
{
struct MessageDispatcher::Impl
{
  Impl() = default;

  ~Impl() noexcept
  {
    shutdown();
  }

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  void registerChannel(std::weak_ptr<IThrottledChannel> channel)
  {
    {
      std::lock_guard lock(m_ChannelsMutex);
      m_Channels.push_back(std::move(channel));
    }

    ensureTimerRunning();
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
  }

private:
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

      // Lock channels, upgrade weak_ptrs, clean expired
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

      // Call tryFlush() outside the channels lock
      for(auto& channel : activeChannels)
      {
        channel->tryFlush();
      }
    }
  }

  static constexpr auto k_TickInterval = std::chrono::milliseconds(100);

  std::thread m_TimerThread;
  std::atomic<bool> m_Running{false};
  bool m_ThreadStarted{false};

  std::mutex m_StartMutex;
  std::mutex m_ShutdownMutex;
  std::mutex m_ChannelsMutex;
  std::condition_variable m_ShutdownCv;
  std::vector<std::weak_ptr<IThrottledChannel>> m_Channels;
};

MessageDispatcher& MessageDispatcher::instance()
{
  static MessageDispatcher dispatcher;
  return dispatcher;
}

void MessageDispatcher::registerChannel(std::weak_ptr<IThrottledChannel> channel)
{
  m_Impl->registerChannel(std::move(channel));
}

MessageDispatcher::MessageDispatcher()
: m_Impl(std::make_unique<Impl>())
{
}

MessageDispatcher::~MessageDispatcher() noexcept = default;
} // namespace nx::core
