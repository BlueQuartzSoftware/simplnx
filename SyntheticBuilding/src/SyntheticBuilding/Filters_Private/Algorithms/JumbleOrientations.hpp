#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  JumbleOrientationsInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CellEulerAnglesArrayName = filterArgs.value<DataPath>(k_CellEulerAnglesArrayName_Key);
  inputValues.AvgQuatsArrayName = filterArgs.value<DataPath>(k_AvgQuatsArrayName_Key);

  return JumbleOrientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT JumbleOrientationsInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CellEulerAnglesArrayName;
  DataPath AvgQuatsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT JumbleOrientations
{
public:
  JumbleOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, JumbleOrientationsInputValues* inputValues);
  ~JumbleOrientations() noexcept;

  JumbleOrientations(const JumbleOrientations&) = delete;
  JumbleOrientations(JumbleOrientations&&) noexcept = delete;
  JumbleOrientations& operator=(const JumbleOrientations&) = delete;
  JumbleOrientations& operator=(JumbleOrientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const JumbleOrientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
