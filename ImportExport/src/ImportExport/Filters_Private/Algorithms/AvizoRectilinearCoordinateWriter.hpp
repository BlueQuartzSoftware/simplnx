#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AvizoRectilinearCoordinateWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteBinaryFile = filterArgs.value<bool>(k_WriteBinaryFile_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.Units = filterArgs.value<StringParameter::ValueType>(k_Units_Key);

  return AvizoRectilinearCoordinateWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT AvizoRectilinearCoordinateWriterInputValues
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

class IMPORTEXPORT_EXPORT AvizoRectilinearCoordinateWriter
{
public:
  AvizoRectilinearCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   AvizoRectilinearCoordinateWriterInputValues* inputValues);
  ~AvizoRectilinearCoordinateWriter() noexcept;

  AvizoRectilinearCoordinateWriter(const AvizoRectilinearCoordinateWriter&) = delete;
  AvizoRectilinearCoordinateWriter(AvizoRectilinearCoordinateWriter&&) noexcept = delete;
  AvizoRectilinearCoordinateWriter& operator=(const AvizoRectilinearCoordinateWriter&) = delete;
  AvizoRectilinearCoordinateWriter& operator=(AvizoRectilinearCoordinateWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AvizoRectilinearCoordinateWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
