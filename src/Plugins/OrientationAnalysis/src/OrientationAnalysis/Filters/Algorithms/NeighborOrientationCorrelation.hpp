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

struct ORIENTATIONANALYSIS_EXPORT NeighborOrientationCorrelationInputValues
{
  DataPath ImageGeomPath;
  float32 MinConfidence;
  float32 MisorientationTolerance;
  int32 Level;
  DataPath ConfidenceIndexArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT NeighborOrientationCorrelation
{
public:
  NeighborOrientationCorrelation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 NeighborOrientationCorrelationInputValues* inputValues);
  ~NeighborOrientationCorrelation() noexcept;

  NeighborOrientationCorrelation(const NeighborOrientationCorrelation&) = delete;
  NeighborOrientationCorrelation(NeighborOrientationCorrelation&&) noexcept = delete;
  NeighborOrientationCorrelation& operator=(const NeighborOrientationCorrelation&) = delete;
  NeighborOrientationCorrelation& operator=(NeighborOrientationCorrelation&&) noexcept = delete;

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
