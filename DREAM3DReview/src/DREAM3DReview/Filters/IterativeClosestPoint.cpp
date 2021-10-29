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
Result<OutputActions> IterativeClosestPoint::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMovingVertexGeometryValue = filterArgs.value<DataPath>(k_MovingVertexGeometry_Key);
  auto pTargetVertexGeometryValue = filterArgs.value<DataPath>(k_TargetVertexGeometry_Key);
  auto pIterationsValue = filterArgs.value<int32>(k_Iterations_Key);
  auto pApplyTransformValue = filterArgs.value<bool>(k_ApplyTransform_Key);
  auto pTransformAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformAttributeMatrixName_Key);
  auto pTransformArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<IterativeClosestPointAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
