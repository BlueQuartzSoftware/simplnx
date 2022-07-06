#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkImageMathInputValues inputValues;

  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.Operator = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  inputValues.Value = filterArgs.value<float64>(k_Value_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkImageMath(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkImageMathInputValues
{
  bool SaveAsNewArray;
  DataPath SelectedCellArrayPath;
  ChoicesParameter::ValueType Operator;
  float64 Value;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkImageMath
{
public:
  ItkImageMath(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkImageMathInputValues* inputValues);
  ~ItkImageMath() noexcept;

  ItkImageMath(const ItkImageMath&) = delete;
  ItkImageMath(ItkImageMath&&) noexcept = delete;
  ItkImageMath& operator=(const ItkImageMath&) = delete;
  ItkImageMath& operator=(ItkImageMath&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkImageMathInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
