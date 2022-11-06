#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MatchCrystallographyInputValues inputValues;

  inputValues.MaxIterations = filterArgs.value<int32>(k_MaxIterations_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.SurfaceFeaturesArrayPath = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.SharedSurfaceAreaListArrayPath = filterArgs.value<DataPath>(k_SharedSurfaceAreaListArrayPath_Key);
  inputValues.InputStatsArrayPath = filterArgs.value<DataPath>(k_InputStatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.PhaseTypesArrayPath = filterArgs.value<DataPath>(k_PhaseTypesArrayPath_Key);
  inputValues.NumFeaturesArrayPath = filterArgs.value<DataPath>(k_NumFeaturesArrayPath_Key);
  inputValues.CellEulerAnglesArrayName = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  inputValues.VolumesArrayName = filterArgs.value<DataPath>(k_VolumesArrayName_Key);
  inputValues.FeatureEulerAnglesArrayName = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayName_Key);
  inputValues.AvgQuatsArrayName = filterArgs.value<DataPath>(k_AvgQuatsArrayName_Key);

  return MatchCrystallography(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT MatchCrystallographyInputValues
{
  int32 MaxIterations;
  DataPath FeatureIdsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath SurfaceFeaturesArrayPath;
  DataPath NeighborListArrayPath;
  DataPath SharedSurfaceAreaListArrayPath;
  DataPath InputStatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath PhaseTypesArrayPath;
  DataPath NumFeaturesArrayPath;
  DataPath CellEulerAnglesArrayName;
  DataPath VolumesArrayName;
  DataPath FeatureEulerAnglesArrayName;
  DataPath AvgQuatsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT MatchCrystallography
{
public:
  MatchCrystallography(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MatchCrystallographyInputValues* inputValues);
  ~MatchCrystallography() noexcept;

  MatchCrystallography(const MatchCrystallography&) = delete;
  MatchCrystallography(MatchCrystallography&&) noexcept = delete;
  MatchCrystallography& operator=(const MatchCrystallography&) = delete;
  MatchCrystallography& operator=(MatchCrystallography&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MatchCrystallographyInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
