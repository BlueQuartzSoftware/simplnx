#include "ITKSpeckleNoiseImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKSpeckleNoiseImage::name() const
{
  return FilterTraits<ITKSpeckleNoiseImage>::name.str();
}

Uuid ITKSpeckleNoiseImage::uuid() const
{
  return FilterTraits<ITKSpeckleNoiseImage>::uuid;
}

std::string ITKSpeckleNoiseImage::humanName() const
{
  return "ITK::Speckle Noise Image Filter";
}

Parameters ITKSpeckleNoiseImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_StandardDeviation_Key, "StandardDeviation", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Seed_Key, "Seed", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKSpeckleNoiseImage::clone() const
{
  return std::make_unique<ITKSpeckleNoiseImage>();
}

Result<OutputActions> ITKSpeckleNoiseImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pStandardDeviationValue = filterArgs.value<float64>(k_StandardDeviation_Key);
  auto pSeedValue = filterArgs.value<float64>(k_Seed_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKSpeckleNoiseImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKSpeckleNoiseImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pStandardDeviationValue = filterArgs.value<float64>(k_StandardDeviation_Key);
  auto pSeedValue = filterArgs.value<float64>(k_Seed_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
