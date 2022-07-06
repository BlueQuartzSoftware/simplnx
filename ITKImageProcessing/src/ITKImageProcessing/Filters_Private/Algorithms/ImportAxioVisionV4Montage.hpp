#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportAxioVisionV4MontageInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.MontageName = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  inputValues.ColumnMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  inputValues.RowMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  inputValues.MontageInformation = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageInformation_Key);
  inputValues.ImportAllMetaData = filterArgs.value<bool>(k_ImportAllMetaData_Key);
  inputValues.ChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.ChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.ConvertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  inputValues.ColorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.DataContainerPath = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  inputValues.ImageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  inputValues.MetaDataAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_MetaDataAttributeMatrixName_Key);

  return ImportAxioVisionV4Montage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ImportAxioVisionV4MontageInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  StringParameter::ValueType MontageName;
  VectorInt32Parameter::ValueType ColumnMontageLimits;
  VectorInt32Parameter::ValueType RowMontageLimits;
  <<<NOT_IMPLEMENTED>>> MontageInformation;
  bool ImportAllMetaData;
  bool ChangeOrigin;
  VectorFloat32Parameter::ValueType Origin;
  bool ChangeSpacing;
  VectorFloat32Parameter::ValueType Spacing;
  bool ConvertToGrayScale;
  VectorFloat32Parameter::ValueType ColorWeights;
  DataPath DataContainerPath;
  StringParameter::ValueType CellAttributeMatrixName;
  StringParameter::ValueType ImageDataArrayName;
  StringParameter::ValueType MetaDataAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ImportAxioVisionV4Montage
{
public:
  ImportAxioVisionV4Montage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportAxioVisionV4MontageInputValues* inputValues);
  ~ImportAxioVisionV4Montage() noexcept;

  ImportAxioVisionV4Montage(const ImportAxioVisionV4Montage&) = delete;
  ImportAxioVisionV4Montage(ImportAxioVisionV4Montage&&) noexcept = delete;
  ImportAxioVisionV4Montage& operator=(const ImportAxioVisionV4Montage&) = delete;
  ImportAxioVisionV4Montage& operator=(ImportAxioVisionV4Montage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportAxioVisionV4MontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
