#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ComputeIPFColorsInputValues;

/**
 * @class ComputeIPFColorsScanline
 * @brief Computes IPF colors with bounded OOC chunks.
 *
 * The dispatcher normally selects this class when Euler, phase, mask, or color
 * data is OOC. It bulk-reads one 65,536-tuple page, calculates local colors,
 * and bulk-writes the page. The method runs sequentially. Cancellation returns
 * success with completed pages preserved.
 *
 * The current ensemble, input, mask, and output bulk-I/O Result values are not
 * inspected. Therefore, only phase-range errors are returned.
 *
 * @see ComputeIPFColorsDirect for the multi-threaded in-core variant.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsScanline
{
public:
  /**
   * @brief Initializes scanline IPF color computation.
   * @param dataStructure Provides the selected arrays.
   * @param msgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and color settings.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeIPFColorsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const ComputeIPFColorsInputValues* inputValues);

  /**
   * @brief Destroys the scanline IPF color executor.
   */
  ~ComputeIPFColorsScanline() noexcept;

  ComputeIPFColorsScanline(const ComputeIPFColorsScanline&) = delete;
  ComputeIPFColorsScanline(ComputeIPFColorsScanline&&) = delete;
  ComputeIPFColorsScanline& operator=(const ComputeIPFColorsScanline&) = delete;
  ComputeIPFColorsScanline& operator=(ComputeIPFColorsScanline&&) = delete;

  /**
   * @brief Computes IPF colors with bounded pages.
   * @pre Cell phase IDs are nonnegative.
   * @return Success, or error -48000 if a positive phase ID exceeds the crystal-
   *         structure array.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeIPFColorsInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
