#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKImageWriterInputValues inputValues;

  inputValues.Plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.FileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  inputValues.IndexOffset = filterArgs.value<int32>(k_IndexOffset_Key);
  inputValues.ImageArrayPath = filterArgs.value<DataPath>(k_ImageArrayPath_Key);

  return ITKImageWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKImageWriterInputValues
{
  ChoicesParameter::ValueType Plane;
  FileSystemPathParameter::ValueType FileName;
  int32 IndexOffset;
  DataPath ImageArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKImageWriter
{
public:
  ITKImageWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImageWriterInputValues* inputValues);
  ~ITKImageWriter() noexcept;

  ITKImageWriter(const ITKImageWriter&) = delete;
  ITKImageWriter(ITKImageWriter&&) noexcept = delete;
  ITKImageWriter& operator=(const ITKImageWriter&) = delete;
  ITKImageWriter& operator=(ITKImageWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKImageWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
