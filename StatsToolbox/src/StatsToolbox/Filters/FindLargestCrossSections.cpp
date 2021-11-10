#include "FindLargestCrossSections.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindLargestCrossSections::name() const
{
  return FilterTraits<FindLargestCrossSections>::name.str();
}

//------------------------------------------------------------------------------
std::string FindLargestCrossSections::className() const
{
  return FilterTraits<FindLargestCrossSections>::className;
}

//------------------------------------------------------------------------------
Uuid FindLargestCrossSections::uuid() const
{
  return FilterTraits<FindLargestCrossSections>::uuid;
}

//------------------------------------------------------------------------------
std::string FindLargestCrossSections::humanName() const
{
  return "Find Feature Largest Cross-Section Areas";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindLargestCrossSections::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindLargestCrossSections::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane of Interest", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_LargestCrossSectionsArrayPath_Key, "Largest Cross Sections", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindLargestCrossSections::clone() const
{
  return std::make_unique<FindLargestCrossSections>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindLargestCrossSections::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pLargestCrossSectionsArrayPathValue = filterArgs.value<DataPath>(k_LargestCrossSectionsArrayPath_Key);

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
  auto action = std::make_unique<FindLargestCrossSectionsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindLargestCrossSections::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pLargestCrossSectionsArrayPathValue = filterArgs.value<DataPath>(k_LargestCrossSectionsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
