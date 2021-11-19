#include "AdaptiveAlignmentFeature.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AdaptiveAlignmentFeature::name() const
{
  return FilterTraits<AdaptiveAlignmentFeature>::name.str();
}

//------------------------------------------------------------------------------
std::string AdaptiveAlignmentFeature::className() const
{
  return FilterTraits<AdaptiveAlignmentFeature>::className;
}

//------------------------------------------------------------------------------
Uuid AdaptiveAlignmentFeature::uuid() const
{
  return FilterTraits<AdaptiveAlignmentFeature>::uuid;
}

//------------------------------------------------------------------------------
std::string AdaptiveAlignmentFeature::humanName() const
{
  return "Adaptive Alignment (Feature)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AdaptiveAlignmentFeature::defaultTags() const
{
  return {"#Reconstruction", "#Anisotropic Alignment"};
}

//------------------------------------------------------------------------------
Parameters AdaptiveAlignmentFeature::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GlobalCorrection_Key, "Global Correction", "", 0, ChoicesParameter::Choices{"None", "SEM Images", "Own Shifts"}));
  params.insertSeparator(Parameters::Separator{"Image Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageDataArrayPath_Key, "Image Data", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_ShiftX_Key, "Total Shift In X-Direction (Microns)", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_ShiftY_Key, "Total Shift In Y-Direction (Microns)", "", 1.23345f));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_GlobalCorrection_Key, k_ImageDataArrayPath_Key, 1);
  params.linkParameters(k_GlobalCorrection_Key, k_ShiftX_Key, 2);
  params.linkParameters(k_GlobalCorrection_Key, k_ShiftY_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AdaptiveAlignmentFeature::clone() const
{
  return std::make_unique<AdaptiveAlignmentFeature>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AdaptiveAlignmentFeature::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pGlobalCorrectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_GlobalCorrection_Key);
  auto pImageDataArrayPathValue = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  auto pShiftXValue = filterArgs.value<float32>(k_ShiftX_Key);
  auto pShiftYValue = filterArgs.value<float32>(k_ShiftY_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

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
Result<> AdaptiveAlignmentFeature::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pGlobalCorrectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_GlobalCorrection_Key);
  auto pImageDataArrayPathValue = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  auto pShiftXValue = filterArgs.value<float32>(k_ShiftX_Key);
  auto pShiftYValue = filterArgs.value<float32>(k_ShiftY_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
