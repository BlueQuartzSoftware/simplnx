#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

/**
 * @brief The RotateEulerRefFrameInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrameInputValues
{
  float rotationAngle;
  std::vector<float> rotationAxis;
  DataPath eulerAngleDataPath;
};

/**
 * @brief The RotateEulerRefFrame class
 */
class RotateEulerRefFrame
{
public:
  RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel);
  ~RotateEulerRefFrame() noexcept;

  RotateEulerRefFrame(const RotateEulerRefFrame&) = delete;            // Copy Constructor Not Implemented
  RotateEulerRefFrame(RotateEulerRefFrame&&) = delete;                 // Move Constructor Not Implemented
  RotateEulerRefFrame& operator=(const RotateEulerRefFrame&) = delete; // Copy Assignment Not Implemented
  RotateEulerRefFrame& operator=(RotateEulerRefFrame&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

  RotateEulerRefFrameInputValues* inputValues();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  RotateEulerRefFrameInputValues m_InputValues;
};

} // namespace complex
