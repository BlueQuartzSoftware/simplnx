#include "ItkSobelEdge.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkSobelEdge::name() const
{
  return FilterTraits<ItkSobelEdge>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkSobelEdge::className() const
{
  return FilterTraits<ItkSobelEdge>::className;
}

//------------------------------------------------------------------------------
Uuid ItkSobelEdge::uuid() const
{
  return FilterTraits<ItkSobelEdge>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkSobelEdge::humanName() const
{
  return "Sobel Edge Detector (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkSobelEdge::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkSobelEdge::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_Slice_Key, "Slice at a Time", "", false));
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
IFilter::UniquePointer ItkSobelEdge::clone() const
{
  return std::make_unique<ItkSobelEdge>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkSobelEdge::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSliceValue = filterArgs.value<bool>(k_Slice_Key);
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkSobelEdgeAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkSobelEdge::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
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
