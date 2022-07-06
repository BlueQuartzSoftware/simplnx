#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  EMsoftSO3SamplerInputValues inputValues;

  inputValues.sampleModeSelector = filterArgs.value<ChoicesParameter::ValueType>(k_sampleModeSelector_Key);
  inputValues.PointGroup = filterArgs.value<int32>(k_PointGroup_Key);
  inputValues.OffsetGrid = filterArgs.value<bool>(k_OffsetGrid_Key);
  inputValues.MisOr = filterArgs.value<float64>(k_MisOr_Key);
  inputValues.RefOr = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RefOr_Key);
  inputValues.MisOrFull = filterArgs.value<float64>(k_MisOrFull_Key);
  inputValues.RefOrFull = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RefOrFull_Key);
  inputValues.Numsp = filterArgs.value<int32>(k_Numsp_Key);
  inputValues.EulerAnglesArrayName = filterArgs.value<DataPath>(k_EulerAnglesArrayName_Key);

  return EMsoftSO3Sampler(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT EMsoftSO3SamplerInputValues
{
  ChoicesParameter::ValueType sampleModeSelector;
  int32 PointGroup;
  bool OffsetGrid;
  float64 MisOr;
  VectorFloat32Parameter::ValueType RefOr;
  float64 MisOrFull;
  VectorFloat32Parameter::ValueType RefOrFull;
  int32 Numsp;
  DataPath EulerAnglesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT EMsoftSO3Sampler
{
public:
  EMsoftSO3Sampler(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EMsoftSO3SamplerInputValues* inputValues);
  ~EMsoftSO3Sampler() noexcept;

  EMsoftSO3Sampler(const EMsoftSO3Sampler&) = delete;
  EMsoftSO3Sampler(EMsoftSO3Sampler&&) noexcept = delete;
  EMsoftSO3Sampler& operator=(const EMsoftSO3Sampler&) = delete;
  EMsoftSO3Sampler& operator=(EMsoftSO3Sampler&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EMsoftSO3SamplerInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
