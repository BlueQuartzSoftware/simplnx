#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ApplyDewarpParametersInputValues inputValues;

  inputValues.MontageName = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageName_Key);
  inputValues.AttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  inputValues.TransformPath = filterArgs.value<DataPath>(k_TransformPath_Key);
  inputValues.TransformPrefix = filterArgs.value<StringParameter::ValueType>(k_TransformPrefix_Key);
  inputValues.MaskName = filterArgs.value<StringParameter::ValueType>(k_MaskName_Key);

  return ApplyDewarpParameters(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ApplyDewarpParametersInputValues
{
  <<<NOT_IMPLEMENTED>>> MontageName;
  /*[x]*/ StringParameter::ValueType AttributeMatrixName;
  DataPath TransformPath;
  StringParameter::ValueType TransformPrefix;
  StringParameter::ValueType MaskName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ApplyDewarpParameters
{
public:
  ApplyDewarpParameters(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ApplyDewarpParametersInputValues* inputValues);
  ~ApplyDewarpParameters() noexcept;

  ApplyDewarpParameters(const ApplyDewarpParameters&) = delete;
  ApplyDewarpParameters(ApplyDewarpParameters&&) noexcept = delete;
  ApplyDewarpParameters& operator=(const ApplyDewarpParameters&) = delete;
  ApplyDewarpParameters& operator=(ApplyDewarpParameters&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ApplyDewarpParametersInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
