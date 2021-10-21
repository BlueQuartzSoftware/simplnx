#include "GenerateFeatureIDsbyBoundingBoxes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateFeatureIDsbyBoundingBoxes::name() const
{
  return FilterTraits<GenerateFeatureIDsbyBoundingBoxes>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateFeatureIDsbyBoundingBoxes::className() const
{
  return FilterTraits<GenerateFeatureIDsbyBoundingBoxes>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateFeatureIDsbyBoundingBoxes::uuid() const
{
  return FilterTraits<GenerateFeatureIDsbyBoundingBoxes>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateFeatureIDsbyBoundingBoxes::humanName() const
{
  return "Generate FeatureIDs by Bounding Boxes";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateFeatureIDsbyBoundingBoxes::defaultTags() const
{
  return {"#Unsupported", "#DREAM3DReview"};
}

//------------------------------------------------------------------------------
Parameters GenerateFeatureIDsbyBoundingBoxes::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIDsArrayPath_Key, "Feature IDs", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_FeatureAttributeMatrixArrayPath_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoxCenterArrayPath_Key, "Box Corner", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoxDimensionsArrayPath_Key, "Box Dimensions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoxFeatureIDsArrayPath_Key, "Box Feature IDs Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateFeatureIDsbyBoundingBoxes::clone() const
{
  return std::make_unique<GenerateFeatureIDsbyBoundingBoxes>();
}

//------------------------------------------------------------------------------
Result<OutputActions> GenerateFeatureIDsbyBoundingBoxes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIDsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIDsArrayPath_Key);
  auto pFeatureAttributeMatrixArrayPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixArrayPath_Key);
  auto pBoxCenterArrayPathValue = filterArgs.value<DataPath>(k_BoxCenterArrayPath_Key);
  auto pBoxDimensionsArrayPathValue = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  auto pBoxFeatureIDsArrayPathValue = filterArgs.value<DataPath>(k_BoxFeatureIDsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateFeatureIDsbyBoundingBoxesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> GenerateFeatureIDsbyBoundingBoxes::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIDsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIDsArrayPath_Key);
  auto pFeatureAttributeMatrixArrayPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixArrayPath_Key);
  auto pBoxCenterArrayPathValue = filterArgs.value<DataPath>(k_BoxCenterArrayPath_Key);
  auto pBoxDimensionsArrayPathValue = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  auto pBoxFeatureIDsArrayPathValue = filterArgs.value<DataPath>(k_BoxFeatureIDsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
