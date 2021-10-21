#include "RemoveFlaggedFeatures.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RemoveFlaggedFeatures::name() const
{
  return FilterTraits<RemoveFlaggedFeatures>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedFeatures::className() const
{
  return FilterTraits<RemoveFlaggedFeatures>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveFlaggedFeatures::uuid() const
{
  return FilterTraits<RemoveFlaggedFeatures>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveFlaggedFeatures::humanName() const
{
  return "Remove Flagged Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveFlaggedFeatures::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters RemoveFlaggedFeatures::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_FillRemovedFeatures_Key, "Fill-in Removed Features", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FlaggedFeaturesArrayPath_Key, "Flagged Features", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveFlaggedFeatures::clone() const
{
  return std::make_unique<RemoveFlaggedFeatures>();
}

//------------------------------------------------------------------------------
Result<OutputActions> RemoveFlaggedFeatures::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFillRemovedFeaturesValue = filterArgs.value<bool>(k_FillRemovedFeatures_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFlaggedFeaturesArrayPathValue = filterArgs.value<DataPath>(k_FlaggedFeaturesArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RemoveFlaggedFeaturesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RemoveFlaggedFeatures::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFillRemovedFeaturesValue = filterArgs.value<bool>(k_FillRemovedFeatures_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFlaggedFeaturesArrayPathValue = filterArgs.value<DataPath>(k_FlaggedFeaturesArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
