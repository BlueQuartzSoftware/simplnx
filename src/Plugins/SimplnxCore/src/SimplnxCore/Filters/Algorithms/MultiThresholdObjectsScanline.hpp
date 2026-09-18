#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct MultiThresholdObjectsInputValues;

/**
 * @class MultiThresholdObjectsScanline
 * @brief Evaluates a threshold tree through bounded bulk I/O.
 *
 * Each pass evaluates 65,536 tuples. Peak scratch depends on chunk size, tree
 * depth, and the widest input tuple instead of total tuple count.
 *
 * @see MultiThresholdObjectsDirect
 */
class SIMPLNXCORE_EXPORT MultiThresholdObjectsScanline
{
public:
  /**
   * @brief Creates a scanline threshold evaluator.
   * @param dataStructure Provides threshold inputs and the output mask.
   * @param mesgHandler Is unused by scanline evaluation.
   * @param shouldCancel Stops before later input or output chunks when true.
   * @param inputValues Specifies validated threshold settings. The caller must keep
   * this object alive for the evaluator lifetime.
   */
  MultiThresholdObjectsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const MultiThresholdObjectsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning evaluator.
   */
  ~MultiThresholdObjectsScanline() noexcept;

  MultiThresholdObjectsScanline(const MultiThresholdObjectsScanline&) = delete;
  MultiThresholdObjectsScanline(MultiThresholdObjectsScanline&&) noexcept = delete;
  MultiThresholdObjectsScanline& operator=(const MultiThresholdObjectsScanline&) = delete;
  MultiThresholdObjectsScanline& operator=(MultiThresholdObjectsScanline&&) noexcept = delete;

  /**
   * @brief Evaluates and writes the mask one tuple chunk at a time.
   * @return Input or output bulk-I/O error, or success after cancellation.
   *
   * Cancellation can retain complete output chunks written before the current chunk.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace nx::core
