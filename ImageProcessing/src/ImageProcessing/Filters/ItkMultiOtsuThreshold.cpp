#include "ItkMultiOtsuThreshold.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkMultiOtsuThreshold::name() const
{
  return FilterTraits<ItkMultiOtsuThreshold>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkMultiOtsuThreshold::className() const
{
  return FilterTraits<ItkMultiOtsuThreshold>::className;
}

//------------------------------------------------------------------------------
Uuid ItkMultiOtsuThreshold::uuid() const
{
  return FilterTraits<ItkMultiOtsuThreshold>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkMultiOtsuThreshold::humanName() const
{
  return "Multi Level Otsu Threshold (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkMultiOtsuThreshold::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkMultiOtsuThreshold::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_Slice_Key, "Slice at a Time", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_Levels_Key, "Number of Levels", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewArray_Key, "Save as New Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Process", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Output Attribute Array", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveAsNewArray_Key, k_NewCellArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkMultiOtsuThreshold::clone() const
{
  return std::make_unique<ItkMultiOtsuThreshold>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkMultiOtsuThreshold::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSliceValue = filterArgs.value<bool>(k_Slice_Key);
  auto pLevelsValue = filterArgs.value<int32>(k_Levels_Key);
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkMultiOtsuThresholdAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkMultiOtsuThreshold::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSliceValue = filterArgs.value<bool>(k_Slice_Key);
  auto pLevelsValue = filterArgs.value<int32>(k_Levels_Key);
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
