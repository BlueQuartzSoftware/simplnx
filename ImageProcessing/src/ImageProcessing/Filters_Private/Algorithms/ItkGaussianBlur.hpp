#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkGaussianBlurInputValues inputValues;

  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkGaussianBlur(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkGaussianBlurInputValues
{
  bool SaveAsNewArray;
  DataPath SelectedCellArrayPath;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkGaussianBlur
{
public:
  ItkGaussianBlur(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkGaussianBlurInputValues* inputValues);
  ~ItkGaussianBlur() noexcept;

  ItkGaussianBlur(const ItkGaussianBlur&) = delete;
  ItkGaussianBlur(ItkGaussianBlur&&) noexcept = delete;
  ItkGaussianBlur& operator=(const ItkGaussianBlur&) = delete;
  ItkGaussianBlur& operator=(ItkGaussianBlur&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkGaussianBlurInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
