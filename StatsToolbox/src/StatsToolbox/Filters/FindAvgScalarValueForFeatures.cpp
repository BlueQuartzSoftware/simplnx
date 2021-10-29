#include "FindAvgScalarValueForFeatures.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindAvgScalarValueForFeatures::name() const
{
  return FilterTraits<FindAvgScalarValueForFeatures>::name.str();
}

//------------------------------------------------------------------------------
std::string FindAvgScalarValueForFeatures::className() const
{
  return FilterTraits<FindAvgScalarValueForFeatures>::className;
}

//------------------------------------------------------------------------------
Uuid FindAvgScalarValueForFeatures::uuid() const
{
  return FilterTraits<FindAvgScalarValueForFeatures>::uuid;
}

//------------------------------------------------------------------------------
std::string FindAvgScalarValueForFeatures::humanName() const
{
  return "Find Average Value of Scalars For Feature";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindAvgScalarValueForFeatures::defaultTags() const
{
  return {"#Statistics", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters FindAvgScalarValueForFeatures::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Average", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewFeatureArrayArrayPath_Key, "Scalar Feature Averages", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindAvgScalarValueForFeatures::clone() const
{
  return std::make_unique<FindAvgScalarValueForFeatures>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindAvgScalarValueForFeatures::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pNewFeatureArrayArrayPathValue = filterArgs.value<DataPath>(k_NewFeatureArrayArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindAvgScalarValueForFeaturesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindAvgScalarValueForFeatures::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pNewFeatureArrayArrayPathValue = filterArgs.value<DataPath>(k_NewFeatureArrayArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
