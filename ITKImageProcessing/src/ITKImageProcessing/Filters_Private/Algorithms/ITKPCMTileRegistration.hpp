#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKPCMTileRegistrationInputValues inputValues;

  inputValues.ColumnMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  inputValues.RowMontageLimits = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  inputValues.DataContainerPaddingDigits = filterArgs.value<int32>(k_DataContainerPaddingDigits_Key);
  inputValues.DataContainerPrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPrefix_Key);
  inputValues.CommonAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  inputValues.CommonDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  return ITKPCMTileRegistration(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKPCMTileRegistrationInputValues
{
  VectorInt32Parameter::ValueType ColumnMontageLimits;
  VectorInt32Parameter::ValueType RowMontageLimits;
  int32 DataContainerPaddingDigits;
  StringParameter::ValueType DataContainerPrefix;
  StringParameter::ValueType CommonAttributeMatrixName;
  StringParameter::ValueType CommonDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKPCMTileRegistration
{
public:
  ITKPCMTileRegistration(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKPCMTileRegistrationInputValues* inputValues);
  ~ITKPCMTileRegistration() noexcept;

  ITKPCMTileRegistration(const ITKPCMTileRegistration&) = delete;
  ITKPCMTileRegistration(ITKPCMTileRegistration&&) noexcept = delete;
  ITKPCMTileRegistration& operator=(const ITKPCMTileRegistration&) = delete;
  ITKPCMTileRegistration& operator=(ITKPCMTileRegistration&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKPCMTileRegistrationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
