#include "ITKDiscreteGaussianImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::name() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::className() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKDiscreteGaussianImage::uuid() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::humanName() const
{
  return "ITK::Discrete Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDiscreteGaussianImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKDiscreteGaussianImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Variance_Key, "Variance", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<Int32Parameter>(k_MaximumKernelWidth_Key, "MaximumKernelWidth", "", 1234356));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaximumError_Key, "MaximumError", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKDiscreteGaussianImage::clone() const
{
  return std::make_unique<ITKDiscreteGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDiscreteGaussianImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pVarianceValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Variance_Key);
  auto pMaximumKernelWidthValue = filterArgs.value<int32>(k_MaximumKernelWidth_Key);
  auto pMaximumErrorValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaximumError_Key);
  auto pUseImageSpacingValue = filterArgs.value<bool>(k_UseImageSpacing_Key);
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
  auto action = std::make_unique<ITKDiscreteGaussianImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKDiscreteGaussianImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pVarianceValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Variance_Key);
  auto pMaximumKernelWidthValue = filterArgs.value<int32>(k_MaximumKernelWidth_Key);
  auto pMaximumErrorValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaximumError_Key);
  auto pUseImageSpacingValue = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
