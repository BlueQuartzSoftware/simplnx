#include "ItkAutoThreshold.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkAutoThreshold::name() const
{
  return FilterTraits<ItkAutoThreshold>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkAutoThreshold::className() const
{
  return FilterTraits<ItkAutoThreshold>::className;
}

//------------------------------------------------------------------------------
Uuid ItkAutoThreshold::uuid() const
{
  return FilterTraits<ItkAutoThreshold>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkAutoThreshold::humanName() const
{
  return "Threshold Image (Auto) (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkAutoThreshold::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkAutoThreshold::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Method_Key, "Threshold Method", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<BoolParameter>(k_Slice_Key, "Slice at a Time", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewArray_Key, "Save as New Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Process", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Threshold Array", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveAsNewArray_Key, k_NewCellArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkAutoThreshold::clone() const
{
  return std::make_unique<ItkAutoThreshold>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkAutoThreshold::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMethodValue = filterArgs.value<ChoicesParameter::ValueType>(k_Method_Key);
  auto pSliceValue = filterArgs.value<bool>(k_Slice_Key);
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkAutoThresholdAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkAutoThreshold::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMethodValue = filterArgs.value<ChoicesParameter::ValueType>(k_Method_Key);
  auto pSliceValue = filterArgs.value<bool>(k_Slice_Key);
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
