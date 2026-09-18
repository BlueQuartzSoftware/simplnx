#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct BadDataNeighborOrientationCheckInputValues
 * @brief Identifies Bad Data Neighbor Orientation Check inputs.
 *
 * A bad voxel becomes good when enough good face neighbors have a matching
 * crystallographic orientation. MisorientationTolerance is in degrees. The
 * algorithm changes MaskArrayPath in place.
 */
struct ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckInputValues
{
  float32 MisorientationTolerance;
  int32 NumberOfNeighbors;
  DataPath ImageGeomPath;
  DataPath QuatsArrayPath;
  DataPath MaskArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class BadDataNeighborOrientationCheck
 * @brief Dispatches Bad Data Neighbor Orientation Check execution.
 *
 * The in-memory worklist retains one count per voxel and propagates changes
 * through a deque. The OOC scanline variant uses three local slices and
 * recomputes counts to avoid random OOC writes.
 *
 * @see BadDataNeighborOrientationCheckWorklist
 * @see BadDataNeighborOrientationCheckScanline
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheck
{
public:
  /**
   * @brief Initializes the Bad Data Neighbor Orientation Check dispatcher.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  BadDataNeighborOrientationCheck(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  BadDataNeighborOrientationCheckInputValues* inputValues);
  /**
   * @brief Destroys the Bad Data Neighbor Orientation Check dispatcher.
   */
  ~BadDataNeighborOrientationCheck() noexcept;

  BadDataNeighborOrientationCheck(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck(BadDataNeighborOrientationCheck&&) noexcept = delete;
  BadDataNeighborOrientationCheck& operator=(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck& operator=(BadDataNeighborOrientationCheck&&) noexcept = delete;

  /**
   * @brief Dispatches to the storage-appropriate executor.
   * @return Result from the selected executor.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
