#include "RobustAutomaticThreshold.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RobustAutomaticThreshold::name() const
{
  return FilterTraits<RobustAutomaticThreshold>::name.str();
}

//------------------------------------------------------------------------------
std::string RobustAutomaticThreshold::className() const
{
  return FilterTraits<RobustAutomaticThreshold>::className;
}

//------------------------------------------------------------------------------
Uuid RobustAutomaticThreshold::uuid() const
{
  return FilterTraits<RobustAutomaticThreshold>::uuid;
}

//------------------------------------------------------------------------------
std::string RobustAutomaticThreshold::humanName() const
{
  return "Robust Automatic Threshold";
}

//------------------------------------------------------------------------------
std::vector<std::string> RobustAutomaticThreshold::defaultTags() const
{
  return {"#DREAM3D Review", "#Threshold"};
}

//------------------------------------------------------------------------------
Parameters RobustAutomaticThreshold::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPath_Key, "Attribute Array to Threshold", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GradientMagnitudeArrayPath_Key, "Gradient Magnitude", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayPath_Key, "Mask", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RobustAutomaticThreshold::clone() const
{
  return std::make_unique<RobustAutomaticThreshold>();
}

//------------------------------------------------------------------------------
Result<OutputActions> RobustAutomaticThreshold::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputArrayPathValue = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  auto pGradientMagnitudeArrayPathValue = filterArgs.value<DataPath>(k_GradientMagnitudeArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RobustAutomaticThresholdAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RobustAutomaticThreshold::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputArrayPathValue = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  auto pGradientMagnitudeArrayPathValue = filterArgs.value<DataPath>(k_GradientMagnitudeArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
