#include "ProgressEstimator.hpp"

#include "simplnx/Utilities/TimeUtilities.hpp"

#include <fmt/format.h>

namespace nx::core
{
ProgressEstimator::ProgressEstimator()
{
  restart();
}

void ProgressEstimator::restart()
{
  restart(std::chrono::steady_clock::now());
}

void ProgressEstimator::restart(std::chrono::steady_clock::time_point startTime)
{
  m_Start = startTime;
  m_LastSample = startTime;
  m_LastProgress = 0;
  m_UnitsPerSecond = 0.0;
}

std::string ProgressEstimator::estimate(usize current, usize max)
{
  if(max == 0 || current == 0 || current >= max)
  {
    return {};
  }

  const auto now = std::chrono::steady_clock::now();

  // Sample the rate since the previous call, then fold it into the running rate. The sample is
  // taken even before an estimate is offered, so the first estimate already has a measured rate.
  const auto sampleMillis = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastSample).count();
  if(current > m_LastProgress && sampleMillis > 0)
  {
    const float64 sampleRate = static_cast<float64>(current - m_LastProgress) / (static_cast<float64>(sampleMillis) / 1000.0);
    // The first sample has nothing to blend with, so it becomes the rate outright.
    m_UnitsPerSecond = (m_UnitsPerSecond <= 0.0) ? sampleRate : (k_SmoothingFactor * sampleRate) + ((1.0 - k_SmoothingFactor) * m_UnitsPerSecond);
    m_LastSample = now;
    m_LastProgress = current;
  }

  if(m_UnitsPerSecond <= 0.0)
  {
    return {};
  }
  if(std::chrono::duration_cast<std::chrono::seconds>(now - m_Start) < k_MinimumElapsed)
  {
    return {};
  }

  const float64 remainingSeconds = static_cast<float64>(max - current) / m_UnitsPerSecond;
  const auto remainingMillis = static_cast<unsigned long long int>(remainingSeconds * 1000.0);
  return fmt::format("about {} remaining", ConvertMillisToFriendlyDuration(remainingMillis));
}
} // namespace nx::core
