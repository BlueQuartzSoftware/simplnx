#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKStitchMontageInputValues inputValues;

  inputValues.MontageSelection = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  inputValues.CommonAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  inputValues.CommonDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);
  inputValues.MontageDataContainerName = filterArgs.value<StringParameter::ValueType>(k_MontageDataContainerName_Key);
  inputValues.MontageAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_MontageAttributeMatrixName_Key);
  inputValues.MontageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_MontageDataArrayName_Key);

  return ITKStitchMontage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKStitchMontageInputValues
{
  <<<NOT_IMPLEMENTED>>> MontageSelection;
  /*[x]*/ StringParameter::ValueType CommonAttributeMatrixName;
  StringParameter::ValueType CommonDataArrayName;
  StringParameter::ValueType MontageDataContainerName;
  StringParameter::ValueType MontageAttributeMatrixName;
  StringParameter::ValueType MontageDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKStitchMontage
{
public:
  ITKStitchMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKStitchMontageInputValues* inputValues);
  ~ITKStitchMontage() noexcept;

  ITKStitchMontage(const ITKStitchMontage&) = delete;
  ITKStitchMontage(ITKStitchMontage&&) noexcept = delete;
  ITKStitchMontage& operator=(const ITKStitchMontage&) = delete;
  ITKStitchMontage& operator=(ITKStitchMontage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKStitchMontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
