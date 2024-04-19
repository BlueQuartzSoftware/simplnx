#pragma once

#include "simplnx/simplnx_export.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <chrono>
#include <string>

namespace nx::core
{

inline std::string timestamp()
{
  std::time_t t = std::time(nullptr);
  return fmt::format("[{:%Y:%m:%d %H:%M:%S}]", fmt::localtime(t));
}

/**
 * @brief Converts a millisecond count to Hours:Minutes:Seconds type of display. This assumes
 * that the time would not be longer than 99 hours, 99 minutes, 99 seconds.
 * @param millis Input millisecond count
 * @return std::string in the form of HH:MM:SS
 */
inline std::string ConvertMillisToHrsMinSecs(unsigned long long int millis)
{
  constexpr unsigned long long int k_HoursConversion = 3600000;
  constexpr unsigned long long int k_MinutesConversion = 60000;
  unsigned long long int hours = millis / k_HoursConversion;
  unsigned long long minutes = (millis % k_HoursConversion) / k_MinutesConversion;
  unsigned long long seconds = ((millis % k_HoursConversion) % k_MinutesConversion) / 1000;
  return fmt::format("{:0>2}:{:0>2}:{:0>2}", hours, minutes, seconds);
}

/**
 * @brief A stopwatch class for measuring durations with high precision.
 *
 * The StopWatch class utilizes the std::chrono library to measure time intervals.
 * It can be started and stopped to measure specific durations, and the elapsed time
 * can be printed to an output stream in a formatted manner.
 */
class SIMPLNX_EXPORT StopWatch
{
public:
  StopWatch() = default;
  ~StopWatch() = default;

  StopWatch(const StopWatch&) = default;            // Copy Constructor Not Implemented
  StopWatch(StopWatch&&) = default;                 // Move Constructor Not Implemented
  StopWatch& operator=(const StopWatch&) = default; // Copy Assignment Not Implemented
  StopWatch& operator=(StopWatch&&) = default;      // Move Assignment Not Implemented

  /**
   * @brief Start the stopwatch.
   *
   * Captures the current time as the start point of the duration measurement.
   * This function must be called before calling stop().
   */
  void start();

  /**
   * @brief Stop the stopwatch.
   *
   * Captures the current time as the end point of the duration measurement.
   * This function should be called after start().
   */
  void stop();

  /**
   * @brief Print the elapsed time to a given output stream.
   *
   * Calculates the elapsed time between the start and stop points and formats it as
   * "Hours:Minutes:Seconds.Milliseconds". This formatted string is then written to
   * the provided std::ostream object.
   *
   * @param os The output stream to which the elapsed time will be written.
   */
  void print(std::ostream& os) const;

  /**
   * @brief Returns a generated string of the elapsed time
   * @return
   */
  std::string toString() const;

private:
  std::chrono::steady_clock::time_point start_time; ///< The start time point of the measurement.
  std::chrono::steady_clock::time_point end_time;   ///< The end time point of the measurement.
};

} // namespace nx::core
