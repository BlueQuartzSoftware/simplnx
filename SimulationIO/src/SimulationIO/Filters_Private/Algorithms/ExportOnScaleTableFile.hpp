#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExportOnScaleTableFileInputValues inputValues;

  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.OutputFilePrefix = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  inputValues.NumKeypoints = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumKeypoints_Key);
  inputValues.PhaseNamesArrayPath = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);
  inputValues.PzflexFeatureIdsArrayPath = filterArgs.value<DataPath>(k_PzflexFeatureIdsArrayPath_Key);

  return ExportOnScaleTableFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ExportOnScaleTableFileInputValues
{
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType OutputFilePrefix;
  VectorInt32Parameter::ValueType NumKeypoints;
  DataPath PhaseNamesArrayPath;
  DataPath PzflexFeatureIdsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ExportOnScaleTableFile
{
public:
  ExportOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportOnScaleTableFileInputValues* inputValues);
  ~ExportOnScaleTableFile() noexcept;

  ExportOnScaleTableFile(const ExportOnScaleTableFile&) = delete;
  ExportOnScaleTableFile(ExportOnScaleTableFile&&) noexcept = delete;
  ExportOnScaleTableFile& operator=(const ExportOnScaleTableFile&) = delete;
  ExportOnScaleTableFile& operator=(ExportOnScaleTableFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExportOnScaleTableFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
