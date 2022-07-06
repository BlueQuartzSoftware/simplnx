#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  IlluminationCorrectionInputValues inputValues;

  inputValues.MontageSelection = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.ImageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  inputValues.CorrectedImageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CorrectedImageDataArrayName_Key);
  inputValues.BackgroundDataContainerPath = filterArgs.value<DataPath>(k_BackgroundDataContainerPath_Key);
  inputValues.BackgroundCellAttributeMatrixPath = filterArgs.value<DataPath>(k_BackgroundCellAttributeMatrixPath_Key);
  inputValues.BackgroundImageArrayPath = filterArgs.value<DataPath>(k_BackgroundImageArrayPath_Key);
  inputValues.LowThreshold = filterArgs.value<int32>(k_LowThreshold_Key);
  inputValues.HighThreshold = filterArgs.value<int32>(k_HighThreshold_Key);
  inputValues.ApplyMedianFilter = filterArgs.value<bool>(k_ApplyMedianFilter_Key);
  inputValues.MedianRadius = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MedianRadius_Key);
  inputValues.ApplyCorrection = filterArgs.value<bool>(k_ApplyCorrection_Key);
  inputValues.ExportCorrectedImages = filterArgs.value<bool>(k_ExportCorrectedImages_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);

  return IlluminationCorrection(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT IlluminationCorrectionInputValues
{
  <<<NOT_IMPLEMENTED>>> MontageSelection;
  /*[x]*/ StringParameter::ValueType CellAttributeMatrixName;
  StringParameter::ValueType ImageDataArrayName;
  StringParameter::ValueType CorrectedImageDataArrayName;
  DataPath BackgroundDataContainerPath;
  DataPath BackgroundCellAttributeMatrixPath;
  DataPath BackgroundImageArrayPath;
  int32 LowThreshold;
  int32 HighThreshold;
  bool ApplyMedianFilter;
  VectorFloat32Parameter::ValueType MedianRadius;
  bool ApplyCorrection;
  bool ExportCorrectedImages;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType FileExtension;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT IlluminationCorrection
{
public:
  IlluminationCorrection(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IlluminationCorrectionInputValues* inputValues);
  ~IlluminationCorrection() noexcept;

  IlluminationCorrection(const IlluminationCorrection&) = delete;
  IlluminationCorrection(IlluminationCorrection&&) noexcept = delete;
  IlluminationCorrection& operator=(const IlluminationCorrection&) = delete;
  IlluminationCorrection& operator=(IlluminationCorrection&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const IlluminationCorrectionInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
