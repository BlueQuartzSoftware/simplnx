#pragma once

#include "complex/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <string>

namespace complex
{
inline std::string ConvertMillisToHrsMinSecs(unsigned long long int millis)
{
  unsigned long long int Hours = millis / (1000 * 60 * 60);
  unsigned long long intMinutes = (millis % (1000 * 60 * 60)) / (1000 * 60);
  unsigned long long intSeconds = ((millis % (1000 * 60 * 60)) % (1000 * 60)) / 1000;
  std::string str = fmt::format("{:0>2}:{:0>2}:{:0>2}", StringUtilities::number(Hours), StringUtilities::number(intMinutes), StringUtilities::number(intSeconds));
  return str;
}

} // namespace complex
