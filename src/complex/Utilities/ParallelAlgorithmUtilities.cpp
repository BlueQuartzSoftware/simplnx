#include "ParallelAlgorithmUtilities.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ThreadSafeMessenger::ThreadSafeMessenger(const IFilter::MessageHandler& messageHandler, const usize milliDelay)
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
    auto progressInt = static_cast<size_t>((static_cast<double>(m_ProgressCounter) / static_cast<double>(m_TotalElements)) * 100.0);
    std::string progressMessage = "Calculating... ";
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
    m_InitialTime = std::chrono::steady_clock::now();
  }
}