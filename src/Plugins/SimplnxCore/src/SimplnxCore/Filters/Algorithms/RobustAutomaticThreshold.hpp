#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

/**
 * @struct RobustAutomaticThresholdInputValues
 * @brief Stores scalar, gradient, and output-mask selections.
 */
struct SIMPLNXCORE_EXPORT RobustAutomaticThresholdInputValues
{
  DataObjectNameParameter::ValueType CreatedMaskName;
  ArraySelectionParameter::ValueType GradientArrayPath;
  ArraySelectionParameter::ValueType InputArrayPath;
};

/**
 * @class RobustAutomaticThreshold
 * @brief Creates a mask from a gradient-weighted scalar threshold.
 *
 * The threshold is sum(value times gradient) divided by sum(gradient). A second
 * pass marks values greater than or equal to that threshold. Both passes use
 * 65,536-value bulk buffers. Sequential accumulation preserves the established
 * float rounding order without resident array-sized scratch.
 *
 * The implementation does not guard a zero gradient sum. Division can then
 * produce a nonfinite threshold. A NaN threshold makes every comparison false.
 * In-core and out-of-core telemetry labels use this same bounded implementation.
 */
class SIMPLNXCORE_EXPORT RobustAutomaticThreshold
{
public:
  /**
   * @brief Initializes the automatic-threshold algorithm.
   * @param dataStructure Contains scalar, gradient, and mask arrays.
   * @param mesgHandler Preserves the common algorithm constructor signature.
   * @param shouldCancel Signals cancellation between bulk chunks.
   * @param inputValues Identifies input and output arrays.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  RobustAutomaticThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RobustAutomaticThresholdInputValues* inputValues);
  /**
   * @brief Destroys the automatic-threshold algorithm.
   */
  ~RobustAutomaticThreshold() noexcept;

  RobustAutomaticThreshold(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold(RobustAutomaticThreshold&&) noexcept = delete;
  RobustAutomaticThreshold& operator=(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold& operator=(RobustAutomaticThreshold&&) noexcept = delete;

  /**
   * @brief Computes the weighted threshold and writes the mask.
   * @return Source, gradient, or output bulk-I/O result.
   * @pre Input and gradient arrays are scalar and have equal tuple counts.
   *
   * Cancellation during accumulation leaves the output unchanged. Cancellation
   * during mask creation returns success with completed mask chunks in place.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RobustAutomaticThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
