#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceAreaToVolumeInputValues;

/**
 * @class ComputeSurfaceAreaToVolumeScanline
 * @brief Out-of-core (OOC) optimized algorithm for computing surface-area-to-volume
 * ratio and optional sphericity using Z-slice sequential bulk I/O with a 3-slice
 * rolling window.
 *
 * **The problem this solves**: When the FeatureIds array is stored out-of-core in
 * chunked format, the Direct variant's operator[] access to check 6 face neighbors
 * triggers chunk thrashing. Each voxel requires reading up to 7 different locations
 * (itself + 6 neighbors), and the +/-Z neighbors are dimX*dimY elements apart in
 * flat index space, almost certainly spanning different chunks.
 *
 * **How the rolling window solves it**: This variant reads the FeatureIds array one
 * native Z-slice at a time using copyIntoBuffer(), maintaining three in-memory
 * buffers (prevSlice, curSlice, nextSlice). All neighbor lookups are performed on
 * these buffers:
 *   - X and Y neighbors: index arithmetic within curSlice (+/-1 and +/-dimX).
 *   - Z neighbors: same position in prevSlice (-Z) or nextSlice (+Z).
 *
 * After the voxel scan, the feature-level arrays (NumCells, SA/V ratio, sphericity)
 * are also accessed via local std::vector caches with bulk copyIntoBuffer/copyFromBuffer
 * calls, avoiding per-element OOC access.
 *
 * **Surface area accumulation**: Like the Direct variant, this uses a local
 * std::vector<float32> to accumulate per-feature surface area during the voxel scan.
 * Face areas are pre-computed from the image geometry spacing.
 *
 * **Memory overhead**: 3 input slice buffers (dimX * dimY * 4 bytes each), plus
 * local caches for NumCells, SA/V ratio, and sphericity arrays (numFeatures * 4
 * bytes each). For typical datasets, total overhead is a few MB.
 *
 * @see ComputeSurfaceAreaToVolumeDirect for the in-core variant.
 * @see ComputeSurfaceAreaToVolume for the dispatcher.
 * @see DispatchAlgorithm for the selection mechanism.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeScanline
{
public:
  /**
   * @brief Constructs the OOC-optimized SA/V ratio calculator.
   * @param dataStructure The DataStructure containing all arrays and the ImageGeom.
   * @param mesgHandler Handler for progress/info messages.
   * @param shouldCancel Atomic flag for cooperative cancellation.
   * @param inputValues Algorithm parameters (geometry path, array paths, sphericity flag).
   */
  ComputeSurfaceAreaToVolumeScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     const ComputeSurfaceAreaToVolumeInputValues* inputValues);
  ~ComputeSurfaceAreaToVolumeScanline() noexcept;

  ComputeSurfaceAreaToVolumeScanline(const ComputeSurfaceAreaToVolumeScanline&) = delete;
  ComputeSurfaceAreaToVolumeScanline(ComputeSurfaceAreaToVolumeScanline&&) noexcept = delete;
  ComputeSurfaceAreaToVolumeScanline& operator=(const ComputeSurfaceAreaToVolumeScanline&) = delete;
  ComputeSurfaceAreaToVolumeScanline& operator=(ComputeSurfaceAreaToVolumeScanline&&) noexcept = delete;

  /**
   * @brief Executes the OOC-optimized surface-area-to-volume ratio algorithm
   * using a 3-slice rolling window with copyIntoBuffer/copyFromBuffer bulk I/O.
   *
   * All feature-level computations (ratio, sphericity) also use local caches
   * with bulk read/write to avoid per-element OOC access.
   *
   * @return Result<> indicating success or errors.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                       ///< Reference to the DataStructure containing all data.
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr; ///< Algorithm parameters.
  const std::atomic_bool& m_ShouldCancel;                               ///< Cooperative cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                      ///< Progress message handler.
};

} // namespace nx::core
