#include "TimeUtilities.hpp"

#include <chrono>
#include <iostream>

#include <fmt/chrono.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
void StopWatch::start()
{
  start_time = std::chrono::steady_clock::now();
}

// -----------------------------------------------------------------------------
void StopWatch::stop()
{
  end_time = std::chrono::steady_clock::now();
}

// -----------------------------------------------------------------------------
void StopWatch::print(std::ostream& os) const
{
  os << toString();
}

// -----------------------------------------------------------------------------
std::string StopWatch::toString() const
{
  auto elapsed = end_time - start_time;
  return fmt::format("{:%H:%M:%S}.{:03}", elapsed, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() % 1000);
}
