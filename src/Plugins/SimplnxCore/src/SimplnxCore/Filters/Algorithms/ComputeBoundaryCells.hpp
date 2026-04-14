#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeBoundaryCellsInputValues
 * @brief Holds all user-configurable parameters for the ComputeBoundaryCells algorithm.
 *
 * These values are extracted from the filter's parameter map and passed through
 * the dispatcher to whichever algorithm variant (Direct or Scanline) is selected.
 */
struct SIMPLNXCORE_EXPORT ComputeBoundaryCellsInputValues
{
  bool IgnoreFeatureZero;          ///< When true, neighbors with FeatureId == 0 are not counted as boundary faces.
  bool IncludeVolumeBoundary;      ///< When true, cells on the edge of the image geometry volume contribute extra boundary counts.
  DataPath ImageGeometryPath;      ///< Path to the ImageGeom that defines grid dimensions.
  DataPath FeatureIdsArrayPath;    ///< Path to the cell-level Int32 FeatureIds array.
  DataPath BoundaryCellsArrayName; ///< Path where the output Int8 boundary-cell-count array will be stored.
};

/**
 * @class ComputeBoundaryCells
 * @brief Dispatcher that selects between the in-core (Direct) and out-of-core (Scanline)
 * boundary-cell counting algorithms at runtime.
 *
 * This class does not contain any algorithm logic itself. Its operator()() inspects
 * the storage backing of the FeatureIds array and calls
 * `DispatchAlgorithm<ComputeBoundaryCellsDirect, ComputeBoundaryCellsScanline>(...)`.
 *
 * **Algorithm overview**: For each voxel in the image geometry, count how many of its
 * 6 face-connected neighbors belong to a different feature. The result is an Int8 array
 * where each cell stores its boundary face count (0-6).
 *
 * **Dispatch rules** (see AlgorithmDispatch.hpp):
 * - If all input arrays are backed by in-memory DataStore, the Direct variant is used.
 * - If any input array uses out-of-core (chunked/Zarr) storage, the Scanline variant
 *   is used to avoid random-access chunk thrashing.
 * - Global test-override flags (ForceOocAlgorithm, ForceInCoreAlgorithm) can override
 *   the automatic detection for unit testing purposes.
 *
 * @see ComputeBoundaryCellsDirect, ComputeBoundaryCellsScanline, DispatchAlgorithm
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCells
{
public:
  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure The DataStructure containing all arrays and geometries.
   * @param mesgHandler Handler for sending progress/info messages back to the UI.
   * @param shouldCancel Atomic flag checked periodically to support user cancellation.
   * @param inputValues User-configured parameters for the algorithm.
   */
  ComputeBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCells() noexcept;

  ComputeBoundaryCells(const ComputeBoundaryCells&) = delete;
  ComputeBoundaryCells(ComputeBoundaryCells&&) noexcept = delete;
  ComputeBoundaryCells& operator=(const ComputeBoundaryCells&) = delete;
  ComputeBoundaryCells& operator=(ComputeBoundaryCells&&) noexcept = delete;

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
  DataStructure& m_DataStructure;                                 ///< Reference to the DataStructure containing all data.
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr; ///< User-configured algorithm parameters.
  const std::atomic_bool& m_ShouldCancel;                         ///< Atomic flag for cooperative cancellation.
  const IFilter::MessageHandler& m_MessageHandler;                ///< Handler for progress and informational messages.
};

} // namespace nx::core
