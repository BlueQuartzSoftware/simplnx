#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

namespace nx::core
{

class MessageHelper
{
public:
  MessageHelper(const std::atomic_bool& shouldCancel, std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
  : m_ShouldCancel(shouldCancel)
  , m_Interval(interval)
  , m_TimeSentinel(false)
  , m_StopFlag(false)
  {
    m_Thread = std::thread(&MessageHelper::run, this);
  }

  ~MessageHelper()
  {
    m_StopFlag.store(true);
    m_Condition.notify_all();
    if(m_Thread.joinable())
    {
      m_Thread.join();
    }
  }

  bool canSendMessage()
  {
    return m_TimeSentinel;
  }

  void resetTimeSentinel()
  {
    m_TimeSentinel = false;
  }

private:
  void run()
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    while(!m_StopFlag.load() && !m_ShouldCancel.load())
    {
      m_Condition.wait_for(lock, m_Interval, [this] { return m_StopFlag.load() || m_ShouldCancel.load(); });

      if(m_StopFlag.load() || m_ShouldCancel.load())
      {
        break;
      }

      m_TimeSentinel.store(true);
    }
  }

  const std::atomic_bool& m_ShouldCancel;
  std::chrono::milliseconds m_Interval;

  std::atomic_bool m_TimeSentinel;
  std::atomic_bool m_StopFlag;

  std::mutex m_Mutex;
  std::condition_variable m_Condition;
  std::thread m_Thread;
};

} // namespace nx::core
