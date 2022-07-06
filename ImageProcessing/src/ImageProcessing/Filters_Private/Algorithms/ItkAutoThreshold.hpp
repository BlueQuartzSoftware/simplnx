#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkAutoThresholdInputValues inputValues;

  inputValues.Method = filterArgs.value<ChoicesParameter::ValueType>(k_Method_Key);
  inputValues.Slice = filterArgs.value<bool>(k_Slice_Key);
  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkAutoThreshold(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkAutoThresholdInputValues
{
  ChoicesParameter::ValueType Method;
  bool Slice;
  bool SaveAsNewArray;
  DataPath SelectedCellArrayPath;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkAutoThreshold
{
public:
  ItkAutoThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkAutoThresholdInputValues* inputValues);
  ~ItkAutoThreshold() noexcept;

  ItkAutoThreshold(const ItkAutoThreshold&) = delete;
  ItkAutoThreshold(ItkAutoThreshold&&) noexcept = delete;
  ItkAutoThreshold& operator=(const ItkAutoThreshold&) = delete;
  ItkAutoThreshold& operator=(ItkAutoThreshold&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkAutoThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
