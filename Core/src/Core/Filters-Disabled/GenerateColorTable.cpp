#include "GenerateColorTable.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GenerateColorTableFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateColorTable::name() const
{
  return FilterTraits<GenerateColorTable>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateColorTable::className() const
{
  return FilterTraits<GenerateColorTable>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateColorTable::uuid() const
{
  return FilterTraits<GenerateColorTable>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateColorTable::humanName() const
{
  return "Generate Color Table";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateColorTable::defaultTags() const
{
  return {"#Core", "#Image"};
}

//------------------------------------------------------------------------------
Parameters GenerateColorTable::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<GenerateColorTableFilterParameter>(k_SelectedPresetName_Key, "Select Preset...", "", {}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedDataArrayPath_Key, "Data Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_RgbArrayName_Key, "RGB Array Name", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateColorTable::clone() const
{
  return std::make_unique<GenerateColorTable>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateColorTable::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedPresetNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedPresetName_Key);
  auto pSelectedDataArrayPathValue = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  auto pRgbArrayNameValue = filterArgs.value<DataPath>(k_RgbArrayName_Key);

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
  auto action = std::make_unique<GenerateColorTableAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> GenerateColorTable::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedPresetNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SelectedPresetName_Key);
  auto pSelectedDataArrayPathValue = filterArgs.value<DataPath>(k_SelectedDataArrayPath_Key);
  auto pRgbArrayNameValue = filterArgs.value<DataPath>(k_RgbArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
