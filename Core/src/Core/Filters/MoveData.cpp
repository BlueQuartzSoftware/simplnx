#include "MoveData.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
  params.linkParameters(k_WhatToMove_Key, k_AttributeMatrixSource_Key, 1);
  params.linkParameters(k_WhatToMove_Key, k_AttributeMatrixDestination_Key, 2);
  params.linkParameters(k_WhatToMove_Key, k_DataArraySource_Key, 3);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MoveData::clone() const
{
  return std::make_unique<MoveData>();
}

//------------------------------------------------------------------------------
Result<OutputActions> MoveData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pWhatToMoveValue = filterArgs.value<ChoicesParameter::ValueType>(k_WhatToMove_Key);
  auto pAttributeMatrixSourceValue = filterArgs.value<DataPath>(k_AttributeMatrixSource_Key);
  auto pDataContainerDestinationValue = filterArgs.value<DataPath>(k_DataContainerDestination_Key);
  auto pDataArraySourceValue = filterArgs.value<DataPath>(k_DataArraySource_Key);
  auto pAttributeMatrixDestinationValue = filterArgs.value<DataPath>(k_AttributeMatrixDestination_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<MoveDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> MoveData::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
