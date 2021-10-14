#include "ItkWatershed.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
std::string ItkWatershed::name() const
{
  return FilterTraits<ItkWatershed>::name.str();
}

Uuid ItkWatershed::uuid() const
{
  return FilterTraits<ItkWatershed>::uuid;
}

std::string ItkWatershed::humanName() const
{
  return "Watershed Filter (ImageProcessing)";
}

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

IFilter::UniquePointer ItkWatershed::clone() const
{
  return std::make_unique<ItkWatershed>();
}

Result<OutputActions> ItkWatershed::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  auto pThresholdValue = filterArgs.value<float32>(k_Threshold_Key);
  auto pLevelValue = filterArgs.value<float32>(k_Level_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ItkWatershedAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ItkWatershed::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
