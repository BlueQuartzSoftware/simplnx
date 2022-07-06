#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKDiscreteGaussianImageInputValues inputValues;

  inputValues.Variance = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Variance_Key);
  inputValues.MaximumKernelWidth = filterArgs.value<int32>(k_MaximumKernelWidth_Key);
  inputValues.MaximumError = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaximumError_Key);
  inputValues.UseImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  return ITKDiscreteGaussianImage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKDiscreteGaussianImageInputValues
{
  VectorFloat32Parameter::ValueType Variance;
  int32 MaximumKernelWidth;
  VectorFloat32Parameter::ValueType MaximumError;
  bool UseImageSpacing;
  DataPath SelectedCellArrayPath;
  StringParameter::ValueType NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKDiscreteGaussianImage
{
public:
  ITKDiscreteGaussianImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKDiscreteGaussianImageInputValues* inputValues);
  ~ITKDiscreteGaussianImage() noexcept;

  ITKDiscreteGaussianImage(const ITKDiscreteGaussianImage&) = delete;
  ITKDiscreteGaussianImage(ITKDiscreteGaussianImage&&) noexcept = delete;
  ITKDiscreteGaussianImage& operator=(const ITKDiscreteGaussianImage&) = delete;
  ITKDiscreteGaussianImage& operator=(ITKDiscreteGaussianImage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKDiscreteGaussianImageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
