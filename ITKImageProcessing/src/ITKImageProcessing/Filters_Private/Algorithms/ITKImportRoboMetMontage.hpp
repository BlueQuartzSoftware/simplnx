#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKImportRoboMetMontageInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.MontageName = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  inputValues.ColumnMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  inputValues.RowMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  inputValues.LengthUnit = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  inputValues.MontageInformation = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageInformation_Key);
  inputValues.SliceNumber = filterArgs.value<int32>(k_SliceNumber_Key);
  inputValues.ImageFilePrefix = filterArgs.value<StringParameter::ValueType>(k_ImageFilePrefix_Key);
  inputValues.ImageFileExtension = filterArgs.value<StringParameter::ValueType>(k_ImageFileExtension_Key);
  inputValues.ChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.ChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.ConvertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  inputValues.ColorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.DataContainerPath = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.ImageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  return ITKImportRoboMetMontage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKImportRoboMetMontageInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  StringParameter::ValueType MontageName;
  VectorInt32Parameter::ValueType ColumnMontageLimits;
  VectorInt32Parameter::ValueType RowMontageLimits;
  ChoicesParameter::ValueType LengthUnit;
  <<<NOT_IMPLEMENTED>>> MontageInformation;
  int32 SliceNumber;
  StringParameter::ValueType ImageFilePrefix;
  StringParameter::ValueType ImageFileExtension;
  bool ChangeOrigin;
  VectorFloat32Parameter::ValueType Origin;
  bool ChangeSpacing;
  VectorFloat32Parameter::ValueType Spacing;
  bool ConvertToGrayScale;
  VectorFloat32Parameter::ValueType ColorWeights;
  DataPath DataContainerPath;
  StringParameter::ValueType CellAttributeMatrixName;
  StringParameter::ValueType ImageDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKImportRoboMetMontage
{
public:
  ITKImportRoboMetMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImportRoboMetMontageInputValues* inputValues);
  ~ITKImportRoboMetMontage() noexcept;

  ITKImportRoboMetMontage(const ITKImportRoboMetMontage&) = delete;
  ITKImportRoboMetMontage(ITKImportRoboMetMontage&&) noexcept = delete;
  ITKImportRoboMetMontage& operator=(const ITKImportRoboMetMontage&) = delete;
  ITKImportRoboMetMontage& operator=(ITKImportRoboMetMontage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKImportRoboMetMontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
