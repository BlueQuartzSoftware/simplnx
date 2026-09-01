#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct BadDataNeighborOrientationCheckInputValues;

/**
 * @class BadDataNeighborOrientationCheckScanline
 * @brief Rehabilitates bad voxels with scanline OOC I/O.
 *
 * The dispatcher selects this class when a required per-voxel array is OOC. Three Z-slice
 * buffers provide face-neighbor data through bulk reads. The algorithm recomputes neighbor counts
 * instead of maintaining a global count array that would require random OOC writes. Levels descend
 * from six to the requested count. Each level scans until no mask value changes. Cancellation is
 * checked before each Z slice and returns success with already applied mask changes.
 *
 * The rolling window bounds memory to three quaternion, phase, and mask slices. The bool-mask path
 * also needs one raw bool scratch slice. The algorithm runs sequentially and gives no concurrent
 * DataArray or DataStore access guarantee. Current slice bulk-I/O Result values are not inspected.
 *
 * @see BadDataNeighborOrientationCheckWorklist for the in-core worklist variant.
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckScanline
{
public:
  /**
   * @brief Initializes the scanline bad-data executor.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  BadDataNeighborOrientationCheckScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          const BadDataNeighborOrientationCheckInputValues* inputValues);

  /**
   * @brief Destroys the scanline bad-data executor.
   */
  ~BadDataNeighborOrientationCheckScanline() noexcept;

  BadDataNeighborOrientationCheckScanline(const BadDataNeighborOrientationCheckScanline&) = delete;
  BadDataNeighborOrientationCheckScanline(BadDataNeighborOrientationCheckScanline&&) noexcept = delete;
  BadDataNeighborOrientationCheckScanline& operator=(const BadDataNeighborOrientationCheckScanline&) = delete;
  BadDataNeighborOrientationCheckScanline& operator=(BadDataNeighborOrientationCheckScanline&&) noexcept = delete;

  /**
   * @brief Rehabilitates eligible bad voxels.
   * @pre Cell phase IDs are nonnegative and within the crystal-structure array.
   * @return An error if the mask type is unsupported or crystal structures are
   *         invalid.
   *
   * Cancellation returns success with already applied mask changes. Slice
   * bulk-I/O Result values are not inspected.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
