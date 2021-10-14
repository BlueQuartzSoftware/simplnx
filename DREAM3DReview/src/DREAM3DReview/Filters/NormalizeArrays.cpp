#include "NormalizeArrays.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string NormalizeArrays::name() const
{
  return FilterTraits<NormalizeArrays>::name.str();
}

Uuid NormalizeArrays::uuid() const
{
  return FilterTraits<NormalizeArrays>::uuid;
}

std::string NormalizeArrays::humanName() const
{
  return "Normalize Attribute Arrays";
}

Parameters NormalizeArrays::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_NormalizeType_Key, "Operation Type", "", 0, ChoicesParameter::Choices{"Rescale to Range", "Standardize"}));
  params.insert(std::make_unique<StringParameter>(k_Postfix_Key, "Postfix", "", "SomeString"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_DefaultValue_Key, "Default Masked Value", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_RangeMin_Key, "Range Minimum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_RangeMax_Key, "Range Maximum", "", 2.3456789));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Normalize", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_NormalizeType_Key, k_RangeMin_Key, 0);
  params.linkParameters(k_NormalizeType_Key, k_RangeMax_Key, 1);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_UseMask_Key, k_DefaultValue_Key, true);

  return params;
}

IFilter::UniquePointer NormalizeArrays::clone() const
{
  return std::make_unique<NormalizeArrays>();
}

Result<OutputActions> NormalizeArrays::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pNormalizeTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_NormalizeType_Key);
  auto pPostfixValue = filterArgs.value<StringParameter::ValueType>(k_Postfix_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pDefaultValueValue = filterArgs.value<float64>(k_DefaultValue_Key);
  auto pRangeMinValue = filterArgs.value<float64>(k_RangeMin_Key);
  auto pRangeMaxValue = filterArgs.value<float64>(k_RangeMax_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<NormalizeArraysAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> NormalizeArrays::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNormalizeTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_NormalizeType_Key);
  auto pPostfixValue = filterArgs.value<StringParameter::ValueType>(k_Postfix_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pDefaultValueValue = filterArgs.value<float64>(k_DefaultValue_Key);
  auto pRangeMinValue = filterArgs.value<float64>(k_RangeMin_Key);
  auto pRangeMaxValue = filterArgs.value<float64>(k_RangeMax_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
