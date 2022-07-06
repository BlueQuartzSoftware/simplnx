#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkImageCalculatorInputValues inputValues;

  inputValues.SelectedCellArrayPath1 = filterArgs.value<DataPath>(k_SelectedCellArrayPath1_Key);
  inputValues.Operator = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  inputValues.SelectedCellArrayPath2 = filterArgs.value<DataPath>(k_SelectedCellArrayPath2_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkImageCalculator(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkImageCalculatorInputValues
{
  DataPath SelectedCellArrayPath1;
  ChoicesParameter::ValueType Operator;
  DataPath SelectedCellArrayPath2;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkImageCalculator
{
public:
  ItkImageCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkImageCalculatorInputValues* inputValues);
  ~ItkImageCalculator() noexcept;

  ItkImageCalculator(const ItkImageCalculator&) = delete;
  ItkImageCalculator(ItkImageCalculator&&) noexcept = delete;
  ItkImageCalculator& operator=(const ItkImageCalculator&) = delete;
  ItkImageCalculator& operator=(ItkImageCalculator&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkImageCalculatorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
