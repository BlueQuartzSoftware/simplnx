#include "ComputeUmeyamaTransform.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ComputeUmeyamaTransform::name() const
{
  return FilterTraits<ComputeUmeyamaTransform>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeUmeyamaTransform::className() const
{
  return FilterTraits<ComputeUmeyamaTransform>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeUmeyamaTransform::uuid() const
{
  return FilterTraits<ComputeUmeyamaTransform>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeUmeyamaTransform::humanName() const
{
  return "Compute Umeyama Transform";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeUmeyamaTransform::defaultTags() const
{
  return {"#DREAM3D Review", "#Registration"};
}

//------------------------------------------------------------------------------
Parameters ComputeUmeyamaTransform::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_UseScaling_Key, "Use Scaling", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SourcePointSet_Key, "Moving Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DestPointSet_Key, "Fixed Geometry", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Transformation Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformationAttributeMatrixName_Key, "Transformation Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformationMatrixName_Key, "Transformation Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeUmeyamaTransform::clone() const
{
  return std::make_unique<ComputeUmeyamaTransform>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeUmeyamaTransform::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pUseScalingValue = filterArgs.value<bool>(k_UseScaling_Key);
  auto pSourcePointSetValue = filterArgs.value<DataPath>(k_SourcePointSet_Key);
  auto pDestPointSetValue = filterArgs.value<DataPath>(k_DestPointSet_Key);
  auto pTransformationAttributeMatrixNameValue = filterArgs.value<DataPath>(k_TransformationAttributeMatrixName_Key);
  auto pTransformationMatrixNameValue = filterArgs.value<DataPath>(k_TransformationMatrixName_Key);

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
  auto action = std::make_unique<ComputeUmeyamaTransformAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ComputeUmeyamaTransform::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseScalingValue = filterArgs.value<bool>(k_UseScaling_Key);
  auto pSourcePointSetValue = filterArgs.value<DataPath>(k_SourcePointSet_Key);
  auto pDestPointSetValue = filterArgs.value<DataPath>(k_DestPointSet_Key);
  auto pTransformationAttributeMatrixNameValue = filterArgs.value<DataPath>(k_TransformationAttributeMatrixName_Key);
  auto pTransformationMatrixNameValue = filterArgs.value<DataPath>(k_TransformationMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
