#include "RotateSampleRefFrame.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RotateSampleRefFrame::name() const
{
  return FilterTraits<RotateSampleRefFrame>::name.str();
}

//------------------------------------------------------------------------------
std::string RotateSampleRefFrame::className() const
{
  return FilterTraits<RotateSampleRefFrame>::className;
}

//------------------------------------------------------------------------------
Uuid RotateSampleRefFrame::uuid() const
{
  return FilterTraits<RotateSampleRefFrame>::uuid;
}

//------------------------------------------------------------------------------
std::string RotateSampleRefFrame::humanName() const
{
  return "Rotate Sample Reference Frame";
}

//------------------------------------------------------------------------------
std::vector<std::string> RotateSampleRefFrame::defaultTags() const
{
  return {"#Sampling", "#Rotating/Transforming"};
}

//------------------------------------------------------------------------------
Parameters RotateSampleRefFrame::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_RotationRepresentationChoice_Key, "Rotation Representation", "", 0, ChoicesParameter::Choices{"Axis Angle", "Rotation Matrix"}));
  params.insert(std::make_unique<Float32Parameter>(k_RotationAngle_Key, "Rotation Angle (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxis_Key, "Rotation Axis (ijk)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  /*[x]*/ params.insert(std::make_unique<DynamicTableFilterParameter>(k_RotationTable_Key, "Rotation Matrix", "", {}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_RotationRepresentationChoice_Key, k_RotationAngle_Key, 0);
  params.linkParameters(k_RotationRepresentationChoice_Key, k_RotationAxis_Key, 1);
  params.linkParameters(k_RotationRepresentationChoice_Key, k_RotationTable_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RotateSampleRefFrame::clone() const
{
  return std::make_unique<RotateSampleRefFrame>();
}

//------------------------------------------------------------------------------
Result<OutputActions> RotateSampleRefFrame::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRotationRepresentationChoiceValue = filterArgs.value<ChoicesParameter::ValueType>(k_RotationRepresentationChoice_Key);
  auto pRotationAngleValue = filterArgs.value<float32>(k_RotationAngle_Key);
  auto pRotationAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  auto pRotationTableValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RotationTable_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RotateSampleRefFrameAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RotateSampleRefFrame::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRotationRepresentationChoiceValue = filterArgs.value<ChoicesParameter::ValueType>(k_RotationRepresentationChoice_Key);
  auto pRotationAngleValue = filterArgs.value<float32>(k_RotationAngle_Key);
  auto pRotationAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  auto pRotationTableValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RotationTable_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
