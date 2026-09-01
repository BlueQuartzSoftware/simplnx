#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct BadDataNeighborOrientationCheckInputValues;

/**
 * @class BadDataNeighborOrientationCheckWorklist
 * @brief Rehabilitates bad in-memory voxels with a worklist.
 *
 * The dispatcher selects this class only for the in-memory scenario. It initializes one neighbor
 * count per voxel, then uses a deque to propagate flips from six neighbors to the requested count.
 * The count array uses four bytes per voxel and the deque grows with eligible voxels. Cancellation
 * is checked during the initial scan and before each worklist pop. Cancellation returns success
 * with already applied mask changes.
 *
 * The worklist reads and writes arbitrary voxel positions. It is unsuitable for OOC stores because
 * random chunk access can thrash. This sequential implementation gives no concurrent DataArray or
 * DataStore access guarantee.
 *
 * @see BadDataNeighborOrientationCheckScanline for the OOC-optimized variant.
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckWorklist
{
public:
  /**
   * @brief Initializes the worklist bad-data executor.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  BadDataNeighborOrientationCheckWorklist(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          const BadDataNeighborOrientationCheckInputValues* inputValues);

  /**
   * @brief Destroys the worklist bad-data executor.
   */
  ~BadDataNeighborOrientationCheckWorklist() noexcept;

  BadDataNeighborOrientationCheckWorklist(const BadDataNeighborOrientationCheckWorklist&) = delete;
  BadDataNeighborOrientationCheckWorklist(BadDataNeighborOrientationCheckWorklist&&) noexcept = delete;
  BadDataNeighborOrientationCheckWorklist& operator=(const BadDataNeighborOrientationCheckWorklist&) = delete;
  BadDataNeighborOrientationCheckWorklist& operator=(BadDataNeighborOrientationCheckWorklist&&) noexcept = delete;

  /**
   * @brief Rehabilitates eligible bad voxels.
   * @pre Cell phase IDs are nonnegative and within the crystal-structure array.
   * @return An error if the mask type or crystal structures are invalid.
   *
   * Cancellation returns success with already applied mask changes.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
