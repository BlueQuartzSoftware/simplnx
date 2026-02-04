#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WriteASCIIDataInputValues inputValues;
  inputValues.DelimiterIndex = filterArgs.value<ChoicesParameter::ValueType>(delimiter_index);
  inputValues.FileExtension = filterArgs.value<StringParameter::ValueType>(file_extension);
  inputValues.HeaderOptionIndex = filterArgs.value<ChoicesParameter::ValueType>(header_option_index);
  inputValues.InputDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(input_data_array_paths);
  inputValues.MaxValPerLine = filterArgs.value<Int32Parameter::ValueType>(max_val_per_line);
  inputValues.OutputDir = filterArgs.value<FileSystemPathParameter::ValueType>(output_dir);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(output_path);
  inputValues.OutputStyleIndex = filterArgs.value<ChoicesParameter::ValueType>(output_style_index);
  return WriteASCIIData(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT WriteASCIIDataInputValues
{
  ChoicesParameter::ValueType DelimiterIndex;
  StringParameter::ValueType FileExtension;
  ChoicesParameter::ValueType HeaderOptionIndex;
  MultiArraySelectionParameter::ValueType InputDataArrayPaths;
  Int32Parameter::ValueType MaxValPerLine;
  FileSystemPathParameter::ValueType OutputDir;
  FileSystemPathParameter::ValueType OutputPath;
  ChoicesParameter::ValueType OutputStyleIndex;
};

/**
 * @class WriteASCIIData
 * @brief This algorithm implements support code for the WriteASCIIDataFilter
 */

class SIMPLNXCORE_EXPORT WriteASCIIData
{
public:
  WriteASCIIData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteASCIIDataInputValues* inputValues);
  ~WriteASCIIData() noexcept;

  WriteASCIIData(const WriteASCIIData&) = delete;
  WriteASCIIData(WriteASCIIData&&) noexcept = delete;
  WriteASCIIData& operator=(const WriteASCIIData&) = delete;
  WriteASCIIData& operator=(WriteASCIIData&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteASCIIDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
