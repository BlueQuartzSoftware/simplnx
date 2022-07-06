#pragma once

#include "EMMPM/EMMPM_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  EMMPMFilterInputValues inputValues;

  inputValues.NumClasses = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_NumClasses_Key);
  inputValues.UseOneBasedValues = filterArgs.value<bool>(k_UseOneBasedValues_Key);
  inputValues.UseGradientPenalty = filterArgs.value<bool>(k_UseGradientPenalty_Key);
  inputValues.GradientBetaE = filterArgs.value<float64>(k_GradientBetaE_Key);
  inputValues.UseCurvaturePenalty = filterArgs.value<bool>(k_UseCurvaturePenalty_Key);
  inputValues.CurvatureBetaC = filterArgs.value<float64>(k_CurvatureBetaC_Key);
  inputValues.CurvatureRMax = filterArgs.value<float64>(k_CurvatureRMax_Key);
  inputValues.CurvatureEMLoopDelay = filterArgs.value<int32>(k_CurvatureEMLoopDelay_Key);
  inputValues.InputDataArrayPath = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);

  return EMMPMFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct EMMPM_EXPORT EMMPMFilterInputValues
{
  <<<NOT_IMPLEMENTED>>> NumClasses;
  /*[x]*/ bool UseOneBasedValues;
  bool UseGradientPenalty;
  float64 GradientBetaE;
  bool UseCurvaturePenalty;
  float64 CurvatureBetaC;
  float64 CurvatureRMax;
  int32 CurvatureEMLoopDelay;
  DataPath InputDataArrayPath;
  DataPath OutputDataArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class EMMPM_EXPORT EMMPMFilter
{
public:
  EMMPMFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EMMPMFilterInputValues* inputValues);
  ~EMMPMFilter() noexcept;

  EMMPMFilter(const EMMPMFilter&) = delete;
  EMMPMFilter(EMMPMFilter&&) noexcept = delete;
  EMMPMFilter& operator=(const EMMPMFilter&) = delete;
  EMMPMFilter& operator=(EMMPMFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EMMPMFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
