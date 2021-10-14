#include "ITKBilateralImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKBilateralImage::name() const
{
  return FilterTraits<ITKBilateralImage>::name.str();
}

std::string ITKBilateralImage::className() const
{
  return FilterTraits<ITKBilateralImage>::className;
}

Uuid ITKBilateralImage::uuid() const
{
  return FilterTraits<ITKBilateralImage>::uuid;
}

std::string ITKBilateralImage::humanName() const
{
  return "ITK::Bilateral Image Filter";
}

Parameters ITKBilateralImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_DomainSigma_Key, "DomainSigma", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_RangeSigma_Key, "RangeSigma", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfRangeGaussianSamples_Key, "NumberOfRangeGaussianSamples", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKBilateralImage::clone() const
{
  return std::make_unique<ITKBilateralImage>();
}

Result<OutputActions> ITKBilateralImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDomainSigmaValue = filterArgs.value<float64>(k_DomainSigma_Key);
  auto pRangeSigmaValue = filterArgs.value<float64>(k_RangeSigma_Key);
  auto pNumberOfRangeGaussianSamplesValue = filterArgs.value<float64>(k_NumberOfRangeGaussianSamples_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKBilateralImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKBilateralImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDomainSigmaValue = filterArgs.value<float64>(k_DomainSigma_Key);
  auto pRangeSigmaValue = filterArgs.value<float64>(k_RangeSigma_Key);
  auto pNumberOfRangeGaussianSamplesValue = filterArgs.value<float64>(k_NumberOfRangeGaussianSamples_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
