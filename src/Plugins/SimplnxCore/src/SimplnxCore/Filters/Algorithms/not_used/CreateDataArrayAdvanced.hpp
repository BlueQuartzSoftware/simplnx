#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/DataStoreFormatParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateDataArrayAdvancedInputValues inputValues;
  inputValues.ComponentDimensions = filterArgs.value<DynamicTableParameter::ValueType>(component_dimensions);
  inputValues.DataFormat = filterArgs.value<DataStoreFormatParameter::ValueType>(data_format);
  inputValues.InitEndRange = filterArgs.value<StringParameter::ValueType>(init_end_range);
  inputValues.InitStartRange = filterArgs.value<StringParameter::ValueType>(init_start_range);
  inputValues.InitTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(init_type_index);
  inputValues.InitValue = filterArgs.value<StringParameter::ValueType>(init_value);
  inputValues.NumericTypeIndex = filterArgs.value<NumericTypeParameter::ValueType>(numeric_type_index);
  inputValues.OutputArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(output_array_path);
  inputValues.SeedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(seed_array_name);
  inputValues.SeedValue = filterArgs.value<UInt64Parameter::ValueType>(seed_value);
  inputValues.SetTupleDimensions = filterArgs.value<BoolParameter::ValueType>(set_tuple_dimensions);
  inputValues.StandardizeSeed = filterArgs.value<BoolParameter::ValueType>(standardize_seed);
  inputValues.StartingFillValue = filterArgs.value<StringParameter::ValueType>(starting_fill_value);
  inputValues.StepOperationIndex = filterArgs.value<ChoicesParameter::ValueType>(step_operation_index);
  inputValues.StepValue = filterArgs.value<StringParameter::ValueType>(step_value);
  inputValues.TupleDimensions = filterArgs.value<DynamicTableParameter::ValueType>(tuple_dimensions);
  inputValues.UseSeed = filterArgs.value<BoolParameter::ValueType>(use_seed);
  return CreateDataArrayAdvanced(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateDataArrayAdvancedInputValues
{
  DynamicTableParameter::ValueType ComponentDimensions;
  DataStoreFormatParameter::ValueType DataFormat;
  StringParameter::ValueType InitEndRange;
  StringParameter::ValueType InitStartRange;
  ChoicesParameter::ValueType InitTypeIndex;
  StringParameter::ValueType InitValue;
  NumericTypeParameter::ValueType NumericTypeIndex;
  ArrayCreationParameter::ValueType OutputArrayPath;
  DataObjectNameParameter::ValueType SeedArrayName;
  UInt64Parameter::ValueType SeedValue;
  BoolParameter::ValueType SetTupleDimensions;
  BoolParameter::ValueType StandardizeSeed;
  StringParameter::ValueType StartingFillValue;
  ChoicesParameter::ValueType StepOperationIndex;
  StringParameter::ValueType StepValue;
  DynamicTableParameter::ValueType TupleDimensions;
  BoolParameter::ValueType UseSeed;
};

/**
 * @class CreateDataArrayAdvanced
 * @brief This algorithm implements support code for the CreateDataArrayAdvancedFilter
 */

class SIMPLNXCORE_EXPORT CreateDataArrayAdvanced
{
public:
  CreateDataArrayAdvanced(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateDataArrayAdvancedInputValues* inputValues);
  ~CreateDataArrayAdvanced() noexcept;

  CreateDataArrayAdvanced(const CreateDataArrayAdvanced&) = delete;
  CreateDataArrayAdvanced(CreateDataArrayAdvanced&&) noexcept = delete;
  CreateDataArrayAdvanced& operator=(const CreateDataArrayAdvanced&) = delete;
  CreateDataArrayAdvanced& operator=(CreateDataArrayAdvanced&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateDataArrayAdvancedInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
