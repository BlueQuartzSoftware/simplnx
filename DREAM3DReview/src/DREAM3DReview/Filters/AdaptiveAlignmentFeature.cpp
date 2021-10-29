#include "AdaptiveAlignmentFeature.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
  params.linkParameters(k_GlobalCorrection_Key, k_ImageDataArrayPath_Key, 0);
  params.linkParameters(k_GlobalCorrection_Key, k_ShiftX_Key, 1);
  params.linkParameters(k_GlobalCorrection_Key, k_ShiftY_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AdaptiveAlignmentFeature::clone() const
{
  return std::make_unique<AdaptiveAlignmentFeature>();
}

//------------------------------------------------------------------------------
Result<OutputActions> AdaptiveAlignmentFeature::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pGlobalCorrectionValue = filterArgs.value<ChoicesParameter::ValueType>(k_GlobalCorrection_Key);
  auto pImageDataArrayPathValue = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  auto pShiftXValue = filterArgs.value<float32>(k_ShiftX_Key);
  auto pShiftYValue = filterArgs.value<float32>(k_ShiftY_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AdaptiveAlignmentFeatureAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
