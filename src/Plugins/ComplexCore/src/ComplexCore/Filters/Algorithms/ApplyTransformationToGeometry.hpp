#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

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

namespace
{

const std::string k_NoTransform("No Transform");
const std::string k_PrecomputedTransformationMatrix("Pre-Computed Transformation Matrix (4x4)");
const std::string k_ManualTransformationMatrix("Manual Transformation Matrix");
const std::string k_Rotation("Rotation");
const std::string k_Translation("Translation");
const std::string k_Scale("Scale");
const complex::ChoicesParameter::Choices k_TransformationChoices = {k_NoTransform, k_PrecomputedTransformationMatrix, k_ManualTransformationMatrix, k_Rotation, k_Translation, k_Scale};

const complex::ChoicesParameter::ValueType k_NoTransformIdx = 0ULL;
const complex::ChoicesParameter::ValueType k_PrecomputedTransformationMatrixIdx = 1ULL;
const complex::ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;
const complex::ChoicesParameter::ValueType k_RotationIdx = 3ULL;
const complex::ChoicesParameter::ValueType k_TranslationIdx = 4ULL;
const complex::ChoicesParameter::ValueType k_ScaleIdx = 5ULL;

const std::string k_NearestNeighborInterpolation("Nearest Neighbor");
const std::string k_LinearInterpolation("Linear Interpolation");
const complex::ChoicesParameter::Choices k_InterpolationChoices = {k_NearestNeighborInterpolation, k_LinearInterpolation};

const complex::ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const complex::ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;
} // namespace

namespace complex
{

struct COMPLEXCORE_EXPORT ApplyTransformationToGeometryInputValues
{
  DataPath SelectedGeometryPath;

  ChoicesParameter::ValueType TransformationSelection;
  ChoicesParameter::ValueType InterpolationSelection;

  complex::ImageRotationUtilities::Matrix4fR TransformationMatrix;
  DataPath ComputedTransformationMatrix;

  VectorFloat32Parameter::ValueType RotationAxis;
  VectorFloat32Parameter::ValueType Translation;
  VectorFloat32Parameter::ValueType Scale;
  DataPath CellAttributeMatrixPath;
  bool UseDataArraySelection;
  MultiArraySelectionParameter::ValueType DataArraySelection;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ApplyTransformationToGeometry
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
