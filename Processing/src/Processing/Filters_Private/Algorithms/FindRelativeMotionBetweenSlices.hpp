#pragma once

#include "Processing/Processing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindRelativeMotionBetweenSlicesInputValues inputValues;

  inputValues.Plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.PSize1 = filterArgs.value<int32>(k_PSize1_Key);
  inputValues.PSize2 = filterArgs.value<int32>(k_PSize2_Key);
  inputValues.SSize1 = filterArgs.value<int32>(k_SSize1_Key);
  inputValues.SSize2 = filterArgs.value<int32>(k_SSize2_Key);
  inputValues.SliceStep = filterArgs.value<int32>(k_SliceStep_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.MotionDirectionArrayName = filterArgs.value<DataPath>(k_MotionDirectionArrayName_Key);

  return FindRelativeMotionBetweenSlices(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROCESSING_EXPORT FindRelativeMotionBetweenSlicesInputValues
{
  ChoicesParameter::ValueType Plane;
  int32 PSize1;
  int32 PSize2;
  int32 SSize1;
  int32 SSize2;
  int32 SliceStep;
  DataPath SelectedArrayPath;
  DataPath MotionDirectionArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROCESSING_EXPORT FindRelativeMotionBetweenSlices
{
public:
  FindRelativeMotionBetweenSlices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  FindRelativeMotionBetweenSlicesInputValues* inputValues);
  ~FindRelativeMotionBetweenSlices() noexcept;

  FindRelativeMotionBetweenSlices(const FindRelativeMotionBetweenSlices&) = delete;
  FindRelativeMotionBetweenSlices(FindRelativeMotionBetweenSlices&&) noexcept = delete;
  FindRelativeMotionBetweenSlices& operator=(const FindRelativeMotionBetweenSlices&) = delete;
  FindRelativeMotionBetweenSlices& operator=(FindRelativeMotionBetweenSlices&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindRelativeMotionBetweenSlicesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
