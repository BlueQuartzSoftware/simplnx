#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct NeighborOrientationCorrelationInputValues
 * @brief Holds all user-supplied parameters for the NeighborOrientationCorrelation algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT NeighborOrientationCorrelationInputValues
{
  DataPath ImageGeomPath;                                        ///< Path to the ImageGeom that defines the voxel grid dimensions
  float32 MinConfidence;                                         ///< Cells with confidence index below this value are candidates for replacement
  float32 MisorientationTolerance;                               ///< Angular tolerance (degrees) for comparing neighbor orientations
  int32 Level;                                                   ///< Minimum neighbor agreement count required to replace a cell (cleanup level)
  DataPath ConfidenceIndexArrayPath;                             ///< Path to the float32 confidence index array
  DataPath CellPhasesArrayPath;                                  ///< Path to the int32 cell phases array
  DataPath QuatsArrayPath;                                       ///< Path to the float32 quaternion array (4 components per tuple)
  DataPath CrystalStructuresArrayPath;                           ///< Path to the uint32 crystal structures ensemble array
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths; ///< Data arrays excluded from the neighbor-copy transfer step
};

/**
 * @class NeighborOrientationCorrelation
 * @brief Corrects low-confidence EBSD voxels by replacing their cell data with
 * data from the most orientation-correlated face neighbor.
 *
 * The algorithm iterates through multiple "cleanup levels" (from 6 down to the
 * user-specified Level). At each level, every voxel whose confidence index is
 * below MinConfidence is examined. For that voxel, the 6 face neighbors are
 * compared pairwise: two neighbors "agree" if they share the same nonzero phase
 * and their misorientation is within MisorientationTolerance. Each neighbor
 * accumulates a similarity count (how many other neighbors agree with it). The
 * neighbor with the highest agreement is chosen as the replacement source.
 *
 * ## Z-Slice Buffering (Out-of-Core Optimization)
 *
 * To avoid random-access thrashing of out-of-core (OOC) compressed chunk stores,
 * the algorithm maintains a rolling window of 3 adjacent Z-slices for the
 * quaternion and phase arrays, plus 1 Z-slice for the confidence index. At each
 * Z-step, the window advances by swapping buffer slots and reading only the new
 * z+1 slice. All neighbor lookups then read from these local buffers instead of
 * the backing DataArray, eliminating repeated chunk decompressions.
 *
 * After identifying the best neighbor for every low-confidence voxel in a level,
 * all cell-level DataArrays (except ignored ones) are updated in parallel using
 * ParallelTaskAlgorithm, copying tuple data from each best neighbor.
 */
class ORIENTATIONANALYSIS_EXPORT NeighborOrientationCorrelation
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  NeighborOrientationCorrelation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 NeighborOrientationCorrelationInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~NeighborOrientationCorrelation() noexcept;

  NeighborOrientationCorrelation(const NeighborOrientationCorrelation&) = delete;
  NeighborOrientationCorrelation(NeighborOrientationCorrelation&&) noexcept = delete;
  NeighborOrientationCorrelation& operator=(const NeighborOrientationCorrelation&) = delete;
  NeighborOrientationCorrelation& operator=(NeighborOrientationCorrelation&&) noexcept = delete;

  /**
   * @brief Executes the neighbor orientation correlation algorithm.
   * @return Result<> indicating success or any errors encountered during execution
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const NeighborOrientationCorrelationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
