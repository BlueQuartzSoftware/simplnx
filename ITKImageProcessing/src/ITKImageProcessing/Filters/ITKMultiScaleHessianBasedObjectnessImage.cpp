#include "ITKMultiScaleHessianBasedObjectnessImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMultiScaleHessianBasedObjectnessImage::name() const
{
  return FilterTraits<ITKMultiScaleHessianBasedObjectnessImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKMultiScaleHessianBasedObjectnessImage::className() const
{
  return FilterTraits<ITKMultiScaleHessianBasedObjectnessImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMultiScaleHessianBasedObjectnessImage::uuid() const
{
  return FilterTraits<ITKMultiScaleHessianBasedObjectnessImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMultiScaleHessianBasedObjectnessImage::humanName() const
{
  return "ITK::Multi-scale Hessian Based Objectness Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMultiScaleHessianBasedObjectnessImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Edge"};
}

//------------------------------------------------------------------------------
Parameters ITKMultiScaleHessianBasedObjectnessImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_ObjectDimension_Key, "ObjectDimension", "", 1234356));
  params.insert(std::make_unique<Float64Parameter>(k_Alpha_Key, "Alpha", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Beta_Key, "Beta", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Gamma_Key, "Gamma", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_BrightObject_Key, "BrightObject", "", false));
  params.insert(std::make_unique<BoolParameter>(k_ScaleObjectnessMeasure_Key, "ScaleObjectnessMeasure", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_SigmaMinimum_Key, "SigmaMinimum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_SigmaMaximum_Key, "SigmaMaximum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfSigmaSteps_Key, "NumberOfSigmaSteps", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMultiScaleHessianBasedObjectnessImage::clone() const
{
  return std::make_unique<ITKMultiScaleHessianBasedObjectnessImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMultiScaleHessianBasedObjectnessImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pObjectDimensionValue = filterArgs.value<int32>(k_ObjectDimension_Key);
  auto pAlphaValue = filterArgs.value<float64>(k_Alpha_Key);
  auto pBetaValue = filterArgs.value<float64>(k_Beta_Key);
  auto pGammaValue = filterArgs.value<float64>(k_Gamma_Key);
  auto pBrightObjectValue = filterArgs.value<bool>(k_BrightObject_Key);
  auto pScaleObjectnessMeasureValue = filterArgs.value<bool>(k_ScaleObjectnessMeasure_Key);
  auto pSigmaMinimumValue = filterArgs.value<float64>(k_SigmaMinimum_Key);
  auto pSigmaMaximumValue = filterArgs.value<float64>(k_SigmaMaximum_Key);
  auto pNumberOfSigmaStepsValue = filterArgs.value<float64>(k_NumberOfSigmaSteps_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

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
  auto action = std::make_unique<ITKMultiScaleHessianBasedObjectnessImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKMultiScaleHessianBasedObjectnessImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pObjectDimensionValue = filterArgs.value<int32>(k_ObjectDimension_Key);
  auto pAlphaValue = filterArgs.value<float64>(k_Alpha_Key);
  auto pBetaValue = filterArgs.value<float64>(k_Beta_Key);
  auto pGammaValue = filterArgs.value<float64>(k_Gamma_Key);
  auto pBrightObjectValue = filterArgs.value<bool>(k_BrightObject_Key);
  auto pScaleObjectnessMeasureValue = filterArgs.value<bool>(k_ScaleObjectnessMeasure_Key);
  auto pSigmaMinimumValue = filterArgs.value<float64>(k_SigmaMinimum_Key);
  auto pSigmaMaximumValue = filterArgs.value<float64>(k_SigmaMaximum_Key);
  auto pNumberOfSigmaStepsValue = filterArgs.value<float64>(k_NumberOfSigmaSteps_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
