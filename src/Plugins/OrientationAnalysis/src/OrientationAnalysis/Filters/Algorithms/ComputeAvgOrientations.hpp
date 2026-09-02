#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <vector>

namespace nx::core
{

/**
 * @brief The ComputeAvgOrientationsInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeAvgOrientationsInputValues
{
  DataPath cellFeatureIdsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath cellQuatsArrayPath;
  DataPath crystalStructuresArrayPath;

  bool useRodriguesAverage;
  bool useVonMisesAverage;
  bool useWatsonAverage;
  DataPath avgQuatsArrayPath;
  DataPath avgEulerAnglesArrayPath;

  DataPath VMFQuatsArrayPath;
  DataPath VMFEulerAnglesArrayPath;
  DataPath VMFKappaArrayPath;

  DataPath WatsonQuatsArrayPath;
  DataPath WatsonEulerAnglesArrayPath;
  DataPath WatsonKappaArrayPath;

  uint32 RandomSeed = 43514;
  int32 NumEMIterations = 5;
  int32 NumIterations = 10;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT ComputeAvgOrientations
{
public:
  ComputeAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeAvgOrientationsInputValues* inputValues);
  ~ComputeAvgOrientations() noexcept;

  ComputeAvgOrientations(const ComputeAvgOrientations&) = delete;            // Copy Constructor Not Implemented
  ComputeAvgOrientations(ComputeAvgOrientations&&) = delete;                 // Move Constructor Not Implemented
  ComputeAvgOrientations& operator=(const ComputeAvgOrientations&) = delete; // Copy Assignment Not Implemented
  ComputeAvgOrientations& operator=(ComputeAvgOrientations&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

  void sendThreadSafeProgressMessage(usize counter);

  bool getCancel() const
  {
    return m_ShouldCancel;
  }

protected:
private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeAvgOrientationsInputValues* m_InputValues = nullptr;

  Result<> computeRodriguesAverage();
  Result<> computeVmfWatsonAverage();

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
  size_t m_NumberOfFeatures = 0;
};

} // namespace nx::core
