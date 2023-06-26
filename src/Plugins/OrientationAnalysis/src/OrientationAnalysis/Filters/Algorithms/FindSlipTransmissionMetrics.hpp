#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"


/**
* This is example code to put in the Execute Method of the filter.
  FindSlipTransmissionMetricsInputValues inputValues;

  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.F1ListArrayName = filterArgs.value<DataPath>(k_F1ListArrayName_Key);
  inputValues.F1sptListArrayName = filterArgs.value<DataPath>(k_F1sptListArrayName_Key);
  inputValues.F7ListArrayName = filterArgs.value<DataPath>(k_F7ListArrayName_Key);
  inputValues.mPrimeListArrayName = filterArgs.value<DataPath>(k_mPrimeListArrayName_Key);

  return FindSlipTransmissionMetrics(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindSlipTransmissionMetricsInputValues
{
  DataPath NeighborListArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath F1ListArrayName;
  DataPath F1sptListArrayName;
  DataPath F7ListArrayName;
  DataPath mPrimeListArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindSlipTransmissionMetrics
{
public:
  FindSlipTransmissionMetrics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSlipTransmissionMetricsInputValues* inputValues);
  ~FindSlipTransmissionMetrics() noexcept;

  FindSlipTransmissionMetrics(const FindSlipTransmissionMetrics&) = delete;
  FindSlipTransmissionMetrics(FindSlipTransmissionMetrics&&) noexcept = delete;
  FindSlipTransmissionMetrics& operator=(const FindSlipTransmissionMetrics&) = delete;
  FindSlipTransmissionMetrics& operator=(FindSlipTransmissionMetrics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindSlipTransmissionMetricsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
