#include "MessageUtilities.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ThreadSafeMessenger::ThreadSafeMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage, const usize milliDelay)
: m_MessageHandler(messageHandler)
, m_MilliDelay(milliDelay)
, m_InitialTime(std::chrono::steady_clock::now())
{
}

// -----------------------------------------------------------------------------
void ThreadSafeMessenger::updateProgress(size_t counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;

  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > m_MilliDelay)
  {
    auto percentage = static_cast<int32_t>((static_cast<double>(m_ProgressCounter) / static_cast<double>(m_TotalElements)) * 100.0);
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, m_ProgressMessage, percentage});
    m_InitialTime = std::chrono::steady_clock::now();
  }
}