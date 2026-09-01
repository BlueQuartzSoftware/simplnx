#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct MultiThresholdObjectsInputValues;

/**
 * @class MultiThresholdObjectsDirect
 * @brief Evaluates a threshold tree through direct array access.
 *
 * Each active tree level owns one cell-count result vector. This storage avoids
 * disk I/O overhead when all participating arrays are resident.
 *
 * @see MultiThresholdObjectsScanline
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjectsDirect
{
public:
  /**
   * @brief Creates a direct threshold evaluator.
   * @param dataStructure Provides threshold inputs and the output mask.
   * @param mesgHandler Is unused by direct evaluation.
   * @param shouldCancel Stops later evaluation or output values when true.
   * @param inputValues Specifies validated threshold settings. The caller must keep
   * this object alive for the evaluator lifetime.
   */
  MultiThresholdObjectsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const MultiThresholdObjectsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning evaluator.
   */
  ~MultiThresholdObjectsDirect() noexcept;

  MultiThresholdObjectsDirect(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect(MultiThresholdObjectsDirect&&) noexcept = delete;
  MultiThresholdObjectsDirect& operator=(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect& operator=(MultiThresholdObjectsDirect&&) noexcept = delete;

  /**
   * @brief Evaluates the complete tree and writes the resident mask.
   * @return Success after completion or cancellation.
   *
   * Cancellation during tree evaluation leaves the output unchanged. Cancellation
   * during final value writes can retain a partially written output.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace nx::core
