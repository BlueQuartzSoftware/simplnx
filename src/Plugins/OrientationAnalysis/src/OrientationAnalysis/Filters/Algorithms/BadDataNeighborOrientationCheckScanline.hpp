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
 * @brief Out-of-core (Scanline) algorithm for the bad-data neighbor orientation check.
 *
 * This algorithm is selected by the dispatcher when any of the quaternion, mask, or
 * phase arrays are backed by chunked (OOC) storage. It avoids random-access patterns
 * that would cause chunk thrashing by using a 3-slice rolling window over the Z axis.
 *
 * **Strategy -- rolling window with on-the-fly neighbor counting**:
 *
 * For each "level" (starting at 6, decrementing to NumberOfNeighbors), the algorithm
 * repeatedly scans the entire volume until no more voxels are flipped:
 *
 *   1. Load Z-slices 0 (current) and 1 (next) via bulk copyIntoBuffer() for
 *      quaternions, phases, and the mask.
 *   2. For each Z-slice, iterate over every (x, y) in the slice:
 *      - If the voxel is already good, skip it.
 *      - Otherwise, check its 6 face-neighbors (4 in-plane from curSlice, 1 from
 *        prevSlice, 1 from nextSlice) for good voxels with matching orientation.
 *      - If the count of matching neighbors >= currentLevel, flip the mask to true
 *        in the local buffer.
 *   3. If any voxels were flipped in the current Z-slice, write the updated mask
 *      back to the OOC store via copyFromBuffer().
 *   4. Shift the rolling window: prev <- cur, cur <- next, and load the next
 *      Z-slice into the "next" buffer.
 *
 * **Key difference from the Worklist variant**: Neighbor counts are recomputed from
 * scratch for every bad voxel on every pass, because maintaining a persistent global
 * neighborCount array would require random-access OOC writes whenever a voxel flips.
 * The rolling-window scan approach trades more computation for strictly sequential I/O.
 *
 * **Memory footprint**: O(3 * sliceSize) for the rolling window buffers -- three
 * Z-slices of quaternions, phases, and mask data. No global per-voxel arrays.
 *
 * @see BadDataNeighborOrientationCheckWorklist for the in-core worklist variant.
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckScanline
{
public:
  /**
   * @brief Constructs the OOC scanline neighbor orientation check algorithm.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Message handler for progress/info messages.
   * @param shouldCancel Atomic cancellation flag checked once per Z-slice.
   * @param inputValues Pointer to the shared parameter struct; must outlive this object.
   */
  BadDataNeighborOrientationCheckScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          const BadDataNeighborOrientationCheckInputValues* inputValues);
  ~BadDataNeighborOrientationCheckScanline() noexcept;

  BadDataNeighborOrientationCheckScanline(const BadDataNeighborOrientationCheckScanline&) = delete;
  BadDataNeighborOrientationCheckScanline(BadDataNeighborOrientationCheckScanline&&) noexcept = delete;
  BadDataNeighborOrientationCheckScanline& operator=(const BadDataNeighborOrientationCheckScanline&) = delete;
  BadDataNeighborOrientationCheckScanline& operator=(BadDataNeighborOrientationCheckScanline&&) noexcept = delete;

  /**
   * @brief Flips bad voxels to good using Z-slice rolling window bulk I/O with
   *        on-the-fly neighbor count recomputation.
   * @return Result<> with any errors (e.g., invalid mask path).
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                            ///< Reference to the live DataStructure.
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr; ///< Borrowed pointer to input parameters.
  const std::atomic_bool& m_ShouldCancel;                                    ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                           ///< Message handler for user-facing messages.
};

} // namespace nx::core
