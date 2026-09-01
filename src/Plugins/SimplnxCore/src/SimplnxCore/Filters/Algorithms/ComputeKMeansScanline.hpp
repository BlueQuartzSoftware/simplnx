#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeKMeansInputValues;

/**
 * @class ComputeKMeansScanline
 * @brief Computes K-Means clusters with bounded bulk I/O.
 *
 * The path streams input values, masks, and assignments in fixed pages. Means
 * remain resident because their size is proportional to cluster count and
 * component count. One input pass recomputes all centroid components. Each
 * accumulator still receives values in increasing tuple order.
 */
class SIMPLNXCORE_EXPORT ComputeKMeansScanline
{
public:
  /**
   * @brief Initializes scanline K-Means clustering.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives iteration messages.
   * @param shouldCancel Signals cancellation between bounded operations.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ComputeKMeansScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeKMeansInputValues* inputValues);
  ~ComputeKMeansScanline() noexcept;

  ComputeKMeansScanline(const ComputeKMeansScanline&) = delete;
  ComputeKMeansScanline(ComputeKMeansScanline&&) noexcept = delete;
  ComputeKMeansScanline& operator=(const ComputeKMeansScanline&) = delete;
  ComputeKMeansScanline& operator=(ComputeKMeansScanline&&) noexcept = delete;

  /**
   * @brief Executes K-Means with bounded bulk transfers.
   * @return Success, or a mask, shape, overflow, or bulk-transfer error.
   *
   * Cancellation returns success. Completed assignment pages and centroids from
   * earlier phases remain in the output arrays.
   */
  Result<> operator()();

  void updateProgress(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeKMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
