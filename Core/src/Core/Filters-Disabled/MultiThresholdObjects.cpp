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
Result<OutputActions> MultiThresholdObjects::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedThresholdsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedThresholds_Key);
  auto pDestinationArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_DestinationArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<MultiThresholdObjectsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
