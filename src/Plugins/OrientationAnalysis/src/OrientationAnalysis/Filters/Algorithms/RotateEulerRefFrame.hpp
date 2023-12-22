#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @brief The RotateEulerRefFrameInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrameInputValues
{
  std::vector<float> rotationAxis;
  DataPath eulerAngleDataPath;
};

/**
 * @brief The RotateEulerRefFrame class
 */
class ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrame
{
public:
  RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues);
  ~RotateEulerRefFrame() noexcept;

  RotateEulerRefFrame(const RotateEulerRefFrame&) = delete;            // Copy Constructor Not Implemented
  RotateEulerRefFrame(RotateEulerRefFrame&&) = delete;                 // Move Constructor Not Implemented
  RotateEulerRefFrame& operator=(const RotateEulerRefFrame&) = delete; // Copy Assignment Not Implemented
  RotateEulerRefFrame& operator=(RotateEulerRefFrame&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const RotateEulerRefFrameInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
