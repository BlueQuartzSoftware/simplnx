#include "ItkHoughCircles.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkHoughCircles::name() const
{
  return FilterTraits<ItkHoughCircles>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkHoughCircles::className() const
{
  return FilterTraits<ItkHoughCircles>::className;
}

//------------------------------------------------------------------------------
Uuid ItkHoughCircles::uuid() const
{
  return FilterTraits<ItkHoughCircles>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkHoughCircles::humanName() const
{
  return "Hough Circle Detection (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkHoughCircles::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkHoughCircles::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewArray_Key, "Save as New Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Process", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Output Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_MinRadius_Key, "Minimum Radius", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_MaxRadius_Key, "Maximum Radius", "", 1.23345f));
  params.insert(std::make_unique<Int32Parameter>(k_NumberCircles_Key, "Number of Circles", "", 1234356));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveAsNewArray_Key, k_NewCellArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkHoughCircles::clone() const
{
  return std::make_unique<ItkHoughCircles>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ItkHoughCircles::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);
  auto pMinRadiusValue = filterArgs.value<float32>(k_MinRadius_Key);
  auto pMaxRadiusValue = filterArgs.value<float32>(k_MaxRadius_Key);
  auto pNumberCirclesValue = filterArgs.value<int32>(k_NumberCircles_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkHoughCirclesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ItkHoughCircles::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSaveAsNewArrayValue = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<DataPath>(k_NewCellArrayName_Key);
  auto pMinRadiusValue = filterArgs.value<float32>(k_MinRadius_Key);
  auto pMaxRadiusValue = filterArgs.value<float32>(k_MaxRadius_Key);
  auto pNumberCirclesValue = filterArgs.value<int32>(k_NumberCircles_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
