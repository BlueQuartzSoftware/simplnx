#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportPrintRiteHDF5FileInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.HFDataContainerName = filterArgs.value<StringParameter::ValueType>(k_HFDataContainerName_Key);
  inputValues.HFDataName = filterArgs.value<StringParameter::ValueType>(k_HFDataName_Key);
  inputValues.HFSliceIdsArrayName = filterArgs.value<StringParameter::ValueType>(k_HFSliceIdsArrayName_Key);
  inputValues.HFSliceDataName = filterArgs.value<StringParameter::ValueType>(k_HFSliceDataName_Key);

  return ImportPrintRiteHDF5File(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ImportPrintRiteHDF5FileInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  StringParameter::ValueType HFDataContainerName;
  StringParameter::ValueType HFDataName;
  StringParameter::ValueType HFSliceIdsArrayName;
  StringParameter::ValueType HFSliceDataName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ImportPrintRiteHDF5File
{
public:
  ImportPrintRiteHDF5File(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportPrintRiteHDF5FileInputValues* inputValues);
  ~ImportPrintRiteHDF5File() noexcept;

  ImportPrintRiteHDF5File(const ImportPrintRiteHDF5File&) = delete;
  ImportPrintRiteHDF5File(ImportPrintRiteHDF5File&&) noexcept = delete;
  ImportPrintRiteHDF5File& operator=(const ImportPrintRiteHDF5File&) = delete;
  ImportPrintRiteHDF5File& operator=(ImportPrintRiteHDF5File&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportPrintRiteHDF5FileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
