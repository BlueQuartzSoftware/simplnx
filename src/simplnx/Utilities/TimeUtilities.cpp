#include "TimeUtilities.hpp"

#include <chrono>
#include <iostream>

#include <fmt/chrono.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
void StopWatch::start()
{
  m_StartTime = std::chrono::steady_clock::now();
}

// -----------------------------------------------------------------------------
void StopWatch::stop()
{
  m_EndTime = std::chrono::steady_clock::now();
}

// -----------------------------------------------------------------------------
void StopWatch::print(std::ostream& os) const
{
  os << toString();
}

// -----------------------------------------------------------------------------
std::string StopWatch::toString() const
{
  auto elapsed = m_EndTime - m_StartTime;
  return fmt::format("{:%H:%M:%S}", elapsed, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() % 1000);
}
