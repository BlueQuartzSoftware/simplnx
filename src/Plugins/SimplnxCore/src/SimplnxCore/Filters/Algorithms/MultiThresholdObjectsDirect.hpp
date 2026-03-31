#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayThresholdsParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/DataTypeParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MultiThresholdObjectsInputValues inputValues;
  inputValues.ArrayThresholdsObject = filterArgs.value<ArrayThresholdsParameter::ValueType>(array_thresholds_object);
  inputValues.CreatedMaskType = filterArgs.value<DataTypeParameter::ValueType>(created_mask_type);
  inputValues.CustomFalseValue = filterArgs.value<Float64Parameter::ValueType>(custom_false_value);
  inputValues.CustomTrueValue = filterArgs.value<Float64Parameter::ValueType>(custom_true_value);
  inputValues.OutputDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(output_data_array_name);
  inputValues.UseCustomFalseValue = filterArgs.value<BoolParameter::ValueType>(use_custom_false_value);
  inputValues.UseCustomTrueValue = filterArgs.value<BoolParameter::ValueType>(use_custom_true_value);
  return MultiThresholdObjectsDirect(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT MultiThresholdObjectsInputValues
{
  ArrayThresholdsParameter::ValueType ArrayThresholdsObject;
  DataTypeParameter::ValueType CreatedMaskType;
  Float64Parameter::ValueType CustomFalseValue;
  Float64Parameter::ValueType CustomTrueValue;
  DataObjectNameParameter::ValueType OutputDataArrayName;
  BoolParameter::ValueType UseCustomFalseValue;
  BoolParameter::ValueType UseCustomTrueValue;
};

/**
 * @class MultiThresholdObjectsDirect
 * @brief This algorithm implements support code for the MultiThresholdObjectsFilter
 */

class SIMPLNXCORE_EXPORT MultiThresholdObjectsDirect
{
public:
  MultiThresholdObjectsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiThresholdObjectsInputValues* inputValues);
  ~MultiThresholdObjectsDirect() noexcept;

  MultiThresholdObjectsDirect(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect(MultiThresholdObjectsDirect&&) noexcept = delete;
  MultiThresholdObjectsDirect& operator=(const MultiThresholdObjectsDirect&) = delete;
  MultiThresholdObjectsDirect& operator=(MultiThresholdObjectsDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const MultiThresholdObjectsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
