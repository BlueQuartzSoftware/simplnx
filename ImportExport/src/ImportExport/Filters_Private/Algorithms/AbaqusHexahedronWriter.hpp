#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AbaqusHexahedronWriterInputValues inputValues;

  inputValues.HourglassStiffness = filterArgs.value<int32>(k_HourglassStiffness_Key);
  inputValues.JobName = filterArgs.value<StringParameter::ValueType>(k_JobName_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FilePrefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  return AbaqusHexahedronWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT AbaqusHexahedronWriterInputValues
{
  int32 HourglassStiffness;
  StringParameter::ValueType JobName;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType FilePrefix;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT AbaqusHexahedronWriter
{
public:
  AbaqusHexahedronWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AbaqusHexahedronWriterInputValues* inputValues);
  ~AbaqusHexahedronWriter() noexcept;

  AbaqusHexahedronWriter(const AbaqusHexahedronWriter&) = delete;
  AbaqusHexahedronWriter(AbaqusHexahedronWriter&&) noexcept = delete;
  AbaqusHexahedronWriter& operator=(const AbaqusHexahedronWriter&) = delete;
  AbaqusHexahedronWriter& operator=(AbaqusHexahedronWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AbaqusHexahedronWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
