#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RotateSampleRefFrameInputValues inputValues;

  inputValues.RotationRepresentationChoice = filterArgs.value<ChoicesParameter::ValueType>(k_RotationRepresentationChoice_Key);
  inputValues.RotationAngle = filterArgs.value<float32>(k_RotationAngle_Key);
  inputValues.RotationAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  inputValues.RotationTable = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RotationTable_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  return RotateSampleRefFrame(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RotateSampleRefFrameInputValues
{
  ChoicesParameter::ValueType RotationRepresentationChoice;
  float32 RotationAngle;
  VectorFloat32Parameter::ValueType RotationAxis;
  <<<NOT_IMPLEMENTED>>> RotationTable;
  /*[x]*/ DataPath CellAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RotateSampleRefFrame
{
public:
  RotateSampleRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateSampleRefFrameInputValues* inputValues);
  ~RotateSampleRefFrame() noexcept;

  RotateSampleRefFrame(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame(RotateSampleRefFrame&&) noexcept = delete;
  RotateSampleRefFrame& operator=(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame& operator=(RotateSampleRefFrame&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RotateSampleRefFrameInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
