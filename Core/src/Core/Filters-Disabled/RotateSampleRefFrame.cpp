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
  params.linkParameters(k_RotationRepresentationChoice_Key, k_RotationAxis_Key, 0);
  params.linkParameters(k_RotationRepresentationChoice_Key, k_RotationTable_Key, 1);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RotateSampleRefFrame::clone() const
{
  return std::make_unique<RotateSampleRefFrame>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RotateSampleRefFrame::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pRotationRepresentationChoiceValue = filterArgs.value<ChoicesParameter::ValueType>(k_RotationRepresentationChoice_Key);
  auto pRotationAngleValue = filterArgs.value<float32>(k_RotationAngle_Key);
  auto pRotationAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  auto pRotationTableValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RotationTable_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RotateSampleRefFrameAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> RotateSampleRefFrame::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
