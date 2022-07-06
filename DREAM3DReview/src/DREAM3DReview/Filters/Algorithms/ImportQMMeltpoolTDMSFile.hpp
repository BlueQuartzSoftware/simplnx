#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportQMMeltpoolTDMSFileInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);

  return ImportQMMeltpoolTDMSFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ImportQMMeltpoolTDMSFileInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  DataPath DataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ImportQMMeltpoolTDMSFile
{
public:
  ImportQMMeltpoolTDMSFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportQMMeltpoolTDMSFileInputValues* inputValues);
  ~ImportQMMeltpoolTDMSFile() noexcept;

  ImportQMMeltpoolTDMSFile(const ImportQMMeltpoolTDMSFile&) = delete;
  ImportQMMeltpoolTDMSFile(ImportQMMeltpoolTDMSFile&&) noexcept = delete;
  ImportQMMeltpoolTDMSFile& operator=(const ImportQMMeltpoolTDMSFile&) = delete;
  ImportQMMeltpoolTDMSFile& operator=(ImportQMMeltpoolTDMSFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportQMMeltpoolTDMSFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
