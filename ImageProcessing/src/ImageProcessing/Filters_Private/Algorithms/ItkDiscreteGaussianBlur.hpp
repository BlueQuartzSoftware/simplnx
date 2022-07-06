#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkDiscreteGaussianBlurInputValues inputValues;

  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);
  inputValues.Stdev = filterArgs.value<float32>(k_Stdev_Key);

  return ItkDiscreteGaussianBlur(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkDiscreteGaussianBlurInputValues
{
  bool SaveAsNewArray;
  DataPath SelectedCellArrayPath;
  DataPath NewCellArrayName;
  float32 Stdev;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkDiscreteGaussianBlur
{
public:
  ItkDiscreteGaussianBlur(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkDiscreteGaussianBlurInputValues* inputValues);
  ~ItkDiscreteGaussianBlur() noexcept;

  ItkDiscreteGaussianBlur(const ItkDiscreteGaussianBlur&) = delete;
  ItkDiscreteGaussianBlur(ItkDiscreteGaussianBlur&&) noexcept = delete;
  ItkDiscreteGaussianBlur& operator=(const ItkDiscreteGaussianBlur&) = delete;
  ItkDiscreteGaussianBlur& operator=(ItkDiscreteGaussianBlur&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkDiscreteGaussianBlurInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
