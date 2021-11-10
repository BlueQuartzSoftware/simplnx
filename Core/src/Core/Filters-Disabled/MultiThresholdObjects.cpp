#include "MultiThresholdObjects.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ComparisonSelectionFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MultiThresholdObjects::name() const
{
  return FilterTraits<MultiThresholdObjects>::name.str();
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjects::className() const
{
  return FilterTraits<MultiThresholdObjects>::className;
}

//------------------------------------------------------------------------------
Uuid MultiThresholdObjects::uuid() const
{
  return FilterTraits<MultiThresholdObjects>::uuid;
}

//------------------------------------------------------------------------------
std::string MultiThresholdObjects::humanName() const
{
  return "Threshold Objects";
}

//------------------------------------------------------------------------------
std::vector<std::string> MultiThresholdObjects::defaultTags() const
{
  return {"#Processing", "#Threshold"};
}

//------------------------------------------------------------------------------
Parameters MultiThresholdObjects::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<ComparisonSelectionFilterParameter>(k_SelectedThresholds_Key, "Select Arrays to Threshold", "", {}));
  params.insert(std::make_unique<StringParameter>(k_DestinationArrayName_Key, "Output Attribute Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MultiThresholdObjects::clone() const
{
  return std::make_unique<MultiThresholdObjects>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MultiThresholdObjects::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pDestinationArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_DestinationArrayName_Key);

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
  auto action = std::make_unique<MultiThresholdObjectsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> MultiThresholdObjects::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedThresholdsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  auto pDestinationArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_DestinationArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
