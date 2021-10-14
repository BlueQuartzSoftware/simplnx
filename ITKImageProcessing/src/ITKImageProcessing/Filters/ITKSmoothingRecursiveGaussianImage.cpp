#include "ITKSmoothingRecursiveGaussianImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKSmoothingRecursiveGaussianImage::name() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImage>::name.str();
}

Uuid ITKSmoothingRecursiveGaussianImage::uuid() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImage>::uuid;
}

std::string ITKSmoothingRecursiveGaussianImage::humanName() const
{
  return "ITK::Smoothing Recursive Gaussian Image Filter";
}

Parameters ITKSmoothingRecursiveGaussianImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Sigma_Key, "Sigma", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "NormalizeAcrossScale", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKSmoothingRecursiveGaussianImage::clone() const
{
  return std::make_unique<ITKSmoothingRecursiveGaussianImage>();
}

Result<OutputActions> ITKSmoothingRecursiveGaussianImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSigmaValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Sigma_Key);
  auto pNormalizeAcrossScaleValue = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKSmoothingRecursiveGaussianImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKSmoothingRecursiveGaussianImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSigmaValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Sigma_Key);
  auto pNormalizeAcrossScaleValue = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
