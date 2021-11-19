#include "MoveData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MoveData::name() const
{
  return FilterTraits<MoveData>::name.str();
}

//------------------------------------------------------------------------------
std::string MoveData::className() const
{
  return FilterTraits<MoveData>::className;
}

//------------------------------------------------------------------------------
Uuid MoveData::uuid() const
{
  return FilterTraits<MoveData>::uuid;
}

//------------------------------------------------------------------------------
std::string MoveData::humanName() const
{
  return "Move Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> MoveData::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters MoveData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_WhatToMove_Key, "Object to Move", "", 0, ChoicesParameter::Choices{"Attribute Matrix", "Attribute Array"}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_AttributeMatrixSource_Key, "Attribute Matrix Source", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerDestination_Key, "Data Container Destination", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_DataArraySource_Key, "Attribute Array Source", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_AttributeMatrixDestination_Key, "Attribute Matrix Destination", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_WhatToMove_Key, k_DataContainerDestination_Key, 0);
  params.linkParameters(k_WhatToMove_Key, k_AttributeMatrixSource_Key, 0);
  params.linkParameters(k_WhatToMove_Key, k_AttributeMatrixDestination_Key, 1);
  params.linkParameters(k_WhatToMove_Key, k_DataArraySource_Key, 1);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MoveData::clone() const
{
  return std::make_unique<MoveData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MoveData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pAttributeMatrixSourceValue = filterArgs.value<DataPath>(k_AttributeMatrixSource_Key);
  auto pDataContainerDestinationValue = filterArgs.value<DataPath>(k_DataContainerDestination_Key);
  auto pDataArraySourceValue = filterArgs.value<DataPath>(k_DataArraySource_Key);
  auto pAttributeMatrixDestinationValue = filterArgs.value<DataPath>(k_AttributeMatrixDestination_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MoveData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWhatToMoveValue = filterArgs.value<ChoicesParameter::ValueType>(k_WhatToMove_Key);
  auto pAttributeMatrixSourceValue = filterArgs.value<DataPath>(k_AttributeMatrixSource_Key);
  auto pDataContainerDestinationValue = filterArgs.value<DataPath>(k_DataContainerDestination_Key);
  auto pDataArraySourceValue = filterArgs.value<DataPath>(k_DataArraySource_Key);
  auto pAttributeMatrixDestinationValue = filterArgs.value<DataPath>(k_AttributeMatrixDestination_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
