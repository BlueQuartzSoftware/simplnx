#pragma once

#include <chrono>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <string>

namespace complex
{

/**
 * @brief Converts a millisecond count to Hours:Minutes:Seconds type of display. This assumes
 * that the time would not be longer than 99 hours, 99 minutes, 99 seconds.
 * @param millis Input millisecond count
 * @return std::string in the form of HH:MM:SS
 */
inline std::string ConvertMillisToHrsMinSecs(unsigned long long int millis)
{
  std::chrono::duration<unsigned long long int, std::milli> millisDuration(millis);
  auto hours = std::chrono::duration_cast<std::chrono::hours>(millisDuration);
  auto intMinutes = std::chrono::duration_cast<std::chrono::minutes>(millisDuration);
  auto intSeconds = std::chrono::duration_cast<std::chrono::seconds>(millisDuration);
  std::string str = fmt::format("{:0>2}:{:0>2}:{:0>2}", hours.count(), intMinutes.count(), intSeconds.count());
  return str;
}

} // namespace complex
