#include "MoveMultiData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/MultiAttributeMatrixSelectionFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MoveMultiData::name() const
{
  return FilterTraits<MoveMultiData>::name.str();
}

//------------------------------------------------------------------------------
std::string MoveMultiData::className() const
{
  return FilterTraits<MoveMultiData>::className;
}

//------------------------------------------------------------------------------
Uuid MoveMultiData::uuid() const
{
  return FilterTraits<MoveMultiData>::uuid;
}

//------------------------------------------------------------------------------
std::string MoveMultiData::humanName() const
{
  return "Move Multi Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> MoveMultiData::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters MoveMultiData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_WhatToMove_Key, "Object to Move", "", 0, ChoicesParameter::Choices{"Attribute Matrix", "Attribute Array"}));
  /*[x]*/ params.insert(std::make_unique<MultiAttributeMatrixSelectionFilterParameter>(k_AttributeMatrixSources_Key, "Attribute Matrix Sources", "", {}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerDestination_Key, "Data Container Destination", "", DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_DataArraySources_Key, "Attribute Array Sources", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_AttributeMatrixDestination_Key, "Attribute Matrix Destination", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_WhatToMove_Key, k_DataContainerDestination_Key, 0);
  params.linkParameters(k_WhatToMove_Key, k_AttributeMatrixSources_Key, 0);
  params.linkParameters(k_WhatToMove_Key, k_AttributeMatrixDestination_Key, 1);
  params.linkParameters(k_WhatToMove_Key, k_DataArraySources_Key, 1);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MoveMultiData::clone() const
{
  return std::make_unique<MoveMultiData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MoveMultiData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pWhatToMoveValue = filterArgs.value<ChoicesParameter::ValueType>(k_WhatToMove_Key);
  auto pAttributeMatrixSourcesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_AttributeMatrixSources_Key);
  auto pDataContainerDestinationValue = filterArgs.value<DataPath>(k_DataContainerDestination_Key);
  auto pDataArraySourcesValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySources_Key);
  auto pAttributeMatrixDestinationValue = filterArgs.value<DataPath>(k_AttributeMatrixDestination_Key);

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
  auto action = std::make_unique<MoveMultiDataAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> MoveMultiData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWhatToMoveValue = filterArgs.value<ChoicesParameter::ValueType>(k_WhatToMove_Key);
  auto pAttributeMatrixSourcesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_AttributeMatrixSources_Key);
  auto pDataContainerDestinationValue = filterArgs.value<DataPath>(k_DataContainerDestination_Key);
  auto pDataArraySourcesValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySources_Key);
  auto pAttributeMatrixDestinationValue = filterArgs.value<DataPath>(k_AttributeMatrixDestination_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
