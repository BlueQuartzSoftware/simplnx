#pragma once

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <chrono>
#include <string>

namespace nx::core
{

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

} // namespace nx::core
