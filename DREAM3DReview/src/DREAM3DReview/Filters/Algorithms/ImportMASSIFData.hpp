#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportMASSIFDataInputValues inputValues;

  inputValues.MassifInputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_MassifInputFilePath_Key);
  inputValues.FilePrefix = filterArgs.value<StringParameter::ValueType>(k_FilePrefix_Key);
  inputValues.StepNumber = filterArgs.value<int32>(k_StepNumber_Key);

  return ImportMASSIFData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ImportMASSIFDataInputValues
{
  FileSystemPathParameter::ValueType MassifInputFilePath;
  StringParameter::ValueType FilePrefix;
  int32 StepNumber;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ImportMASSIFData
{
public:
  ImportMASSIFData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportMASSIFDataInputValues* inputValues);
  ~ImportMASSIFData() noexcept;

  ImportMASSIFData(const ImportMASSIFData&) = delete;
  ImportMASSIFData(ImportMASSIFData&&) noexcept = delete;
  ImportMASSIFData& operator=(const ImportMASSIFData&) = delete;
  ImportMASSIFData& operator=(ImportMASSIFData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportMASSIFDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
