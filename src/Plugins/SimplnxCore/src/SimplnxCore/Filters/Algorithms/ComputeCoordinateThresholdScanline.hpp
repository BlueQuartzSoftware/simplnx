#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

struct ComputeCoordinateThresholdInputValues;

/**
 * @class ComputeCoordinateThresholdScanline
 * @brief Computes ImageGeom masks with bounded buffers and bulk DataStore writes.
 *
 * The executor uses a fixed 65,536-tuple buffer. This avoids per-cell disk access and keeps scratch
 * memory independent of image size.
 */
class SIMPLNXCORE_EXPORT ComputeCoordinateThresholdScanline
{
public:
  /**
   * @brief Initializes the scanline ImageGeom mask algorithm.
   * @param dataStructure Contains the ImageGeom and output mask.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects bounds and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeCoordinateThresholdScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     const ComputeCoordinateThresholdInputValues* inputValues);
  /**
   * @brief Destroys the scanline ImageGeom mask algorithm.
   */
  ~ComputeCoordinateThresholdScanline() noexcept;

  ComputeCoordinateThresholdScanline(const ComputeCoordinateThresholdScanline&) = delete;
  ComputeCoordinateThresholdScanline(ComputeCoordinateThresholdScanline&&) noexcept = delete;
  ComputeCoordinateThresholdScanline& operator=(const ComputeCoordinateThresholdScanline&) = delete;
  ComputeCoordinateThresholdScanline& operator=(ComputeCoordinateThresholdScanline&&) noexcept = delete;

  /**
   * @brief Creates the ImageGeom mask with bounded bulk writes.
   * @return Success, or an output bulk-I/O error.
   *
   * When a chunk checkpoint observes cancellation, the method returns success. Data written before
   * that checkpoint remains in the mask. Later cells are not written.
   * A bulk-I/O error can leave completed mask chunks.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeCoordinateThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
