#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AvizoUniformCoordinateWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.Units = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  return AvizoUniformCoordinateWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT AvizoUniformCoordinateWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath FeatureIdsArrayPath;
  StringParameter::ValueType Units;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT AvizoUniformCoordinateWriter
{
public:
  AvizoUniformCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoUniformCoordinateWriterInputValues* inputValues);
  ~AvizoUniformCoordinateWriter() noexcept;

  AvizoUniformCoordinateWriter(const AvizoUniformCoordinateWriter&) = delete;
  AvizoUniformCoordinateWriter(AvizoUniformCoordinateWriter&&) noexcept = delete;
  AvizoUniformCoordinateWriter& operator=(const AvizoUniformCoordinateWriter&) = delete;
  AvizoUniformCoordinateWriter& operator=(AvizoUniformCoordinateWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AvizoUniformCoordinateWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
