#pragma once

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
  const unsigned long long int hours = millis / (1000 * 60 * 60);
  const unsigned long long intMinutes = (millis % (1000 * 60 * 60)) / (1000 * 60);
  const unsigned long long intSeconds = ((millis % (1000 * 60 * 60)) % (1000 * 60)) / 1000;
  std::string str = fmt::format("{:0>2}:{:0>2}:{:0>2}", hours, intMinutes, intSeconds);
  return str;
}

} // namespace complex
