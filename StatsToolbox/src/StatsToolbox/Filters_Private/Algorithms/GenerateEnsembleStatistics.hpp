#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateEnsembleStatisticsInputValues inputValues;

  inputValues.PhaseTypeArray = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_PhaseTypeArray_Key);
  inputValues.SizeCorrelationResolution = filterArgs.value<float32>(k_SizeCorrelationResolution_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.CalculateMorphologicalStats = filterArgs.value<bool>(k_CalculateMorphologicalStats_Key);
  inputValues.SizeDistributionFitType = filterArgs.value<ChoicesParameter::ValueType>(k_SizeDistributionFitType_Key);
  inputValues.BiasedFeaturesArrayPath = filterArgs.value<DataPath>(k_BiasedFeaturesArrayPath_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.AspectRatioDistributionFitType = filterArgs.value<ChoicesParameter::ValueType>(k_AspectRatioDistributionFitType_Key);
  inputValues.AspectRatiosArrayPath = filterArgs.value<DataPath>(k_AspectRatiosArrayPath_Key);
  inputValues.Omega3DistributionFitType = filterArgs.value<ChoicesParameter::ValueType>(k_Omega3DistributionFitType_Key);
  inputValues.Omega3sArrayPath = filterArgs.value<DataPath>(k_Omega3sArrayPath_Key);
  inputValues.NeighborhoodDistributionFitType = filterArgs.value<ChoicesParameter::ValueType>(k_NeighborhoodDistributionFitType_Key);
  inputValues.NeighborhoodsArrayPath = filterArgs.value<DataPath>(k_NeighborhoodsArrayPath_Key);
  inputValues.AxisEulerAnglesArrayPath = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayPath_Key);
  inputValues.CalculateCrystallographicStats = filterArgs.value<bool>(k_CalculateCrystallographicStats_Key);
  inputValues.SurfaceFeaturesArrayPath = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  inputValues.VolumesArrayPath = filterArgs.value<DataPath>(k_VolumesArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.SharedSurfaceAreaListArrayPath = filterArgs.value<DataPath>(k_SharedSurfaceAreaListArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.PhaseTypesArrayName = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  inputValues.StatisticsArrayName = filterArgs.value<DataPath>(k_StatisticsArrayName_Key);
  inputValues.IncludeRadialDistFunc = filterArgs.value<bool>(k_IncludeRadialDistFunc_Key);
  inputValues.RDFArrayPath = filterArgs.value<DataPath>(k_RDFArrayPath_Key);
  inputValues.MaxMinRDFArrayPath = filterArgs.value<DataPath>(k_MaxMinRDFArrayPath_Key);

  return GenerateEnsembleStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT GenerateEnsembleStatisticsInputValues
{
  <<<NOT_IMPLEMENTED>>> PhaseTypeArray;
  /*[x]*/ float32 SizeCorrelationResolution;
  DataPath FeaturePhasesArrayPath;
  DataPath NeighborListArrayPath;
  bool CalculateMorphologicalStats;
  ChoicesParameter::ValueType SizeDistributionFitType;
  DataPath BiasedFeaturesArrayPath;
  DataPath EquivalentDiametersArrayPath;
  ChoicesParameter::ValueType AspectRatioDistributionFitType;
  DataPath AspectRatiosArrayPath;
  ChoicesParameter::ValueType Omega3DistributionFitType;
  DataPath Omega3sArrayPath;
  ChoicesParameter::ValueType NeighborhoodDistributionFitType;
  DataPath NeighborhoodsArrayPath;
  DataPath AxisEulerAnglesArrayPath;
  bool CalculateCrystallographicStats;
  DataPath SurfaceFeaturesArrayPath;
  DataPath VolumesArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath SharedSurfaceAreaListArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath PhaseTypesArrayName;
  DataPath StatisticsArrayName;
  bool IncludeRadialDistFunc;
  DataPath RDFArrayPath;
  DataPath MaxMinRDFArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT GenerateEnsembleStatistics
{
public:
  GenerateEnsembleStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateEnsembleStatisticsInputValues* inputValues);
  ~GenerateEnsembleStatistics() noexcept;

  GenerateEnsembleStatistics(const GenerateEnsembleStatistics&) = delete;
  GenerateEnsembleStatistics(GenerateEnsembleStatistics&&) noexcept = delete;
  GenerateEnsembleStatistics& operator=(const GenerateEnsembleStatistics&) = delete;
  GenerateEnsembleStatistics& operator=(GenerateEnsembleStatistics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateEnsembleStatisticsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
