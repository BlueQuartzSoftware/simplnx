#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkMedianKernelInputValues inputValues;

  inputValues.KernelSize = filterArgs.value<VectorInt32Parameter::ValueType>(k_KernelSize_Key);
  inputValues.Slice = filterArgs.value<bool>(k_Slice_Key);
  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkMedianKernel(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkMedianKernelInputValues
{
  VectorInt32Parameter::ValueType KernelSize;
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

class IMAGEPROCESSING_EXPORT ItkMedianKernel
{
public:
  ItkMedianKernel(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkMedianKernelInputValues* inputValues);
  ~ItkMedianKernel() noexcept;

  ItkMedianKernel(const ItkMedianKernel&) = delete;
  ItkMedianKernel(ItkMedianKernel&&) noexcept = delete;
  ItkMedianKernel& operator=(const ItkMedianKernel&) = delete;
  ItkMedianKernel& operator=(ItkMedianKernel&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkMedianKernelInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
