#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKImageReaderInputValues inputValues;

  inputValues.FileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.ImageDataArrayName = filterArgs.value<DataPath>(k_ImageDataArrayName_Key);

  return ITKImageReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKImageReaderInputValues
{
  FileSystemPathParameter::ValueType FileName;
  DataPath DataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath ImageDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKImageReader
{
public:
  ITKImageReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImageReaderInputValues* inputValues);
  ~ITKImageReader() noexcept;

  ITKImageReader(const ITKImageReader&) = delete;
  ITKImageReader(ITKImageReader&&) noexcept = delete;
  ITKImageReader& operator=(const ITKImageReader&) = delete;
  ITKImageReader& operator=(ITKImageReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKImageReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
