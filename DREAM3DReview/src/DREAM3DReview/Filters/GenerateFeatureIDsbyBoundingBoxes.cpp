#include "GenerateFeatureIDsbyBoundingBoxes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
IFilter::PreflightResult GenerateFeatureIDsbyBoundingBoxes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureIDsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIDsArrayPath_Key);
  auto pFeatureAttributeMatrixArrayPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixArrayPath_Key);
  auto pBoxCenterArrayPathValue = filterArgs.value<DataPath>(k_BoxCenterArrayPath_Key);
  auto pBoxDimensionsArrayPathValue = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  auto pBoxFeatureIDsArrayPathValue = filterArgs.value<DataPath>(k_BoxFeatureIDsArrayPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateFeatureIDsbyBoundingBoxes::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
