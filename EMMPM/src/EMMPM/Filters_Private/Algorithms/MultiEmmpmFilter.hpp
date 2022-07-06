#pragma once

#include "EMMPM/EMMPM_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MultiEmmpmFilterInputValues inputValues;

  inputValues.NumClasses = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NumClasses_Key);
  inputValues.UseOneBasedValues = filterArgs.value<bool>(k_UseOneBasedValues_Key);
  inputValues.UseGradientPenalty = filterArgs.value<bool>(k_UseGradientPenalty_Key);
  inputValues.GradientBetaE = filterArgs.value<float64>(k_GradientBetaE_Key);
  inputValues.UseCurvaturePenalty = filterArgs.value<bool>(k_UseCurvaturePenalty_Key);
  inputValues.CurvatureBetaC = filterArgs.value<float64>(k_CurvatureBetaC_Key);
  inputValues.CurvatureRMax = filterArgs.value<float64>(k_CurvatureRMax_Key);
  inputValues.CurvatureEMLoopDelay = filterArgs.value<int32>(k_CurvatureEMLoopDelay_Key);
  inputValues.InputDataArrayVector = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayVector_Key);
  inputValues.OutputAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_OutputAttributeMatrixName_Key);
  inputValues.UsePreviousMuSigma = filterArgs.value<bool>(k_UsePreviousMuSigma_Key);
  inputValues.OutputArrayPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputArrayPrefix_Key);

  return MultiEmmpmFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct EMMPM_EXPORT MultiEmmpmFilterInputValues
{
  <<<NOT_IMPLEMENTED>>> NumClasses;
  /*[x]*/ bool UseOneBasedValues;
  bool UseGradientPenalty;
  float64 GradientBetaE;
  bool UseCurvaturePenalty;
  float64 CurvatureBetaC;
  float64 CurvatureRMax;
  int32 CurvatureEMLoopDelay;
  MultiArraySelectionParameter::ValueType InputDataArrayVector;
  StringParameter::ValueType OutputAttributeMatrixName;
  bool UsePreviousMuSigma;
  StringParameter::ValueType OutputArrayPrefix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class EMMPM_EXPORT MultiEmmpmFilter
{
public:
  MultiEmmpmFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MultiEmmpmFilterInputValues* inputValues);
  ~MultiEmmpmFilter() noexcept;

  MultiEmmpmFilter(const MultiEmmpmFilter&) = delete;
  MultiEmmpmFilter(MultiEmmpmFilter&&) noexcept = delete;
  MultiEmmpmFilter& operator=(const MultiEmmpmFilter&) = delete;
  MultiEmmpmFilter& operator=(MultiEmmpmFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MultiEmmpmFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
