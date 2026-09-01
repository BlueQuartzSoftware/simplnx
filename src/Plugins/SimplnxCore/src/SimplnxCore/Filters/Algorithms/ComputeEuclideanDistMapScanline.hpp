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

struct ComputeEuclideanDistMapInputValues;

/**
 * @class ComputeEuclideanDistMapScanline
 * @brief Bounded-memory distance-map implementation for out-of-core ImageGeom data.
 *
 * Feature IDs and output maps use full Z-slice bulk transfers. Obstacle-free volumes use exact
 * forward/backward city-block sweeps. Non-positive Feature IDs use layer-synchronous propagation
 * so blocked cells remain non-traversable.
 *
 * Resident buffers are O(X*Y). Euclidean output uses one nearest-seed DataStore that follows the
 * active storage policy. It is disk-backed when that policy selects out-of-core storage.
 *
 * Current bulk-I/O Result values are not inspected. A storage failure can leave partial maps while
 * the method returns success.
 */
class SIMPLNXCORE_EXPORT ComputeEuclideanDistMapScanline
{
public:
  /**
   * @brief Initializes the scanline distance-map algorithm.
   * @param dataStructure Contains the ImageGeom, Feature IDs, and output maps.
   * @param messageHandler Preserves the common dispatcher constructor signature.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects map types and identifies required objects.
   * @pre inputValues is not null.
   * @pre dataStructure, shouldCancel, and inputValues outlive this executor.
   *
   * This implementation does not use messageHandler.
   */
  ComputeEuclideanDistMapScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                  const ComputeEuclideanDistMapInputValues* inputValues);
  /**
   * @brief Destroys the scanline distance-map algorithm.
   */
  ~ComputeEuclideanDistMapScanline() noexcept;

  ComputeEuclideanDistMapScanline(const ComputeEuclideanDistMapScanline&) = delete;
  ComputeEuclideanDistMapScanline(ComputeEuclideanDistMapScanline&&) noexcept = delete;
  ComputeEuclideanDistMapScanline& operator=(const ComputeEuclideanDistMapScanline&) = delete;
  ComputeEuclideanDistMapScanline& operator=(ComputeEuclideanDistMapScanline&&) noexcept = delete;

  /**
   * @brief Computes requested distance maps with bounded bulk transfers.
   * @return Success.
   *
   * When a checkpoint observes cancellation, the method returns success. Completed seed,
   * propagation, and conversion ranges remain in the output maps. Later ranges are not written.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeEuclideanDistMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace nx::core
