#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExportCLIFileInputValues inputValues;

  inputValues.UnitsScaleFactor = filterArgs.value<float64>(k_UnitsScaleFactor_Key);
  inputValues.Precision = filterArgs.value<int32>(k_Precision_Key);
  inputValues.OutputDirectory = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDirectory_Key);
  inputValues.OutputFilePrefix = filterArgs.value<StringParameter::ValueType>(k_OutputFilePrefix_Key);
  inputValues.SplitByGroup = filterArgs.value<bool>(k_SplitByGroup_Key);
  inputValues.EdgeGeometry = filterArgs.value<DataPath>(k_EdgeGeometry_Key);
  inputValues.LayerIdsArrayPath = filterArgs.value<DataPath>(k_LayerIdsArrayPath_Key);
  inputValues.GroupIdsArrayPath = filterArgs.value<DataPath>(k_GroupIdsArrayPath_Key);

  return ExportCLIFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ExportCLIFileInputValues
{
  float64 UnitsScaleFactor;
  int32 Precision;
  FileSystemPathParameter::ValueType OutputDirectory;
  StringParameter::ValueType OutputFilePrefix;
  bool SplitByGroup;
  DataPath EdgeGeometry;
  DataPath LayerIdsArrayPath;
  DataPath GroupIdsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ExportCLIFile
{
public:
  ExportCLIFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportCLIFileInputValues* inputValues);
  ~ExportCLIFile() noexcept;

  ExportCLIFile(const ExportCLIFile&) = delete;
  ExportCLIFile(ExportCLIFile&&) noexcept = delete;
  ExportCLIFile& operator=(const ExportCLIFile&) = delete;
  ExportCLIFile& operator=(ExportCLIFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExportCLIFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
