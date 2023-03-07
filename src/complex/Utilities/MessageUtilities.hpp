#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Filter/IFilter.hpp"

#include <chrono>
#include <mutex>

namespace complex
{
class COMPLEX_EXPORT ThreadSafeMessenger
{
public:
  ThreadSafeMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage = "Executing...", usize milliDelay = 1000);
  ~ThreadSafeMessenger() = default;

  void updateProgress(usize counter);

  void setProgressMessage(std::string&& progressMessage);

  void setUpdateDelay(usize milliseconds);

  void setProgressIncrement(usize count);

  void setTotalElements(usize totalElements);

  usize getProgressIncrement();

  usize getTotalElements();

private:
  const IFilter::MessageHandler& m_MessageHandler;
  std::string m_ProgressMessage = "Executing...";
  usize m_MilliDelay; // Default = 1 second
  usize m_ProgressCounter = 0;
  usize m_ProgressIncrement = 100;
  usize m_TotalElements = 0;

  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};

class COMPLEX_EXPORT ThreadSafeMultiTaskMessenger
{
  using ArrayValues = std::array<usize, 3>; // {progress counter, progress increment, total elements}

  /*!!!!The C++11 standard guarantees that const method access to containers is safe from different threads (ie, both use const methods)!!!!*/

public:
  ThreadSafeMultiTaskMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage = "Executing...", usize milliDelay = 1000);
  ~ThreadSafeMultiTaskMessenger() = default;

  bool updateProgress(usize counter, uint64 id);

  void setProgressMessage(std::string&& progressMessage);

  void setUpdateDelay(usize milliseconds);

  bool addArray(uint64 id, usize progressIncrement, usize totalElements, std::string&& arrayName);

  bool setProgressIncrement(usize progressIncrement, uint64 id);

  bool setTotalElements(usize totalElements, uint64 id);

  usize getProgressIncrement(uint64 id) const;

  usize getTotalElements(uint64 id) const;

  // may need to implement a function in the future for non dataObjects to check that the id isn't already in the map

private:
  const IFilter::MessageHandler& m_MessageHandler;
  std::string m_ProgressMessage = "Executing...";
  usize m_MilliDelay; // Default = 1 second

  std::map<uint64, std::pair<ArrayValues, std::string>> m_Data = {};

  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};

} // namespace complex