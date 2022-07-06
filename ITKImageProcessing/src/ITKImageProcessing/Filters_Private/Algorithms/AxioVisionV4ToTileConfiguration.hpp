#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AxioVisionV4ToTileConfigurationInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);

  return AxioVisionV4ToTileConfiguration(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT AxioVisionV4ToTileConfigurationInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  FileSystemPathParameter::ValueType OutputFile;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT AxioVisionV4ToTileConfiguration
{
public:
  AxioVisionV4ToTileConfiguration(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  AxioVisionV4ToTileConfigurationInputValues* inputValues);
  ~AxioVisionV4ToTileConfiguration() noexcept;

  AxioVisionV4ToTileConfiguration(const AxioVisionV4ToTileConfiguration&) = delete;
  AxioVisionV4ToTileConfiguration(AxioVisionV4ToTileConfiguration&&) noexcept = delete;
  AxioVisionV4ToTileConfiguration& operator=(const AxioVisionV4ToTileConfiguration&) = delete;
  AxioVisionV4ToTileConfiguration& operator=(AxioVisionV4ToTileConfiguration&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AxioVisionV4ToTileConfigurationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
