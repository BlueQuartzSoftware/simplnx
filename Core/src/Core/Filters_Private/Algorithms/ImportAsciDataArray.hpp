#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportAsciDataArrayInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.ScalarType = filterArgs.value<NumericType>(k_ScalarType_Key);
  inputValues.NumberOfComponents = filterArgs.value<int32>(k_NumberOfComponents_Key);
  inputValues.SkipHeaderLines = filterArgs.value<int32>(k_SkipHeaderLines_Key);
  inputValues.Delimiter = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  inputValues.CreatedAttributeArrayPath = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);
  inputValues.FirstLine = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FirstLine_Key);

  return ImportAsciDataArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ImportAsciDataArrayInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  NumericType ScalarType;
  int32 NumberOfComponents;
  int32 SkipHeaderLines;
  ChoicesParameter::ValueType Delimiter;
  DataPath CreatedAttributeArrayPath;
  <<<NOT_IMPLEMENTED>>> FirstLine;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ImportAsciDataArray
{
public:
  ImportAsciDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportAsciDataArrayInputValues* inputValues);
  ~ImportAsciDataArray() noexcept;

  ImportAsciDataArray(const ImportAsciDataArray&) = delete;
  ImportAsciDataArray(ImportAsciDataArray&&) noexcept = delete;
  ImportAsciDataArray& operator=(const ImportAsciDataArray&) = delete;
  ImportAsciDataArray& operator=(ImportAsciDataArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportAsciDataArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
