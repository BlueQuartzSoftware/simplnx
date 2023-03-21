#pragma once

#include "complex/Common/Types.hpp"
#include "complex/Filter/IFilter.hpp"

#include <chrono>
#include <mutex>

namespace complex
{
/**
 * @class ThreadSafeMessenger
 * @brief This is a thread safe message handler based on the intended use case of ParallelDataAlgorithms
 */
class COMPLEX_EXPORT ThreadSafeMessenger
{
public:
  ThreadSafeMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage = "Executing...", usize milliDelay = 1000);
  ~ThreadSafeMessenger() = default;

  ThreadSafeMessenger(const ThreadSafeMessenger&) = delete;
  ThreadSafeMessenger(ThreadSafeMessenger&&) noexcept = delete;
  ThreadSafeMessenger& operator=(const ThreadSafeMessenger&) = delete;
  ThreadSafeMessenger& operator=(ThreadSafeMessenger&&) noexcept = delete;

  /**
   * @brief This function accepts a number to increment the counter by and handles all of the message assembly and send off
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param counter the number of elements the function has processed since last update progress call
   */
  void updateProgress(usize counter);

  /**
   * @brief sets the message to be displayed next to the percentage
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param progressMessage display message
   */
  void setProgressMessage(std::string&& progressMessage);

  /**
   * @brief sets the elapsed time between progress updates when updateProgress is called [default is 1000 ms = 1 sec]
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param milliseconds the number of milliseconds between updates
   */
  void setUpdateDelay(usize milliseconds);

  /**
   * @brief This sets the count at which the updateProgress function SHOULD be called for that id, ie this is a vessel to carry the value to calling function
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param count this is a helper variable that will determine how often update progress is called
   */
  void setProgressIncrement(usize count);

  /**
   * @brief
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param totalElements the total number of elements that should be counted (this is used in percentage calculation)
   */
  void setTotalElements(usize totalElements);

  /**
   * @briefThis gets the count at which the updateProgress function SHOULD be called for, ie this is a vessel to carry the value to calling function
   * @return m_ProgressIncrement this is a helper variable that will determine how often update progress is called
   */
  usize getProgressIncrement() const;

  /**
   * @brief gets the total number of elements that should be counted (this is used in percentage calculation)
   * @return m_TotalElements
   */
  usize getTotalElements() const;

  /**
   * @brief Sends a status message using the underlying Message class
   * @param message The message to send
   */
  void sendStatusMessage(const std::string& message);

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

/**
 * @class ThreadSafeTaskMessenger
 * @brief This is a task messenger for running multiple arrays in parallel that need progress updates.
 * the thought behind this is based on the ParallelTaskAlgorithm intended use case
 */
class COMPLEX_EXPORT ThreadSafeTaskMessenger
{
  using ArrayValues = std::array<usize, 3>; // {progress counter, progress increment, total elements}

  /*!!!!The C++11 standard guarantees that const method access to containers is safe from different threads (ie, both use const methods)!!!!*/

public:
  ThreadSafeTaskMessenger(const IFilter::MessageHandler& messageHandler, std::string&& progressMessage = "Executing...", usize milliDelay = 1000);
  ~ThreadSafeTaskMessenger() = default;

  ThreadSafeTaskMessenger(const ThreadSafeTaskMessenger&) = delete;
  ThreadSafeTaskMessenger(ThreadSafeTaskMessenger&&) noexcept = delete;
  ThreadSafeTaskMessenger& operator=(const ThreadSafeTaskMessenger&) = delete;
  ThreadSafeTaskMessenger& operator=(ThreadSafeTaskMessenger&&) noexcept = delete;

  /**
   * @brief This function accepts a number to increment the counter by and handles all of the message assembly and send off
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param counter how much to increment the counter of that array by
   * @param id the id of the DataObject being tracked
   * @return bool will return false if object doesn't exist in structure
   */
  bool updateProgress(usize counter, uint64 id);

  /**
   * @brief This sets the message to be shared ahead of the array percentages
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param progressMessage
   */
  void setProgressMessage(std::string&& progressMessage);

  /**
   * @brief This sets how long the time is between updates when update progress is called
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param milliseconds
   */
  void setUpdateDelay(usize milliseconds);

  /**
   * @brief This stores the required values to update an objects progress, progress increment is implicitly calculated to
   * 1% of the total in this function, so no need to set unless you want a custom increment
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param id the id of the DataObject being tracked
   * @param totalElements this is the total number of elements (that will be counted) to be parsed in parallel
   * @param arrayName This is the name that will be displayed next to the percentage in the message
   * @return this will return false if the object already exists in the structure
   */
  bool addArray(uint64 id, usize totalElements, std::string&& arrayName);

  /**
   * @brief This sets the count at which the updateProgress function SHOULD be called for that id, ie this is a vessel to carry the value to calling function
   * (this can be overrode in the actual calling function, but the get method call is the sole intended use case)
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param progressIncrement this is the count at which updateProgress function should be called
   * @param id this is the id associated with the data object being operated on
   * @return false if id isn't in the known ids
   */
  bool setProgressIncrement(usize progressIncrement, uint64 id);

  /**
   * @brief This sets the total elements for the array attached to the provided id
   * [!!Utilizes Mutex Locks!!] be wary calling this excessively in parallel as it can slow the program down
   * @param totalElements the total count that the object aims to reach (used in percentage calculation)
   * @param id this is the id associated with the data object being operated on
   * @return false if id isn't in the known ids
   */
  bool setTotalElements(usize totalElements, uint64 id);

  /**
   * @brief This returns the count at which updateProgress SHOULD be called for that id
   * @param id this is the id associated with the data object being operated on
   * @return 0 if id wasn't found, else returns total elements stored
   */
  usize getProgressIncrement(uint64 id) const;

  /**
   * @brief This returns the total elements for that id
   * @param id this is the id associated with the data object being operated on
   * @return 0 if id wasn't found, else returns total elements stored
   */
  usize getTotalElements(uint64 id) const;

  // may need to implement a function in the future for non dataObjects to check that the id isn't already in the map

private:
  const IFilter::MessageHandler& m_MessageHandler;
  std::string m_ProgressMessage = "Executing...";
  usize m_MilliDelay = 1000; // Default = 1 second

  std::map<uint64, std::pair<ArrayValues, std::string>> m_Data = {};

  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};
} // namespace complex
