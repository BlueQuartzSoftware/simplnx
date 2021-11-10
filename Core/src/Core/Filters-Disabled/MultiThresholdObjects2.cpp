#include "MultiThresholdObjects2.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ComparisonSelectionAdvancedFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MultiThresholdObjects2::name() const
{
  return FilterTraits<MultiThresholdObjects2>::name.str();
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjects2::className() const
{
  return FilterTraits<MultiThresholdObjects2>::className;
}

//------------------------------------------------------------------------------
Uuid MultiThresholdObjects2::uuid() const
{
  return FilterTraits<MultiThresholdObjects2>::uuid;
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjects2::humanName() const
{
  return "Threshold Objects (Advanced)";
}

//------------------------------------------------------------------------------
std::vector<std::string> MultiThresholdObjects2::defaultTags() const
{
  return {"#Processing", "#Threshold"};
}

//------------------------------------------------------------------------------
Parameters MultiThresholdObjects2::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<ComparisonSelectionAdvancedFilterParameter>(k_SelectedThresholds_Key, "Select Arrays to Threshold", "", {}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DestinationArrayName_Key, "Output Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MultiThresholdObjects2::clone() const
{
  return std::make_unique<MultiThresholdObjects2>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MultiThresholdObjects2::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedThresholdsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  auto pDestinationArrayNameValue = filterArgs.value<DataPath>(k_DestinationArrayName_Key);

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
  auto action = std::make_unique<MultiThresholdObjects2Action>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> MultiThresholdObjects2::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedThresholdsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  auto pDestinationArrayNameValue = filterArgs.value<DataPath>(k_DestinationArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
