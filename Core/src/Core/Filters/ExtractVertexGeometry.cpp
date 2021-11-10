#include "ExtractVertexGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExtractVertexGeometry::name() const
{
  return FilterTraits<ExtractVertexGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractVertexGeometry::className() const
{
  return FilterTraits<ExtractVertexGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractVertexGeometry::uuid() const
{
  return FilterTraits<ExtractVertexGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractVertexGeometry::humanName() const
{
  return "Extract Vertex Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractVertexGeometry::defaultTags() const
{
  return {"#Core", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters ExtractVertexGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_ArrayHandling_Key, "Array Handling", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedDataContainerName_Key, "Data Container with Input Geometry", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_IncludedDataArrayPaths_Key, "Included Attribute Arrays", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexDataContainerName_Key, "Vertex Data Container Name", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractVertexGeometry::clone() const
{
  return std::make_unique<ExtractVertexGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractVertexGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pArrayHandlingValue = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedDataContainerNameValue = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  auto pIncludedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IncludedDataArrayPaths_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);

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
  auto action = std::make_unique<ExtractVertexGeometryAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ExtractVertexGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pArrayHandlingValue = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedDataContainerNameValue = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  auto pIncludedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IncludedDataArrayPaths_Key);
  auto pVertexDataContainerNameValue = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
