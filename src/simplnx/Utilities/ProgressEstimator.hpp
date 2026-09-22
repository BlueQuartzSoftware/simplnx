#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <chrono>
#include <string>

namespace nx::core
{
/**
 * @class ProgressEstimator
 * @brief Estimates the time remaining in one phase of work.
 *
 * Any progress message can carry an estimate. ThrottledMessageHandler owns one of these and passes
 * its result to every progress send, so a throttled loop needs no extra code. A filter that sends
 * progress directly can own one too: restart it when the phase begins, then pass estimate() as the
 * detail argument of IFilter::MessageHandler::sendProgressCount() or sendProgressPercent().
 *
 * The rate is an exponentially weighted average of the rate between successive calls. A plain
 * average over the whole phase is stable but answers slowly when the work changes speed; the rate
 * since the previous message answers immediately but swings wildly when one batch stalls, which is
 * why users stop trusting an estimate. Blending the two keeps most of both: k_SmoothingFactor sets
 * how much of each new sample is taken, so a lasting change in speed is followed within a few
 * messages while a single slow batch moves the estimate only slightly.
 *
 * estimate() returns an empty string rather than a misleading number when the work has not run long
 * enough to support one, so a short operation simply shows no estimate.
 *
 * This class reads the clock only inside estimate(). Call it where a message is actually sent,
 * never once per loop iteration. It is not thread-safe; it belongs to whatever already serializes
 * the reporting.
 */
class SIMPLNX_EXPORT ProgressEstimator
{
public:
  /**
   * @brief Elapsed time a phase must run before an estimate is offered. A shorter phase produces no
   * estimate, because an early sample is dominated by start-up cost and reads as noise.
   */
  static constexpr std::chrono::seconds k_MinimumElapsed{3};

  /**
   * @brief Weight given to each new rate sample. A larger value follows a change in speed sooner
   * and admits more noise.
   */
  static constexpr float64 k_SmoothingFactor = 0.25;

  /**
   * @brief Constructs an estimator whose phase starts now.
   */
  ProgressEstimator();

  /**
   * @brief Restarts the phase at the current time and discards the measured rate. Call when the
   * work being measured changes, so one phase's rate never estimates another's.
   */
  void restart();

  /**
   * @brief Restarts the phase at a caller-supplied time. Intended for tests, so they can produce a
   * deterministic elapsed time without sleeping.
   * @param startTime Time the phase began
   */
  void restart(std::chrono::steady_clock::time_point startTime);

  /**
   * @brief Estimates the time remaining in this phase and records a rate sample.
   * @param current Work completed so far
   * @param max Total work in this phase
   * @return Text such as "about 2 m 30 s remaining", or an empty string when no honest estimate is
   * available: a zero denominator, no completed work, work already complete, a phase shorter than
   * k_MinimumElapsed, or no measured rate yet.
   */
  [[nodiscard]] std::string estimate(usize current, usize max);

private:
  std::chrono::steady_clock::time_point m_Start;
  std::chrono::steady_clock::time_point m_LastSample;
  usize m_LastProgress = 0;
  float64 m_UnitsPerSecond = 0.0;
};
} // namespace nx::core
