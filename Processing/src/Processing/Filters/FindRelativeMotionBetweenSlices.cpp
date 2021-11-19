#include "FindRelativeMotionBetweenSlices.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindRelativeMotionBetweenSlices::name() const
{
  return FilterTraits<FindRelativeMotionBetweenSlices>::name.str();
}

//------------------------------------------------------------------------------
std::string FindRelativeMotionBetweenSlices::className() const
{
  return FilterTraits<FindRelativeMotionBetweenSlices>::className;
}

//------------------------------------------------------------------------------
Uuid FindRelativeMotionBetweenSlices::uuid() const
{
  return FilterTraits<FindRelativeMotionBetweenSlices>::uuid;
}

//------------------------------------------------------------------------------
std::string FindRelativeMotionBetweenSlices::humanName() const
{
  return "Find Relative Motion Between Slices";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindRelativeMotionBetweenSlices::defaultTags() const
{
  return {"#Processing", "#Image"};
}

//------------------------------------------------------------------------------
Parameters FindRelativeMotionBetweenSlices::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane of Interest", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Int32Parameter>(k_PSize1_Key, "Patch Size 1", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_PSize2_Key, "Patch Size 2", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_SSize1_Key, "Search Distance 1", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_SSize2_Key, "Search Distance 2", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_SliceStep_Key, "Slice Step", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Track Motion", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_MotionDirectionArrayName_Key, "Motion Direction", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindRelativeMotionBetweenSlices::clone() const
{
  return std::make_unique<FindRelativeMotionBetweenSlices>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindRelativeMotionBetweenSlices::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pPSize1Value = filterArgs.value<int32>(k_PSize1_Key);
  auto pPSize2Value = filterArgs.value<int32>(k_PSize2_Key);
  auto pSSize1Value = filterArgs.value<int32>(k_SSize1_Key);
  auto pSSize2Value = filterArgs.value<int32>(k_SSize2_Key);
  auto pSliceStepValue = filterArgs.value<int32>(k_SliceStep_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMotionDirectionArrayNameValue = filterArgs.value<DataPath>(k_MotionDirectionArrayName_Key);

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
Result<> FindRelativeMotionBetweenSlices::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto pPSize1Value = filterArgs.value<int32>(k_PSize1_Key);
  auto pPSize2Value = filterArgs.value<int32>(k_PSize2_Key);
  auto pSSize1Value = filterArgs.value<int32>(k_SSize1_Key);
  auto pSSize2Value = filterArgs.value<int32>(k_SSize2_Key);
  auto pSliceStepValue = filterArgs.value<int32>(k_SliceStep_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pMotionDirectionArrayNameValue = filterArgs.value<DataPath>(k_MotionDirectionArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
