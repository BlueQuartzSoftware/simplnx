#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CalcDewarpParametersInputValues inputValues;

  inputValues.MontageName = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageName_Key);
  inputValues.MaxIterations = filterArgs.value<int32>(k_MaxIterations_Key);
  inputValues.Delta = filterArgs.value<int32>(k_Delta_Key);
  inputValues.FractionalTolerance = filterArgs.value<float64>(k_FractionalTolerance_Key);
  inputValues.SpecifyInitialSimplex = filterArgs.value<bool>(k_SpecifyInitialSimplex_Key);
  inputValues.XFactors = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_XFactors_Key);
  inputValues.YFactors = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_YFactors_Key);
  inputValues.AttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  inputValues.IPFColorsArrayName = filterArgs.value<StringParameter::ValueType>(k_IPFColorsArrayName_Key);
  inputValues.TransformDCName = filterArgs.value<StringParameter::ValueType>(k_TransformDCName_Key);
  inputValues.TransformMatrixName = filterArgs.value<StringParameter::ValueType>(k_TransformMatrixName_Key);
  inputValues.TransformArrayName = filterArgs.value<StringParameter::ValueType>(k_TransformArrayName_Key);

  return CalcDewarpParameters(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT CalcDewarpParametersInputValues
{
  <<<NOT_IMPLEMENTED>>> MontageName;
  /*[x]*/ int32 MaxIterations;
  int32 Delta;
  float64 FractionalTolerance;
  bool SpecifyInitialSimplex;
  <<<NOT_IMPLEMENTED>>> XFactors;
  /*[x]*/<<<NOT_IMPLEMENTED>>> YFactors;
  /*[x]*/ StringParameter::ValueType AttributeMatrixName;
  StringParameter::ValueType IPFColorsArrayName;
  StringParameter::ValueType TransformDCName;
  StringParameter::ValueType TransformMatrixName;
  StringParameter::ValueType TransformArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT CalcDewarpParameters
{
public:
  CalcDewarpParameters(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CalcDewarpParametersInputValues* inputValues);
  ~CalcDewarpParameters() noexcept;

  CalcDewarpParameters(const CalcDewarpParameters&) = delete;
  CalcDewarpParameters(CalcDewarpParameters&&) noexcept = delete;
  CalcDewarpParameters& operator=(const CalcDewarpParameters&) = delete;
  CalcDewarpParameters& operator=(CalcDewarpParameters&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CalcDewarpParametersInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
