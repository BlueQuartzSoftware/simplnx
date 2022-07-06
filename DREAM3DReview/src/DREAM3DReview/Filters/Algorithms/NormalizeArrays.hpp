#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  NormalizeArraysInputValues inputValues;

  inputValues.NormalizeType = filterArgs.value<ChoicesParameter::ValueType>(k_NormalizeType_Key);
  inputValues.Postfix = filterArgs.value<StringParameter::ValueType>(k_Postfix_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.DefaultValue = filterArgs.value<float64>(k_DefaultValue_Key);
  inputValues.RangeMin = filterArgs.value<float64>(k_RangeMin_Key);
  inputValues.RangeMax = filterArgs.value<float64>(k_RangeMax_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return NormalizeArrays(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT NormalizeArraysInputValues
{
  ChoicesParameter::ValueType NormalizeType;
  StringParameter::ValueType Postfix;
  bool UseMask;
  float64 DefaultValue;
  float64 RangeMin;
  float64 RangeMax;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  DataPath MaskArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT NormalizeArrays
{
public:
  NormalizeArrays(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, NormalizeArraysInputValues* inputValues);
  ~NormalizeArrays() noexcept;

  NormalizeArrays(const NormalizeArrays&) = delete;
  NormalizeArrays(NormalizeArrays&&) noexcept = delete;
  NormalizeArrays& operator=(const NormalizeArrays&) = delete;
  NormalizeArrays& operator=(NormalizeArrays&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const NormalizeArraysInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
