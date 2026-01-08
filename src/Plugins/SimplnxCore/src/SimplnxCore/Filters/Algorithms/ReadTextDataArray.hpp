#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataStoreFormatParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReadTextDataArrayInputValues inputValues;
  inputValues.DataFormat = filterArgs.value<DataStoreFormatParameter::ValueType>(data_format);
  inputValues.DelimiterIndex = filterArgs.value<ChoicesParameter::ValueType>(delimiter_index);
  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(input_file);
  inputValues.NumberComp = filterArgs.value<UInt64Parameter::ValueType>(number_comp);
  inputValues.NumberTuples = filterArgs.value<DynamicTableParameter::ValueType>(number_tuples);
  inputValues.OutputDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(output_data_array_path);
  inputValues.ScalarTypeIndex = filterArgs.value<NumericTypeParameter::ValueType>(scalar_type_index);
  inputValues.SetTupleDimensions = filterArgs.value<BoolParameter::ValueType>(set_tuple_dimensions);
  inputValues.SkipLineCount = filterArgs.value<UInt64Parameter::ValueType>(skip_line_count);
  return ReadTextDataArray(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadTextDataArrayInputValues
{
  DataStoreFormatParameter::ValueType DataFormat;
  ChoicesParameter::ValueType DelimiterIndex;
  FileSystemPathParameter::ValueType InputFile;
  UInt64Parameter::ValueType NumberComp;
  DynamicTableParameter::ValueType NumberTuples;
  ArrayCreationParameter::ValueType OutputDataArrayPath;
  NumericTypeParameter::ValueType ScalarTypeIndex;
  BoolParameter::ValueType SetTupleDimensions;
  UInt64Parameter::ValueType SkipLineCount;
};

/**
 * @class ReadTextDataArray
 * @brief This algorithm implements support code for the ReadTextDataArrayFilter
 */

class SIMPLNXCORE_EXPORT ReadTextDataArray
{
public:
  ReadTextDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadTextDataArrayInputValues* inputValues);
  ~ReadTextDataArray() noexcept;

  ReadTextDataArray(const ReadTextDataArray&) = delete;
  ReadTextDataArray(ReadTextDataArray&&) noexcept = delete;
  ReadTextDataArray& operator=(const ReadTextDataArray&) = delete;
  ReadTextDataArray& operator=(ReadTextDataArray&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadTextDataArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
