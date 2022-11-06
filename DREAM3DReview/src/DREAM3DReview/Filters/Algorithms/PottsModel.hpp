#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  PottsModelInputValues inputValues;

  inputValues.Iterations = filterArgs.value<int32>(k_Iterations_Key);
  inputValues.Temperature = filterArgs.value<float64>(k_Temperature_Key);
  inputValues.PeriodicBoundaries = filterArgs.value<bool>(k_PeriodicBoundaries_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return PottsModel(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT PottsModelInputValues
{
  int32 Iterations;
  float64 Temperature;
  bool PeriodicBoundaries;
  bool UseMask;
  DataPath FeatureIdsArrayPath;
  DataPath MaskArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT PottsModel
{
public:
  PottsModel(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PottsModelInputValues* inputValues);
  ~PottsModel() noexcept;

  PottsModel(const PottsModel&) = delete;
  PottsModel(PottsModel&&) noexcept = delete;
  PottsModel& operator=(const PottsModel&) = delete;
  PottsModel& operator=(PottsModel&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PottsModelInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
