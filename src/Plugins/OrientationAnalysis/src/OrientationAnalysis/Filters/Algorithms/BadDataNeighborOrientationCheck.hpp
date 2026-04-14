#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct BadDataNeighborOrientationCheckInputValues
 * @brief Holds user-facing parameters for the Bad Data Neighbor Orientation Check algorithm.
 *
 * This filter rehabilitates "bad" voxels (mask == false) by checking whether enough
 * of their 6 face-neighbors are "good" and have a similar crystallographic orientation.
 * If a bad voxel has at least NumberOfNeighbors good neighbors whose misorientation is
 * within MisorientationTolerance, its mask is flipped to true.
 */
struct ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckInputValues
{
  float32 MisorientationTolerance;     ///< Maximum allowed misorientation (degrees) between a bad voxel and a good neighbor for the neighbor to count as "matching".
  int32 NumberOfNeighbors;             ///< Minimum number of matching good face-neighbors required to flip a bad voxel to good. The algorithm iterates from 6 down to this value.
  DataPath ImageGeomPath;              ///< Path to the ImageGeometry that defines the voxel grid dimensions.
  DataPath QuatsArrayPath;             ///< Path to the Float32 quaternion array (4 components per tuple) for each voxel.
  DataPath MaskArrayPath;              ///< Path to the boolean or uint8 mask array (true = good, false = bad). Modified in-place.
  DataPath CellPhasesArrayPath;        ///< Path to the Int32 array of per-voxel phase IDs.
  DataPath CrystalStructuresArrayPath; ///< Path to the UInt32 ensemble array mapping phase ID -> EbsdLib crystal structure enum.
};

/**
 * @class BadDataNeighborOrientationCheck
 * @brief Dispatcher that selects between in-core and out-of-core neighbor orientation check algorithms.
 *
 * This class serves as the entry point called by BadDataNeighborOrientationCheckFilter::executeImpl().
 * It inspects the backing storage of the quaternion, mask, and phase arrays using
 * DispatchAlgorithm<BadDataNeighborOrientationCheckWorklist, BadDataNeighborOrientationCheckScanline>:
 *
 * - **In-core (BadDataNeighborOrientationCheckWorklist)**: Precomputes a per-voxel neighbor
 *   count, then uses a deque-based worklist for O(flipped) propagation. Efficient when all
 *   data is in contiguous RAM because random-access updates to any voxel are O(1).
 *
 * - **Out-of-core (BadDataNeighborOrientationCheckScanline)**: Uses a 3-slice rolling window
 *   (prev/cur/next Z-slices) loaded via copyIntoBuffer()/copyFromBuffer(). Neighbor counts
 *   are recomputed on-the-fly for each bad voxel instead of being stored in a global array,
 *   because maintaining a global count array would require random-access OOC writes.
 *
 * @see BadDataNeighborOrientationCheckWorklist, BadDataNeighborOrientationCheckScanline, DispatchAlgorithm
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheck
{
public:
  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure The DataStructure containing all input and output arrays.
   * @param mesgHandler Message handler for progress/info messages.
   * @param shouldCancel Atomic flag checked periodically to support user cancellation.
   * @param inputValues Pointer to the parameter struct; must outlive this object.
   */
  BadDataNeighborOrientationCheck(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  BadDataNeighborOrientationCheckInputValues* inputValues);
  ~BadDataNeighborOrientationCheck() noexcept;

  BadDataNeighborOrientationCheck(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck(BadDataNeighborOrientationCheck&&) noexcept = delete;
  BadDataNeighborOrientationCheck& operator=(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck& operator=(BadDataNeighborOrientationCheck&&) noexcept = delete;

  /**
   * @brief Dispatches to BadDataNeighborOrientationCheckWorklist (in-core) or
   *        BadDataNeighborOrientationCheckScanline (OOC) based on storage type.
   * @return Result<> with any errors.
   */
  Result<> operator()();

  /**
   * @brief Returns the cancellation flag reference.
   * @return const reference to the atomic cancellation flag.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                                            ///< Reference to the live DataStructure.
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr; ///< Borrowed pointer to input parameters.
  const std::atomic_bool& m_ShouldCancel;                                    ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                           ///< Message handler for user-facing messages.
};

} // namespace nx::core
