#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeSurfaceAreaToVolumeInputValues
 * @brief Holds all user-configurable parameters for the ComputeSurfaceAreaToVolume algorithm.
 *
 * These values are extracted from the filter's parameter map and passed through
 * the dispatcher to whichever algorithm variant (Direct or Scanline) is selected.
 */
struct SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeInputValues
{
  DataPath FeatureIdsArrayPath;             ///< Path to the cell-level Int32 FeatureIds array.
  DataPath NumCellsArrayPath;               ///< Path to the feature-level Int32 NumCells array (voxel count per feature).
  DataPath SurfaceAreaVolumeRatioArrayName; ///< Path where the output Float32 SA/V ratio array will be stored.
  bool CalculateSphericity;                 ///< When true, also computes the sphericity for each feature.
  DataPath SphericityArrayName;             ///< Path where the output Float32 sphericity array will be stored (only used when CalculateSphericity is true).
  DataPath InputImageGeometry;              ///< Path to the ImageGeom that defines grid dimensions and voxel spacing.
};

/**
 * @class ComputeSurfaceAreaToVolume
 * @brief Dispatcher that selects between the in-core (Direct) and out-of-core (Scanline)
 * surface-area-to-volume ratio algorithms at runtime.
 *
 * This class does not contain any algorithm logic itself. Its operator()() inspects
 * the storage backing of the FeatureIds array and calls
 * `DispatchAlgorithm<ComputeSurfaceAreaToVolumeDirect, ComputeSurfaceAreaToVolumeScanline>(...)`.
 *
 * **Algorithm overview**: For each voxel in the image geometry, the algorithm examines
 * its 6 face neighbors. Whenever a neighbor belongs to a different feature, the shared
 * face area is added to the current feature's surface area accumulator. After processing
 * all voxels, the surface area is divided by the feature volume (numCells * voxelVolume)
 * to produce the SA/V ratio. Optionally, sphericity is also computed from the same
 * surface area and volume values.
 *
 * **Dispatch rules** (see AlgorithmDispatch.hpp):
 * - If all input arrays are in-memory, the Direct variant is selected.
 * - If any input array uses OOC storage, the Scanline variant is selected.
 * - Test-override flags can force either path.
 *
 * @see ComputeSurfaceAreaToVolumeDirect, ComputeSurfaceAreaToVolumeScanline, DispatchAlgorithm
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolume
{
public:
  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure The DataStructure containing all arrays and geometries.
   * @param mesgHandler Handler for sending progress/info messages back to the UI.
   * @param shouldCancel Atomic flag checked periodically to support user cancellation.
   * @param inputValues User-configured parameters for the algorithm.
   */
  ComputeSurfaceAreaToVolume(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeSurfaceAreaToVolumeInputValues* inputValues);
  ~ComputeSurfaceAreaToVolume() noexcept;

  ComputeSurfaceAreaToVolume(const ComputeSurfaceAreaToVolume&) = delete;
  ComputeSurfaceAreaToVolume(ComputeSurfaceAreaToVolume&&) noexcept = delete;
  ComputeSurfaceAreaToVolume& operator=(const ComputeSurfaceAreaToVolume&) = delete;
  ComputeSurfaceAreaToVolume& operator=(ComputeSurfaceAreaToVolume&&) noexcept = delete;

  /**
   * @brief Dispatches to the appropriate algorithm variant (Direct or Scanline)
   * based on whether the FeatureIds array uses out-of-core storage.
   * @return Result<> indicating success or any errors encountered.
   */
  Result<> operator()();

  /**
   * @brief Returns a reference to the cancellation flag.
   * @return Const reference to the atomic cancellation boolean.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                                       ///< Reference to the DataStructure containing all data.
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr; ///< User-configured algorithm parameters.
  const std::atomic_bool& m_ShouldCancel;                               ///< Atomic flag for cooperative cancellation.
  const IFilter::MessageHandler& m_MessageHandler;                      ///< Handler for progress and informational messages.
};

} // namespace nx::core
