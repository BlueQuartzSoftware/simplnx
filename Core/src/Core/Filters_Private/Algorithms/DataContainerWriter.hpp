#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DataContainerWriterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteXdmfFile = filterArgs.value<bool>(k_WriteXdmfFile_Key);
  inputValues.WriteTimeSeries = filterArgs.value<bool>(k_WriteTimeSeries_Key);

  return DataContainerWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT DataContainerWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteXdmfFile;
  bool WriteTimeSeries;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT DataContainerWriter
{
public:
  DataContainerWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DataContainerWriterInputValues* inputValues);
  ~DataContainerWriter() noexcept;

  DataContainerWriter(const DataContainerWriter&) = delete;
  DataContainerWriter(DataContainerWriter&&) noexcept = delete;
  DataContainerWriter& operator=(const DataContainerWriter&) = delete;
  DataContainerWriter& operator=(DataContainerWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DataContainerWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
