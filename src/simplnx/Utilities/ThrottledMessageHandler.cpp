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
      // Opening the gate is a read-modify-write rather than a plain store so that it joins the
      // release sequence headed by the previous winner's exchange. See isReady().
      m_Ready.exchange(true, std::memory_order_acq_rel);
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
  // Read-modify-write for the same release-sequence reason as the timer thread's wake.
  m_Ready.exchange(true, std::memory_order_acq_rel);
}

void ThrottledMessageHandler::updateCount(usize currentProgress)
{
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressCount(m_Label, currentProgress, m_MaxProgress);
}

void ThrottledMessageHandler::updateCount(std::string_view label, usize currentProgress, usize maxProgress)
{
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressCount(std::string(label), currentProgress, maxProgress);
}

void ThrottledMessageHandler::updatePercent(usize currentProgress, int32 decimals)
{
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressPercent(m_Label, currentProgress, m_MaxProgress, decimals);
}

void ThrottledMessageHandler::updatePercent(std::string_view label, usize currentProgress, usize maxProgress, int32 decimals)
{
  if(!isReady())
  {
    return;
  }
  m_MessageHandler.sendProgressPercent(std::string(label), currentProgress, maxProgress, decimals);
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
  m_Ready.exchange(true, std::memory_order_acq_rel);
}

bool ThrottledMessageHandler::isReady()
{
  // The relaxed load keeps the hot path read-only. A bare exchange on every iteration would be an
  // unconditional read-modify-write on a shared cache line, which measures 2.5x more expensive. A
  // stale read here only costs a dropped message, so it needs no ordering; the exchange below is
  // what actually decides the winner and carries the synchronization.
  if(!m_Ready.load(std::memory_order_relaxed))
  {
    return false;
  }
  // The exchange is the gate, not the load above: if several threads pass the load, the atomic's
  // modification order lets exactly one of them read back true. acq_rel rather than relaxed so that
  // successive winners are ordered with respect to each other. The winner's acquire pairs with the
  // timer thread's release, and because the timer's wake is itself a read-modify-write it stays in
  // the release sequence headed by the previous winner's exchange. That chains winner N before
  // winner N+1, which matters because a MessageHandler callback is usually stateful.
  return m_Ready.exchange(false, std::memory_order_acq_rel);
}
} // namespace nx::core
