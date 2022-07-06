#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportPrintRiteTDMSFilesInputValues inputValues;

  inputValues.LayerThickness = filterArgs.value<float32>(k_LayerThickness_Key);
  inputValues.LaserOnArrayOption = filterArgs.value<ChoicesParameter::ValueType>(k_LaserOnArrayOption_Key);
  inputValues.LaserOnThreshold = filterArgs.value<float32>(k_LaserOnThreshold_Key);
  inputValues.DowncastRawData = filterArgs.value<bool>(k_DowncastRawData_Key);
  inputValues.ScaleLaserPower = filterArgs.value<bool>(k_ScaleLaserPower_Key);
  inputValues.PowerScalingCoefficients = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PowerScalingCoefficients_Key);
  inputValues.ScalePyrometerTemperature = filterArgs.value<bool>(k_ScalePyrometerTemperature_Key);
  inputValues.TemperatureScalingCoefficients = filterArgs.value<VectorFloat32Parameter::ValueType>(k_TemperatureScalingCoefficients_Key);
  inputValues.SpatialTransformOption = filterArgs.value<ChoicesParameter::ValueType>(k_SpatialTransformOption_Key);
  inputValues.LayerForScaling = filterArgs.value<int32>(k_LayerForScaling_Key);
  inputValues.SearchRadius = filterArgs.value<float32>(k_SearchRadius_Key);
  inputValues.SplitRegions1 = filterArgs.value<bool>(k_SplitRegions1_Key);
  inputValues.SplitRegions2 = filterArgs.value<bool>(k_SplitRegions2_Key);
  inputValues.STLFilePath1 = filterArgs.value<FileSystemPathParameter::ValueType>(k_STLFilePath1_Key);
  inputValues.STLFilePath2 = filterArgs.value<FileSystemPathParameter::ValueType>(k_STLFilePath2_Key);
  inputValues.InputSpatialTransformFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputSpatialTransformFilePath_Key);
  inputValues.InputFilesList = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFilesList_Key);
  inputValues.OutputDirectory = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDirectory_Key);
  inputValues.OutputFilePrefix = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);

  return ImportPrintRiteTDMSFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ImportPrintRiteTDMSFilesInputValues
{
  float32 LayerThickness;
  ChoicesParameter::ValueType LaserOnArrayOption;
  float32 LaserOnThreshold;
  bool DowncastRawData;
  bool ScaleLaserPower;
  VectorFloat32Parameter::ValueType PowerScalingCoefficients;
  bool ScalePyrometerTemperature;
  VectorFloat32Parameter::ValueType TemperatureScalingCoefficients;
  ChoicesParameter::ValueType SpatialTransformOption;
  int32 LayerForScaling;
  float32 SearchRadius;
  bool SplitRegions1;
  bool SplitRegions2;
  FileSystemPathParameter::ValueType STLFilePath1;
  FileSystemPathParameter::ValueType STLFilePath2;
  FileSystemPathParameter::ValueType InputSpatialTransformFilePath;
  GeneratedFileListParameter::ValueType InputFilesList;
  FileSystemPathParameter::ValueType OutputDirectory;
  StringParameter::ValueType OutputFilePrefix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ImportPrintRiteTDMSFiles
{
public:
  ImportPrintRiteTDMSFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportPrintRiteTDMSFilesInputValues* inputValues);
  ~ImportPrintRiteTDMSFiles() noexcept;

  ImportPrintRiteTDMSFiles(const ImportPrintRiteTDMSFiles&) = delete;
  ImportPrintRiteTDMSFiles(ImportPrintRiteTDMSFiles&&) noexcept = delete;
  ImportPrintRiteTDMSFiles& operator=(const ImportPrintRiteTDMSFiles&) = delete;
  ImportPrintRiteTDMSFiles& operator=(ImportPrintRiteTDMSFiles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportPrintRiteTDMSFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
