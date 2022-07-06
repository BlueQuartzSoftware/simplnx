#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKImportFijiMontageInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.MontageName = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  inputValues.ColumnMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  inputValues.RowMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  inputValues.LengthUnit = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  inputValues.MontageInformation = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageInformation_Key);
  inputValues.ChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.ConvertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  inputValues.ColorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.DataContainerPath = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.ImageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  return ITKImportFijiMontage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKImportFijiMontageInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  StringParameter::ValueType MontageName;
  VectorInt32Parameter::ValueType ColumnMontageLimits;
  VectorInt32Parameter::ValueType RowMontageLimits;
  ChoicesParameter::ValueType LengthUnit;
  <<<NOT_IMPLEMENTED>>> MontageInformation;
  bool ChangeOrigin;
  VectorFloat32Parameter::ValueType Origin;
  bool ConvertToGrayScale;
  VectorFloat32Parameter::ValueType ColorWeights;
  DataPath DataContainerPath;
  DataPath CellAttributeMatrixName;
  StringParameter::ValueType ImageDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKImportFijiMontage
{
public:
  ITKImportFijiMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImportFijiMontageInputValues* inputValues);
  ~ITKImportFijiMontage() noexcept;

  ITKImportFijiMontage(const ITKImportFijiMontage&) = delete;
  ITKImportFijiMontage(ITKImportFijiMontage&&) noexcept = delete;
  ITKImportFijiMontage& operator=(const ITKImportFijiMontage&) = delete;
  ITKImportFijiMontage& operator=(ITKImportFijiMontage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKImportFijiMontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
