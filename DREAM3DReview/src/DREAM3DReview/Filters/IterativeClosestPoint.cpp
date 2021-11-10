#include "IterativeClosestPoint.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string IterativeClosestPoint::name() const
{
  return FilterTraits<IterativeClosestPoint>::name.str();
}

//------------------------------------------------------------------------------
std::string IterativeClosestPoint::className() const
{
  return FilterTraits<IterativeClosestPoint>::className;
}

//------------------------------------------------------------------------------
Uuid IterativeClosestPoint::uuid() const
{
  return FilterTraits<IterativeClosestPoint>::uuid;
}

//------------------------------------------------------------------------------
std::string IterativeClosestPoint::humanName() const
{
  return "Iterative Closest Point";
}

//------------------------------------------------------------------------------
std::vector<std::string> IterativeClosestPoint::defaultTags() const
{
  return {"#Reconstruction", "#Alignment"};
}

//------------------------------------------------------------------------------
Parameters IterativeClosestPoint::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_MovingVertexGeometry_Key, "Moving Vertex Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TargetVertexGeometry_Key, "Target Vertex Geometry", "", DataPath{}));
  params.insert(std::make_unique<Int32Parameter>(k_Iterations_Key, "Number of Iterations", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_ApplyTransform_Key, "Apply Transform to Moving Geometry", "", false));
  params.insert(std::make_unique<StringParameter>(k_TransformAttributeMatrixName_Key, "Transform Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_TransformArrayName_Key, "Transform Array Name", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IterativeClosestPoint::clone() const
{
  return std::make_unique<IterativeClosestPoint>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult IterativeClosestPoint::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMovingVertexGeometryValue = filterArgs.value<DataPath>(k_MovingVertexGeometry_Key);
  auto pTargetVertexGeometryValue = filterArgs.value<DataPath>(k_TargetVertexGeometry_Key);
  auto pIterationsValue = filterArgs.value<int32>(k_Iterations_Key);
  auto pApplyTransformValue = filterArgs.value<bool>(k_ApplyTransform_Key);
  auto pTransformAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformAttributeMatrixName_Key);
  auto pTransformArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformArrayName_Key);

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
  auto action = std::make_unique<IterativeClosestPointAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> IterativeClosestPoint::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMovingVertexGeometryValue = filterArgs.value<DataPath>(k_MovingVertexGeometry_Key);
  auto pTargetVertexGeometryValue = filterArgs.value<DataPath>(k_TargetVertexGeometry_Key);
  auto pIterationsValue = filterArgs.value<int32>(k_Iterations_Key);
  auto pApplyTransformValue = filterArgs.value<bool>(k_ApplyTransform_Key);
  auto pTransformAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformAttributeMatrixName_Key);
  auto pTransformArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
