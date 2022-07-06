#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeatureClusteringInputValues inputValues;

  inputValues.NumberOfBins = filterArgs.value<int32>(k_NumberOfBins_Key);
  inputValues.PhaseNumber = filterArgs.value<int32>(k_PhaseNumber_Key);
  inputValues.RemoveBiasedFeatures = filterArgs.value<bool>(k_RemoveBiasedFeatures_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.BiasedFeaturesArrayPath = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.ClusteringListArrayName = filterArgs.value<DataPath>(k_ClusteringListArrayName_Key);
  inputValues.NewEnsembleArrayArrayName = filterArgs.value<DataPath>(k_NewEnsembleArrayArrayName_Key);
  inputValues.MaxMinArrayName = filterArgs.value<DataPath>(k_MaxMinArrayName_Key);

  return FindFeatureClustering(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindFeatureClusteringInputValues
{
  int32 NumberOfBins;
  int32 PhaseNumber;
  bool RemoveBiasedFeatures;
  DataPath EquivalentDiametersArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CentroidsArrayPath;
  DataPath BiasedFeaturesArrayPath;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath ClusteringListArrayName;
  DataPath NewEnsembleArrayArrayName;
  DataPath MaxMinArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindFeatureClustering
{
public:
  FindFeatureClustering(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeatureClusteringInputValues* inputValues);
  ~FindFeatureClustering() noexcept;

  FindFeatureClustering(const FindFeatureClustering&) = delete;
  FindFeatureClustering(FindFeatureClustering&&) noexcept = delete;
  FindFeatureClustering& operator=(const FindFeatureClustering&) = delete;
  FindFeatureClustering& operator=(FindFeatureClustering&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureClusteringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
