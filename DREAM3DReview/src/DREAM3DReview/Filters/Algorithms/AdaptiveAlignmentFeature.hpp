#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AdaptiveAlignmentFeatureInputValues inputValues;

  inputValues.GlobalCorrection = filterArgs.value<ChoicesParameter::ValueType>(k_GlobalCorrection_Key);
  inputValues.ImageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  inputValues.ShiftX = filterArgs.value<float32>(k_ShiftX_Key);
  inputValues.ShiftY = filterArgs.value<float32>(k_ShiftY_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

  return AdaptiveAlignmentFeature(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT AdaptiveAlignmentFeatureInputValues
{
  ChoicesParameter::ValueType GlobalCorrection;
  DataPath ImageDataArrayPath;
  float32 ShiftX;
  float32 ShiftY;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
  DataPath GoodVoxelsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT AdaptiveAlignmentFeature
{
public:
  AdaptiveAlignmentFeature(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AdaptiveAlignmentFeatureInputValues* inputValues);
  ~AdaptiveAlignmentFeature() noexcept;

  AdaptiveAlignmentFeature(const AdaptiveAlignmentFeature&) = delete;
  AdaptiveAlignmentFeature(AdaptiveAlignmentFeature&&) noexcept = delete;
  AdaptiveAlignmentFeature& operator=(const AdaptiveAlignmentFeature&) = delete;
  AdaptiveAlignmentFeature& operator=(AdaptiveAlignmentFeature&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AdaptiveAlignmentFeatureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
