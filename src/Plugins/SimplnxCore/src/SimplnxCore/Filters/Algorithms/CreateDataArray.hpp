#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataStoreFormatParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateDataArrayInputValues inputValues;
  inputValues.ComponentCount = filterArgs.value<UInt64Parameter::ValueType>(component_count);
  inputValues.DataFormat = filterArgs.value<DataStoreFormatParameter::ValueType>(data_format);
  inputValues.InitializationValueStr = filterArgs.value<StringParameter::ValueType>(initialization_value_str);
  inputValues.NumericTypeIndex = filterArgs.value<NumericTypeParameter::ValueType>(numeric_type_index);
  inputValues.OutputArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(output_array_path);
  inputValues.SetTupleDimensions = filterArgs.value<BoolParameter::ValueType>(set_tuple_dimensions);
  inputValues.TupleDimensions = filterArgs.value<DynamicTableParameter::ValueType>(tuple_dimensions);
  return CreateDataArray(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateDataArrayInputValues
{
  UInt64Parameter::ValueType ComponentCount;
  DataStoreFormatParameter::ValueType DataFormat;
  StringParameter::ValueType InitializationValueStr;
  NumericTypeParameter::ValueType NumericTypeIndex;
  ArrayCreationParameter::ValueType OutputArrayPath;
  BoolParameter::ValueType SetTupleDimensions;
  DynamicTableParameter::ValueType TupleDimensions;
};

/**
 * @class CreateDataArray
 * @brief This algorithm implements support code for the CreateDataArrayFilter
 */

class SIMPLNXCORE_EXPORT CreateDataArray
{
public:
  CreateDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateDataArrayInputValues* inputValues);
  ~CreateDataArray() noexcept;

  CreateDataArray(const CreateDataArray&) = delete;
  CreateDataArray(CreateDataArray&&) noexcept = delete;
  CreateDataArray& operator=(const CreateDataArray&) = delete;
  CreateDataArray& operator=(CreateDataArray&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateDataArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
