#include "ITKAdaptiveHistogramEqualizationImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKAdaptiveHistogramEqualizationImage::name() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::name.str();
}

std::string ITKAdaptiveHistogramEqualizationImage::className() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::className;
}

Uuid ITKAdaptiveHistogramEqualizationImage::uuid() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::uuid;
}

std::string ITKAdaptiveHistogramEqualizationImage::humanName() const
{
  return "ITK::Adaptive Histogram Equalization Image Filter";
}

Parameters ITKAdaptiveHistogramEqualizationImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Radius_Key, "Radius", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<Float32Parameter>(k_Alpha_Key, "Alpha", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta_Key, "Beta", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKAdaptiveHistogramEqualizationImage::clone() const
{
  return std::make_unique<ITKAdaptiveHistogramEqualizationImage>();
}

Result<OutputActions> ITKAdaptiveHistogramEqualizationImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Radius_Key);
  auto pAlphaValue = filterArgs.value<float32>(k_Alpha_Key);
  auto pBetaValue = filterArgs.value<float32>(k_Beta_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKAdaptiveHistogramEqualizationImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKAdaptiveHistogramEqualizationImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Radius_Key);
  auto pAlphaValue = filterArgs.value<float32>(k_Alpha_Key);
  auto pBetaValue = filterArgs.value<float32>(k_Beta_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
