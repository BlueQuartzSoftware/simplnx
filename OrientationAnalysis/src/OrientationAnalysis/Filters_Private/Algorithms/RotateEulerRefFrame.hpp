#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RotateEulerRefFrameInputValues inputValues;

  inputValues.RotationAngle = filterArgs.value<float32>(k_RotationAngle_Key);
  inputValues.RotationAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  return RotateEulerRefFrame(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrameInputValues
{
  float32 RotationAngle;
  VectorFloat32Parameter::ValueType RotationAxis;
  DataPath CellEulerAnglesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrame
{
public:
  RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues);
  ~RotateEulerRefFrame() noexcept;

  RotateEulerRefFrame(const RotateEulerRefFrame&) = delete;
  RotateEulerRefFrame(RotateEulerRefFrame&&) noexcept = delete;
  RotateEulerRefFrame& operator=(const RotateEulerRefFrame&) = delete;
  RotateEulerRefFrame& operator=(RotateEulerRefFrame&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RotateEulerRefFrameInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
