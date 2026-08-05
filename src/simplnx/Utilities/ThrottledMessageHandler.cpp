#include "ThrottledMessageHandler.hpp"

#include <utility>

namespace nx::core
{
ThrottledMessageHandler::ThrottledMessageHandler(const IFilter::MessageHandler& messageHandler, std::chrono::milliseconds interval)
: m_MessageHandler(messageHandler)
, m_Interval(interval)
{
  m_Thread = std::thread([this] {
    std::unique_lock<std::mutex> lock(m_Mutex);
    // wait_for returns false on timeout, which is the signal to open the gate. It returns true only
    // once m_Stop is set, which ends the loop immediately rather than waiting out the interval.
    while(!m_ConditionVariable.wait_for(lock, m_Interval, [this] { return m_Stop; }))
    {
      m_Ready.store(true, std::memory_order_relaxed);
    }
  });
}

ThrottledMessageHandler::~ThrottledMessageHandler() noexcept
{
  {
    std::lock_guard<std::mutex> guard(m_Mutex);
    m_Stop = true;
  }
  m_ConditionVariable.notify_one();
  if(m_Thread.joinable())
  {
    m_Thread.join();
  }
}

void ThrottledMessageHandler::reset(usize maxProgress, std::string label)
{
  m_MaxProgress = maxProgress;
  m_Label = std::move(label);
  m_CurrentProgress = 0;
  m_Ready.store(true, std::memory_order_relaxed);
}

void ThrottledMessageHandler::updateCount(usize currentProgress)
{
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressCount(m_Label, currentProgress, m_MaxProgress);
}

void ThrottledMessageHandler::updatePercent(usize currentProgress, int32 decimals)
{
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressPercent(m_Label, currentProgress, m_MaxProgress, decimals);
}

void ThrottledMessageHandler::incrementCount(usize delta)
{
  m_CurrentProgress += delta;
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressCount(m_Label, m_CurrentProgress, m_MaxProgress);
}

void ThrottledMessageHandler::incrementPercent(usize delta, int32 decimals)
{
  m_CurrentProgress += delta;
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressPercent(m_Label, m_CurrentProgress, m_MaxProgress, decimals);
}

void ThrottledMessageHandler::trySendMessage(std::string message)
{
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendInfoMessage(std::move(message));
}

void ThrottledMessageHandler::setReadyForTesting()
{
  m_Ready.store(true, std::memory_order_relaxed);
}

bool ThrottledMessageHandler::isReady()
{
  // The relaxed load keeps the hot path read-only. A bare exchange on every iteration would be an
  // unconditional read-modify-write on a shared cache line, which measures 2.5x more expensive.
  if(!m_Ready.load(std::memory_order_relaxed))
  {
    return false;
  }
  return m_Ready.exchange(false, std::memory_order_relaxed);
}
} // namespace nx::core
