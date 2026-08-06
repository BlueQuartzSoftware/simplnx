#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <mutex>
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct NeighborOrientationCorrelationInputValues
 * @brief Identifies neighbor orientation-correlation inputs.
 *
 * MisorientationTolerance is in degrees.
 */
struct ORIENTATIONANALYSIS_EXPORT NeighborOrientationCorrelationInputValues
{
  DataPath ImageGeomPath;
  float32 MinConfidence = 0.0f;
  float32 MisorientationTolerance = 0.0f;
  int32 Level = 0;
  DataPath ConfidenceIndexArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class NeighborOrientationCorrelation
 * @brief Corrects low-confidence EBSD cells from correlated neighbors.
 *
 * Cells below MinConfidence use the best agreeing face neighbor. The rolling
 * window keeps quaternion, phase, and confidence reads local. The replacement
 * transfer excludes selected arrays.
 */
class ORIENTATIONANALYSIS_EXPORT NeighborOrientationCorrelation
{
public:
  /**
   * @brief Initializes neighbor orientation correlation.
   * @param dataStructure Provides selected arrays and the geometry.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies correction settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  NeighborOrientationCorrelation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 NeighborOrientationCorrelationInputValues* inputValues);

  /**
   * @brief Destroys the neighbor-correlation executor.
   */
  ~NeighborOrientationCorrelation() noexcept;

  NeighborOrientationCorrelation(const NeighborOrientationCorrelation&) = delete;
  NeighborOrientationCorrelation(NeighborOrientationCorrelation&&) noexcept = delete;
  NeighborOrientationCorrelation& operator=(const NeighborOrientationCorrelation&) = delete;
  NeighborOrientationCorrelation& operator=(NeighborOrientationCorrelation&&) noexcept = delete;

  /**
   * @brief Corrects low-confidence cells.
   * @return Success, or an error for an invalid Phase or Laue index, correlation, or replacement transfer.
   */
  Result<> operator()();

  /**
   * @brief Thread-safe progress update. Safe to call from the parallel per-array workers.
   * @param message Fully rendered progress text for one array
   */
  void sendThreadSafeProgressMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const NeighborOrientationCorrelationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
