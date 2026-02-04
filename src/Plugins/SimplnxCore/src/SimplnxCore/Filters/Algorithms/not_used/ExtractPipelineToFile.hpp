#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExtractPipelineToFileInputValues inputValues;
  inputValues.InputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(input_file_path);
  inputValues.OutputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(output_file_path);
  return ExtractPipelineToFile(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ExtractPipelineToFileInputValues
{
  FileSystemPathParameter::ValueType InputFilePath;
  FileSystemPathParameter::ValueType OutputFilePath;
};

/**
 * @class ExtractPipelineToFile
 * @brief This algorithm implements support code for the ExtractPipelineToFileFilter
 */

class SIMPLNXCORE_EXPORT ExtractPipelineToFile
{
public:
  ExtractPipelineToFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractPipelineToFileInputValues* inputValues);
  ~ExtractPipelineToFile() noexcept;

  ExtractPipelineToFile(const ExtractPipelineToFile&) = delete;
  ExtractPipelineToFile(ExtractPipelineToFile&&) noexcept = delete;
  ExtractPipelineToFile& operator=(const ExtractPipelineToFile&) = delete;
  ExtractPipelineToFile& operator=(ExtractPipelineToFile&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ExtractPipelineToFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
