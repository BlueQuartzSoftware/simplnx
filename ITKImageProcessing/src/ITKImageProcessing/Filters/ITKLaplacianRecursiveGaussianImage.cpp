#include "ITKLaplacianRecursiveGaussianImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKLaplacianRecursiveGaussianImage::name() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::name.str();
}

Uuid ITKLaplacianRecursiveGaussianImage::uuid() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::uuid;
}

std::string ITKLaplacianRecursiveGaussianImage::humanName() const
{
  return "ITK::Laplacian Recursive Gaussian Image Filter";
}

Parameters ITKLaplacianRecursiveGaussianImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Sigma_Key, "Sigma", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "NormalizeAcrossScale", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKLaplacianRecursiveGaussianImage::clone() const
{
  return std::make_unique<ITKLaplacianRecursiveGaussianImage>();
}

Result<OutputActions> ITKLaplacianRecursiveGaussianImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSigmaValue = filterArgs.value<float64>(k_Sigma_Key);
  auto pNormalizeAcrossScaleValue = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKLaplacianRecursiveGaussianImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKLaplacianRecursiveGaussianImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSigmaValue = filterArgs.value<float64>(k_Sigma_Key);
  auto pNormalizeAcrossScaleValue = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
