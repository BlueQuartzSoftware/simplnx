#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceAreaToVolumeInputValues;

/**
 * @class ComputeSurfaceAreaToVolumeDirect
 * @brief In-core (direct memory access) algorithm for computing per-feature surface area,
 * volume, surface-area-to-volume ratio, and optional sphericity.
 *
 * This is the traditional algorithm that uses operator[] to read FeatureIds directly
 * through the DataStore abstraction. It iterates all voxels in Z-Y-X order and, for
 * each voxel, checks its 6 face neighbors using pre-computed index offsets. When a
 * neighbor belongs to a different feature, the area of the shared face (determined by
 * the voxel spacing along each axis) is added to the current feature's surface-area
 * accumulator. After the full-volume scan, the ratio and optional sphericity are
 * computed per feature.
 *
 * **When this variant is selected**: DispatchAlgorithm selects this class when all
 * input arrays are backed by contiguous in-memory DataStore. With in-memory data,
 * the neighbor lookups via flat-index offsets are simple pointer arithmetic.
 *
 * **Why a separate OOC variant exists**: The 6-neighbor lookup pattern accesses
 * elements at offsets of +/-1, +/-dimX, and +/-(dimX*dimY) from the current voxel.
 * When FeatureIds is stored out-of-core in chunked format, these scattered accesses
 * cause chunk thrashing. The Scanline variant reads entire Z-slices sequentially
 * with copyIntoBuffer() to avoid this.
 *
 * **Output details**: The per-feature surface-area accumulation uses a local
 * std::vector<float32> (not the output DataStore) because multiple voxels
 * contribute to the same feature's surface area. After the scan, the SA/V ratio
 * is computed and written to the output array. Sphericity (if requested) uses
 * the formula: sphericity = (pi^(1/3) * (6V)^(2/3)) / SA.
 *
 * @see ComputeSurfaceAreaToVolumeScanline for the OOC-optimized variant.
 * @see ComputeSurfaceAreaToVolume for the dispatcher.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeDirect
{
public:
  /**
   * @brief Constructs the in-core SA/V ratio calculator.
   * @param dataStructure The DataStructure containing all arrays and the ImageGeom.
   * @param mesgHandler Handler for progress/info messages.
   * @param shouldCancel Atomic flag for cooperative cancellation.
   * @param inputValues Algorithm parameters (geometry path, array paths, sphericity flag).
   */
  ComputeSurfaceAreaToVolumeDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   const ComputeSurfaceAreaToVolumeInputValues* inputValues);
  ~ComputeSurfaceAreaToVolumeDirect() noexcept;

  ComputeSurfaceAreaToVolumeDirect(const ComputeSurfaceAreaToVolumeDirect&) = delete;
  ComputeSurfaceAreaToVolumeDirect(ComputeSurfaceAreaToVolumeDirect&&) noexcept = delete;
  ComputeSurfaceAreaToVolumeDirect& operator=(const ComputeSurfaceAreaToVolumeDirect&) = delete;
  ComputeSurfaceAreaToVolumeDirect& operator=(ComputeSurfaceAreaToVolumeDirect&&) noexcept = delete;

  /**
   * @brief Executes the in-core surface-area-to-volume ratio algorithm.
   *
   * Iterates every voxel in Z-Y-X order, accumulates per-feature surface area
   * from face-neighbor comparisons, then divides by feature volume. Optionally
   * computes sphericity.
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
