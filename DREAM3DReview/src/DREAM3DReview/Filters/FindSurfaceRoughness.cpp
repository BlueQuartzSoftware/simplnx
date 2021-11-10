#include "FindSurfaceRoughness.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSurfaceRoughness::name() const
{
  return FilterTraits<FindSurfaceRoughness>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSurfaceRoughness::className() const
{
  return FilterTraits<FindSurfaceRoughness>::className;
}

//------------------------------------------------------------------------------
Uuid FindSurfaceRoughness::uuid() const
{
  return FilterTraits<FindSurfaceRoughness>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSurfaceRoughness::humanName() const
{
  return "Find Surface Roughness";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSurfaceRoughness::defaultTags() const
{
  return {"#Statistics", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters FindSurfaceRoughness::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoundaryCellsArrayPath_Key, "Boundary Cells", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_AttributeMatrixName_Key, "Roughness Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_RoughnessParamsArrayName_Key, "Roughness Parameters", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSurfaceRoughness::clone() const
{
  return std::make_unique<FindSurfaceRoughness>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSurfaceRoughness::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pBoundaryCellsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  auto pRoughnessParamsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_RoughnessParamsArrayName_Key);

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
  auto action = std::make_unique<FindSurfaceRoughnessAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindSurfaceRoughness::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pBoundaryCellsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  auto pRoughnessParamsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_RoughnessParamsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
