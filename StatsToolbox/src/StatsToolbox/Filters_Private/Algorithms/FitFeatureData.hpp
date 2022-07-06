#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FitFeatureDataInputValues inputValues;

  inputValues.DistributionType = filterArgs.value<ChoicesParameter::ValueType>(k_DistributionType_Key);
  inputValues.RemoveBiasedFeatures = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  inputValues.SelectedFeatureArrayPath = filterArgs.value<DataPath>(k_SelectedFeatureArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.BiasedFeaturesArrayPath = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  inputValues.NewEnsembleArrayArray = filterArgs.value<DataPath>(k_NewEnsembleArrayArray_Key);

  return FitFeatureData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FitFeatureDataInputValues
{
  ChoicesParameter::ValueType DistributionType;
  bool RemoveBiasedFeatures;
  DataPath SelectedFeatureArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath BiasedFeaturesArrayPath;
  DataPath NewEnsembleArrayArray;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FitFeatureData
{
public:
  FitFeatureData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FitFeatureDataInputValues* inputValues);
  ~FitFeatureData() noexcept;

  FitFeatureData(const FitFeatureData&) = delete;
  FitFeatureData(FitFeatureData&&) noexcept = delete;
  FitFeatureData& operator=(const FitFeatureData&) = delete;
  FitFeatureData& operator=(FitFeatureData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FitFeatureDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
