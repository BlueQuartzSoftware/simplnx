#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WriteASCIIDataInputValues inputValues;

  inputValues.OutputStyle = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.FileExtension = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  inputValues.MaxValPerLine = filterArgs.value<int32>(k_MaxValPerLine_Key);
  inputValues.OutputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);
  inputValues.Delimiter = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  return WriteASCIIData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT WriteASCIIDataInputValues
{
  ChoicesParameter::ValueType OutputStyle;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType FileExtension;
  int32 MaxValPerLine;
  FileSystemPathParameter::ValueType OutputFilePath;
  ChoicesParameter::ValueType Delimiter;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT WriteASCIIData
{
public:
  WriteASCIIData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteASCIIDataInputValues* inputValues);
  ~WriteASCIIData() noexcept;

  WriteASCIIData(const WriteASCIIData&) = delete;
  WriteASCIIData(WriteASCIIData&&) noexcept = delete;
  WriteASCIIData& operator=(const WriteASCIIData&) = delete;
  WriteASCIIData& operator=(WriteASCIIData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteASCIIDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
