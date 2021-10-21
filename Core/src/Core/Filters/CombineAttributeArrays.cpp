#include "CombineAttributeArrays.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CombineAttributeArrays::name() const
{
  return FilterTraits<CombineAttributeArrays>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineAttributeArrays::className() const
{
  return FilterTraits<CombineAttributeArrays>::className;
}

//------------------------------------------------------------------------------
Uuid CombineAttributeArrays::uuid() const
{
  return FilterTraits<CombineAttributeArrays>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineAttributeArrays::humanName() const
{
  return "Combine Attribute Arrays";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineAttributeArrays::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CombineAttributeArrays::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_NormalizeData_Key, "Normalize Data", "", false));
  params.insert(std::make_unique<BoolParameter>(k_MoveValues_Key, "Move Data", "", false));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Combine", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<StringParameter>(k_StackedDataArrayName_Key, "Combined Data", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineAttributeArrays::clone() const
{
  return std::make_unique<CombineAttributeArrays>();
}

//------------------------------------------------------------------------------
Result<OutputActions> CombineAttributeArrays::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pNormalizeDataValue = filterArgs.value<bool>(k_NormalizeData_Key);
  auto pMoveValuesValue = filterArgs.value<bool>(k_MoveValues_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pStackedDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_StackedDataArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CombineAttributeArraysAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CombineAttributeArrays::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNormalizeDataValue = filterArgs.value<bool>(k_NormalizeData_Key);
  auto pMoveValuesValue = filterArgs.value<bool>(k_MoveValues_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pStackedDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_StackedDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
