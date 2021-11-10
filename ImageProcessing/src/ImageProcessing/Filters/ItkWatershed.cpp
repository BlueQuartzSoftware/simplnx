#include "ItkWatershed.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ItkWatershed::name() const
{
  return FilterTraits<ItkWatershed>::name.str();
}

//------------------------------------------------------------------------------
std::string ItkWatershed::className() const
{
  return FilterTraits<ItkWatershed>::className;
}

//------------------------------------------------------------------------------
Uuid ItkWatershed::uuid() const
{
  return FilterTraits<ItkWatershed>::uuid;
}

//------------------------------------------------------------------------------
std::string ItkWatershed::humanName() const
{
  return "Watershed Filter (ImageProcessing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ItkWatershed::defaultTags() const
{
  return {"#Unsupported", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters ItkWatershed::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Image Data", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_Threshold_Key, "Threshold", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Level_Key, "Level", "", 1.23345f));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ItkWatershed::clone() const
{
  return std::make_unique<ItkWatershed>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ItkWatershed::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pThresholdValue = filterArgs.value<float32>(k_Threshold_Key);
  auto pLevelValue = filterArgs.value<float32>(k_Level_Key);

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
  auto action = std::make_unique<ItkWatershedAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ItkWatershed::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pThresholdValue = filterArgs.value<float32>(k_Threshold_Key);
  auto pLevelValue = filterArgs.value<float32>(k_Level_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
