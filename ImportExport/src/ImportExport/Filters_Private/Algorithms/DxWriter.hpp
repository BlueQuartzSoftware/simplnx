#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DxWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.AddSurfaceLayer = filterArgs.value<bool>(k_AddSurfaceLayer_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);

  return DxWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT DxWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool AddSurfaceLayer;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT DxWriter
{
public:
  DxWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DxWriterInputValues* inputValues);
  ~DxWriter() noexcept;

  DxWriter(const DxWriter&) = delete;
  DxWriter(DxWriter&&) noexcept = delete;
  DxWriter& operator=(const DxWriter&) = delete;
  DxWriter& operator=(DxWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DxWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
