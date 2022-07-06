#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RobustAutomaticThresholdInputValues inputValues;

  inputValues.InputArrayPath = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  inputValues.GradientMagnitudeArrayPath = filterArgs.value<DataPath>(k_GradientMagnitudeArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  return RobustAutomaticThreshold(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT RobustAutomaticThresholdInputValues
{
  DataPath InputArrayPath;
  DataPath GradientMagnitudeArrayPath;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT RobustAutomaticThreshold
{
public:
  RobustAutomaticThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RobustAutomaticThresholdInputValues* inputValues);
  ~RobustAutomaticThreshold() noexcept;

  RobustAutomaticThreshold(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold(RobustAutomaticThreshold&&) noexcept = delete;
  RobustAutomaticThreshold& operator=(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold& operator=(RobustAutomaticThreshold&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RobustAutomaticThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
