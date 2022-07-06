#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExportDAMASKFilesInputValues inputValues;

  inputValues.DataFormat = filterArgs.value<ChoicesParameter::ValueType>(k_DataFormat_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.GeometryFileName = filterArgs.value<StringParameter::ValueType>(k_GeometryFileName_Key);
  inputValues.HomogenizationIndex = filterArgs.value<int32>(k_HomogenizationIndex_Key);
  inputValues.CompressGeomFile = filterArgs.value<bool>(k_CompressGeomFile_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  return ExportDAMASKFiles(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ExportDAMASKFilesInputValues
{
  ChoicesParameter::ValueType DataFormat;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType GeometryFileName;
  int32 HomogenizationIndex;
  bool CompressGeomFile;
  DataPath FeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ExportDAMASKFiles
{
public:
  ExportDAMASKFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportDAMASKFilesInputValues* inputValues);
  ~ExportDAMASKFiles() noexcept;

  ExportDAMASKFiles(const ExportDAMASKFiles&) = delete;
  ExportDAMASKFiles(ExportDAMASKFiles&&) noexcept = delete;
  ExportDAMASKFiles& operator=(const ExportDAMASKFiles&) = delete;
  ExportDAMASKFiles& operator=(ExportDAMASKFiles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExportDAMASKFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
