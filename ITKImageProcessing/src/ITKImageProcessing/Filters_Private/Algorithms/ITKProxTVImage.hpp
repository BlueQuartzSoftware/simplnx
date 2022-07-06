#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKProxTVImageInputValues inputValues;

  inputValues.MaximumNumberOfIterations = filterArgs.value<float64>(k_MaximumNumberOfIterations_Key);
  inputValues.Weights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Weights_Key);
  inputValues.Norms = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Norms_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  return ITKProxTVImage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKProxTVImageInputValues
{
  float64 MaximumNumberOfIterations;
  VectorFloat32Parameter::ValueType Weights;
  VectorFloat32Parameter::ValueType Norms;
  DataPath SelectedCellArrayPath;
  StringParameter::ValueType NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKProxTVImage
{
public:
  ITKProxTVImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKProxTVImageInputValues* inputValues);
  ~ITKProxTVImage() noexcept;

  ITKProxTVImage(const ITKProxTVImage&) = delete;
  ITKProxTVImage(ITKProxTVImage&&) noexcept = delete;
  ITKProxTVImage& operator=(const ITKProxTVImage&) = delete;
  ITKProxTVImage& operator=(ITKProxTVImage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKProxTVImageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
