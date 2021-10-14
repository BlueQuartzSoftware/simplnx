#include "ITKHistogramMatchingImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKHistogramMatchingImage::name() const
{
  return FilterTraits<ITKHistogramMatchingImage>::name.str();
}

Uuid ITKHistogramMatchingImage::uuid() const
{
  return FilterTraits<ITKHistogramMatchingImage>::uuid;
}

std::string ITKHistogramMatchingImage::humanName() const
{
  return "ITK::Histogram Matching Image Filter";
}

Parameters ITKHistogramMatchingImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfHistogramLevels_Key, "NumberOfHistogramLevels", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfMatchPoints_Key, "NumberOfMatchPoints", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_ThresholdAtMeanIntensity_Key, "ThresholdAtMeanIntensity", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Input Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ReferenceCellArrayPath_Key, "Reference Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKHistogramMatchingImage::clone() const
{
  return std::make_unique<ITKHistogramMatchingImage>();
}

Result<OutputActions> ITKHistogramMatchingImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pNumberOfHistogramLevelsValue = filterArgs.value<float64>(k_NumberOfHistogramLevels_Key);
  auto pNumberOfMatchPointsValue = filterArgs.value<float64>(k_NumberOfMatchPoints_Key);
  auto pThresholdAtMeanIntensityValue = filterArgs.value<bool>(k_ThresholdAtMeanIntensity_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pReferenceCellArrayPathValue = filterArgs.value<DataPath>(k_ReferenceCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKHistogramMatchingImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKHistogramMatchingImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNumberOfHistogramLevelsValue = filterArgs.value<float64>(k_NumberOfHistogramLevels_Key);
  auto pNumberOfMatchPointsValue = filterArgs.value<float64>(k_NumberOfMatchPoints_Key);
  auto pThresholdAtMeanIntensityValue = filterArgs.value<bool>(k_ThresholdAtMeanIntensity_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pReferenceCellArrayPathValue = filterArgs.value<DataPath>(k_ReferenceCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
