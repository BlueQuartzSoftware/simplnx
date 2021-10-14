#include "ITKFFTNormalizedCorrelationImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKFFTNormalizedCorrelationImage::name() const
{
  return FilterTraits<ITKFFTNormalizedCorrelationImage>::name.str();
}

std::string ITKFFTNormalizedCorrelationImage::className() const
{
  return FilterTraits<ITKFFTNormalizedCorrelationImage>::className;
}

Uuid ITKFFTNormalizedCorrelationImage::uuid() const
{
  return FilterTraits<ITKFFTNormalizedCorrelationImage>::uuid;
}

std::string ITKFFTNormalizedCorrelationImage::humanName() const
{
  return "ITK::FFT Normalized Correlation Image";
}

Parameters ITKFFTNormalizedCorrelationImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_RequiredNumberOfOverlappingPixels_Key, "RequiredNumberOfOverlappingPixels", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_RequiredFractionOfOverlappingPixels_Key, "RequiredFractionOfOverlappingPixels", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Fixed Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MovingCellArrayPath_Key, "Moving Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKFFTNormalizedCorrelationImage::clone() const
{
  return std::make_unique<ITKFFTNormalizedCorrelationImage>();
}

Result<OutputActions> ITKFFTNormalizedCorrelationImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRequiredNumberOfOverlappingPixelsValue = filterArgs.value<float64>(k_RequiredNumberOfOverlappingPixels_Key);
  auto pRequiredFractionOfOverlappingPixelsValue = filterArgs.value<float64>(k_RequiredFractionOfOverlappingPixels_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMovingCellArrayPathValue = filterArgs.value<DataPath>(k_MovingCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKFFTNormalizedCorrelationImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKFFTNormalizedCorrelationImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRequiredNumberOfOverlappingPixelsValue = filterArgs.value<float64>(k_RequiredNumberOfOverlappingPixels_Key);
  auto pRequiredFractionOfOverlappingPixelsValue = filterArgs.value<float64>(k_RequiredFractionOfOverlappingPixels_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMovingCellArrayPathValue = filterArgs.value<DataPath>(k_MovingCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
