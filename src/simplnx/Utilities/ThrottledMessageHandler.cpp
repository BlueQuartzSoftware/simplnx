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
  // The final value of the last phase is almost always one the gate discarded.
  flush();
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
  // Complete the phase that is ending before its label and denominator are replaced.
  flush();
  m_Estimator.restart();
  m_MaxProgress = maxProgress;
  m_Label = std::move(label);
  m_CurrentProgress = 0;
  // Read-modify-write for the same release-sequence reason as the timer thread's wake.
  m_Ready.exchange(true, std::memory_order_acq_rel);
}

void ThrottledMessageHandler::report(PendingKind kind, std::string_view label, usize current, usize max, int32 decimals)
{
  m_PendingKind = kind;
  m_PendingLabel = label;
  m_PendingProgress = current;
  m_PendingMax = max;
  m_PendingDecimals = decimals;
  if(!isReady())
  {
    return;
  }
  flush();
}

void ThrottledMessageHandler::flush()
{
  if(m_PendingKind == PendingKind::None)
  {
    return;
  }
  // The estimate is computed only here, where a message is actually sent, so a throttled loop pays
  // nothing for it on the iterations the gate discards.
  const std::string remaining = m_Estimator.estimate(m_PendingProgress, m_PendingMax);
  if(m_PendingKind == PendingKind::Count)
  {
    m_MessageHandler.sendProgressCount(m_PendingLabel, m_PendingProgress, m_PendingMax, remaining);
  }
  else
  {
    m_MessageHandler.sendProgressPercent(m_PendingLabel, m_PendingProgress, m_PendingMax, m_PendingDecimals, remaining);
  }
  m_PendingKind = PendingKind::None;
}

void ThrottledMessageHandler::updateCount(usize currentProgress)
{
  report(PendingKind::Count, m_Label, currentProgress, m_MaxProgress, 2);
}

void ThrottledMessageHandler::updateCount(std::string_view label, usize currentProgress, usize maxProgress)
{
  report(PendingKind::Count, label, currentProgress, maxProgress, 2);
}

void ThrottledMessageHandler::updatePercent(usize currentProgress, int32 decimals)
{
  report(PendingKind::Percent, m_Label, currentProgress, m_MaxProgress, decimals);
}

void ThrottledMessageHandler::updatePercent(std::string_view label, usize currentProgress, usize maxProgress, int32 decimals)
{
  report(PendingKind::Percent, label, currentProgress, maxProgress, decimals);
}

void ThrottledMessageHandler::incrementCount(usize delta)
{
  m_CurrentProgress += delta;
  report(PendingKind::Count, m_Label, m_CurrentProgress, m_MaxProgress, 2);
}

void ThrottledMessageHandler::incrementPercent(usize delta, int32 decimals)
{
  m_CurrentProgress += delta;
  report(PendingKind::Percent, m_Label, m_CurrentProgress, m_MaxProgress, decimals);
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
