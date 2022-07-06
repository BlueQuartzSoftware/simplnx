#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExportMultiOnScaleTableFileInputValues inputValues;

  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.DataContainerPrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  inputValues.MatrixName = filterArgs.value<StringParameter::ValueType>(k_MatrixName_Key);
  inputValues.ArrayName = filterArgs.value<StringParameter::ValueType>(k_ArrayName_Key);
  inputValues.NumKeypoints = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumKeypoints_Key);
  inputValues.SelectedArrays = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedArrays_Key);
  inputValues.PhaseNamesArrayPath = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);

  return ExportMultiOnScaleTableFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ExportMultiOnScaleTableFileInputValues
{
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType DataContainerPrefix;
  StringParameter::ValueType MatrixName;
  StringParameter::ValueType ArrayName;
  VectorInt32Parameter::ValueType NumKeypoints;
  <<<NOT_IMPLEMENTED>>> SelectedArrays;
  DataPath PhaseNamesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ExportMultiOnScaleTableFile
{
public:
  ExportMultiOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportMultiOnScaleTableFileInputValues* inputValues);
  ~ExportMultiOnScaleTableFile() noexcept;

  ExportMultiOnScaleTableFile(const ExportMultiOnScaleTableFile&) = delete;
  ExportMultiOnScaleTableFile(ExportMultiOnScaleTableFile&&) noexcept = delete;
  ExportMultiOnScaleTableFile& operator=(const ExportMultiOnScaleTableFile&) = delete;
  ExportMultiOnScaleTableFile& operator=(ExportMultiOnScaleTableFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExportMultiOnScaleTableFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
