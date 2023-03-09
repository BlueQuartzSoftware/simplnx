#include "MessageUtilities.hpp"

#include <sstream>

using namespace complex;

// -----------------------------------------------------------------------------
ThreadSafeMessenger::ThreadSafeMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage, const usize milliDelay)
: m_MessageHandler(messageHandler)
, m_ProgressMessage(std::move(progressMessage))
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

// -----------------------------------------------------------------------------
void ThreadSafeMessenger::setProgressMessage(std::string&& progressMessage)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressMessage = std::move(progressMessage);
}

// -----------------------------------------------------------------------------
void ThreadSafeMessenger::setUpdateDelay(usize milliseconds)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_MilliDelay = milliseconds;
}

// -----------------------------------------------------------------------------
void ThreadSafeMessenger::setProgressIncrement(usize count)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressIncrement = count;
}

// -----------------------------------------------------------------------------
void ThreadSafeMessenger::setTotalElements(usize totalElements)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_TotalElements = totalElements;
}

// -----------------------------------------------------------------------------
usize ThreadSafeMessenger::getProgressIncrement() const
{
  return m_ProgressIncrement;
}

// -----------------------------------------------------------------------------
usize ThreadSafeMessenger::getTotalElements() const
{
  return m_TotalElements;
}

// -----------------------------------------------------------------------------
ThreadSafeTaskMessenger::ThreadSafeTaskMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage, const usize milliDelay)
: m_MessageHandler(messageHandler)
, m_ProgressMessage(std::move(progressMessage))
, m_MilliDelay(milliDelay)
, m_InitialTime(std::chrono::steady_clock::now())
{
}

// -----------------------------------------------------------------------------
bool ThreadSafeTaskMessenger::updateProgress(usize counter, uint64 id)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  if(!m_Data.count(id))
  {
    return false;
  }

  (m_Data[id]).first[0] += counter;

  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > m_MilliDelay)
  {
    std::stringstream sStream;
    sStream << m_ProgressMessage;
    for(auto const& [key, pair] : m_Data)
    {
      sStream << " || " << pair.second << " : ";
      auto percentage = static_cast<int32_t>((static_cast<double>(pair.first[0]) / static_cast<double>(pair.first[2])) * 100.0);
      sStream << percentage << " %";
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, sStream.str()});
    m_InitialTime = std::chrono::steady_clock::now();
  }

  return true;
}

// -----------------------------------------------------------------------------
void ThreadSafeTaskMessenger::setProgressMessage(std::string&& progressMessage)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressMessage = std::move(progressMessage);
}

// -----------------------------------------------------------------------------
void ThreadSafeTaskMessenger::setUpdateDelay(usize milliseconds)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_MilliDelay = milliseconds;
}

// -----------------------------------------------------------------------------
bool ThreadSafeTaskMessenger::addArray(uint64 id, usize progressIncrement, usize totalElements, std::string&& arrayName)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  if(m_Data.count(id))
  {
    return false;
  }

  m_Data.emplace(id, std::make_pair<ArrayValues, std::string>({0, progressIncrement, totalElements}, std::move(arrayName)));

  return true;
}

// -----------------------------------------------------------------------------
bool ThreadSafeTaskMessenger::setProgressIncrement(usize progressIncrement, uint64 id)
{

  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  if(!m_Data.count(id))
  {
    return false;
  }

  (m_Data[id]).first[1] = progressIncrement;
  return true;
}

// -----------------------------------------------------------------------------
bool ThreadSafeTaskMessenger::setTotalElements(usize totalElements, uint64 id)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  if(!m_Data.count(id))
  {
    return false;
  }

  (m_Data[id]).first[2] = totalElements;
  return true;
}

// -----------------------------------------------------------------------------
usize ThreadSafeTaskMessenger::getProgressIncrement(uint64 id) const
{
  if(!m_Data.count(id))
  {
    // should be treated as a fail: just throw an if not around the returned
    // value to error check
    return 0;
  }

  return (m_Data.at(id)).first[1];
}

// -----------------------------------------------------------------------------
usize ThreadSafeTaskMessenger::getTotalElements(uint64 id) const
{
  if(!m_Data.count(id))
  {
    // should be treated as a fail: just throw an if not around the returned
    // value to error check
    return 0;
  }

  return (m_Data.at(id)).first[2];
}
