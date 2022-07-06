#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ApplyTransformationToGeometryInputValues inputValues;

  inputValues.TransformationMatrixType = filterArgs.value<ChoicesParameter::ValueType>(k_TransformationMatrixType_Key);
  inputValues.InterpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  inputValues.UseDataArraySelection = filterArgs.value<bool>(k_UseDataArraySelection_Key);
  inputValues.ManualTransformationMatrix = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ManualTransformationMatrix_Key);
  inputValues.RotationAngle = filterArgs.value<float32>(k_RotationAngle_Key);
  inputValues.RotationAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  inputValues.Translation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  inputValues.Scale = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  inputValues.ComputedTransformationMatrix = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  inputValues.DataArraySelection = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySelection_Key);

  return ApplyTransformationToGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ApplyTransformationToGeometryInputValues
{
  ChoicesParameter::ValueType TransformationMatrixType;
  ChoicesParameter::ValueType InterpolationType;
  bool UseDataArraySelection;
  <<<NOT_IMPLEMENTED>>> ManualTransformationMatrix;
  /*[x]*/ float32 RotationAngle;
  VectorFloat32Parameter::ValueType RotationAxis;
  VectorFloat32Parameter::ValueType Translation;
  VectorFloat32Parameter::ValueType Scale;
  DataPath ComputedTransformationMatrix;
  DataPath CellAttributeMatrixPath;
  MultiArraySelectionParameter::ValueType DataArraySelection;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ApplyTransformationToGeometry
{
public:
  ApplyTransformationToGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ApplyTransformationToGeometryInputValues* inputValues);
  ~ApplyTransformationToGeometry() noexcept;

  ApplyTransformationToGeometry(const ApplyTransformationToGeometry&) = delete;
  ApplyTransformationToGeometry(ApplyTransformationToGeometry&&) noexcept = delete;
  ApplyTransformationToGeometry& operator=(const ApplyTransformationToGeometry&) = delete;
  ApplyTransformationToGeometry& operator=(ApplyTransformationToGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ApplyTransformationToGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
