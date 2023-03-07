#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Filter/IFilter.hpp"

#include <chrono>
#include <mutex>

namespace complex
{
class COMPLEX_EXPORT AbstractMessenger
{
public:
  AbstractMessenger() = default;
  virtual ~AbstractMessenger() = default;

  virtual void setProgressMessage(std::string&& string) = 0;

  virtual void setUpdateDelay(usize milliseconds) = 0;

  virtual void setProgressIncrement(usize count) = 0;

  virtual void setTotalElements(usize totalElements) = 0;

  virtual usize getProgressIncrement() = 0;

  virtual usize getTotalElements() = 0;
};

class COMPLEX_EXPORT ThreadSafeMessenger : public AbstractMessenger
{
public:
  ThreadSafeMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage = "Executing... ", usize milliDelay = 1000);
  ~ThreadSafeMessenger() = default;

  void updateProgress(size_t counter);

private:
  const IFilter::MessageHandler& m_MessageHandler;
  std::string m_ProgressMessage = "Executing... ";
  usize m_MilliDelay; // Default = 1 second

  size_t m_ProgressCounter = 0;
  size_t m_TotalElements = 0;

  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};

class COMPLEX_EXPORT ThreadSafeMultiTaskMessenger : public AbstractMessenger
{
public:
  ThreadSafeMultiTaskMessenger(const IFilter::MessageHandler& messageHandler, usize milliDelay = 1000);
  ~ThreadSafeMultiTaskMessenger() = default;

  void updateProgress(size_t counter);

private:
  const IFilter::MessageHandler& m_MessageHandler;
  std::string m_ProgressMessage = "Executing... ";
  usize m_MilliDelay; // Default = 1 second

  std::vector<usize> m_ProgressCounter = {0};
  std::vector<usize> m_TotalElements = {0};

  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};

} // namespace complex