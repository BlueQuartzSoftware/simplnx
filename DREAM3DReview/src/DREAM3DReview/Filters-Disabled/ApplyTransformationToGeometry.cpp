#include "ApplyTransformationToGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometry::name() const
{
  return FilterTraits<ApplyTransformationToGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometry::className() const
{
  return FilterTraits<ApplyTransformationToGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid ApplyTransformationToGeometry::uuid() const
{
  return FilterTraits<ApplyTransformationToGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometry::humanName() const
{
  return "Apply Transformation to Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ApplyTransformationToGeometry::defaultTags() const
{
  return {"#DREAM3D Review", "#Rotation/Transforming"};
}

//------------------------------------------------------------------------------
Parameters ApplyTransformationToGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_TransformationMatrixType_Key, "Transformation Type", "", 0,
                                         ChoicesParameter::Choices{"No Transformation", "Pre-Computed Transformation Matrix", "Manual Transformation Matrix", "Rotation", "Translation", "Scale"}));
  /*[x]*/ params.insert(std::make_unique<DynamicTableFilterParameter>(k_ManualTransformationMatrix_Key, "Transformation Matrix", "", {}));
  params.insert(std::make_unique<Float32Parameter>(k_RotationAngle_Key, "Rotation Angle (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxis_Key, "Rotation Axis (ijk)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Translation_Key, "Translation", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Scale_Key, "Scale", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_GeometryToTransform_Key, "Geometry to Transform", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Transformation Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_TransformationMatrixType_Key, k_ComputedTransformationMatrix_Key, 0);
  params.linkParameters(k_TransformationMatrixType_Key, k_ManualTransformationMatrix_Key, 1);
  params.linkParameters(k_TransformationMatrixType_Key, k_RotationAngle_Key, 2);
  params.linkParameters(k_TransformationMatrixType_Key, k_RotationAxis_Key, 3);
  params.linkParameters(k_TransformationMatrixType_Key, k_Translation_Key, 4);
  params.linkParameters(k_TransformationMatrixType_Key, k_Scale_Key, 5);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApplyTransformationToGeometry::clone() const
{
  return std::make_unique<ApplyTransformationToGeometry>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ApplyTransformationToGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pTransformationMatrixTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_TransformationMatrixType_Key);
  auto pManualTransformationMatrixValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ManualTransformationMatrix_Key);
  auto pRotationAngleValue = filterArgs.value<float32>(k_RotationAngle_Key);
  auto pRotationAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  auto pGeometryToTransformValue = filterArgs.value<DataPath>(k_GeometryToTransform_Key);
  auto pComputedTransformationMatrixValue = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ApplyTransformationToGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ApplyTransformationToGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pTransformationMatrixTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_TransformationMatrixType_Key);
  auto pManualTransformationMatrixValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ManualTransformationMatrix_Key);
  auto pRotationAngleValue = filterArgs.value<float32>(k_RotationAngle_Key);
  auto pRotationAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  auto pGeometryToTransformValue = filterArgs.value<DataPath>(k_GeometryToTransform_Key);
  auto pComputedTransformationMatrixValue = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
