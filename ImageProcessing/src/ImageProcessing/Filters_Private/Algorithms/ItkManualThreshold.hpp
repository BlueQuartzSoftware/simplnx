#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkManualThresholdInputValues inputValues;

  inputValues.ManualParameter = filterArgs.value<int32>(k_ManualParameter_Key);
  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkManualThreshold(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkManualThresholdInputValues
{
  int32 ManualParameter;
  bool SaveAsNewArray;
  DataPath SelectedCellArrayPath;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkManualThreshold
{
public:
  ItkManualThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkManualThresholdInputValues* inputValues);
  ~ItkManualThreshold() noexcept;

  ItkManualThreshold(const ItkManualThreshold&) = delete;
  ItkManualThreshold(ItkManualThreshold&&) noexcept = delete;
  ItkManualThreshold& operator=(const ItkManualThreshold&) = delete;
  ItkManualThreshold& operator=(ItkManualThreshold&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkManualThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
