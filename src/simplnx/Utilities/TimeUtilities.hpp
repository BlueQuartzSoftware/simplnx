#pragma once

#include "simplnx/simplnx_export.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <string>

namespace nx::core
{

inline std::string timestamp()
{
  // Waiting on (apple)clang support for P0355R7
  // Migrate to std::chrono::current_zone and zoned_time then
  const auto utc_now = std::chrono::system_clock::now();
  const auto utc_time_t = std::chrono::system_clock::to_time_t(utc_now);
  std::tm tm_local = {};
#ifdef _MSC_VER
  localtime_s(&tm_local, &utc_time_t);
#else
  localtime_r(&utc_time_t, &tm_local);
#endif
  return fmt::format("[{:%Y:%m:%d %H:%M:%S}]", tm_local);
}

/**
 * @brief Converts a millisecond count to a short human-readable duration, such as "45 s",
 * "2 m 30 s" or "1 h 12 m". Seconds are dropped beyond an hour, where they carry no useful
 * precision.
 * @param millis Input millisecond count
 * @return The rendered duration
 */
inline std::string ConvertMillisToFriendlyDuration(unsigned long long int millis)
{
  const unsigned long long int totalSeconds = millis / 1000;
  if(totalSeconds < 60)
  {
    return fmt::format("{} s", totalSeconds);
  }
  const unsigned long long int totalMinutes = totalSeconds / 60;
  if(totalMinutes < 60)
  {
    const unsigned long long int seconds = totalSeconds % 60;
    return seconds == 0 ? fmt::format("{} m", totalMinutes) : fmt::format("{} m {} s", totalMinutes, seconds);
  }
  const unsigned long long int hours = totalMinutes / 60;
  const unsigned long long int minutes = totalMinutes % 60;
  return minutes == 0 ? fmt::format("{} h", hours) : fmt::format("{} h {} m", hours, minutes);
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
