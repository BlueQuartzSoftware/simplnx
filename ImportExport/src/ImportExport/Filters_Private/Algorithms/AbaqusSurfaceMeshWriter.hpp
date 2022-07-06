#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AbaqusSurfaceMeshWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  return AbaqusSurfaceMeshWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT AbaqusSurfaceMeshWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath SurfaceMeshFaceLabelsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT AbaqusSurfaceMeshWriter
{
public:
  AbaqusSurfaceMeshWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AbaqusSurfaceMeshWriterInputValues* inputValues);
  ~AbaqusSurfaceMeshWriter() noexcept;

  AbaqusSurfaceMeshWriter(const AbaqusSurfaceMeshWriter&) = delete;
  AbaqusSurfaceMeshWriter(AbaqusSurfaceMeshWriter&&) noexcept = delete;
  AbaqusSurfaceMeshWriter& operator=(const AbaqusSurfaceMeshWriter&) = delete;
  AbaqusSurfaceMeshWriter& operator=(AbaqusSurfaceMeshWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AbaqusSurfaceMeshWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
