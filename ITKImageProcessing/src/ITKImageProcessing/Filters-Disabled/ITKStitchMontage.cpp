#include "ITKStitchMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/MontageSelectionFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKStitchMontage::name() const
{
  return FilterTraits<ITKStitchMontage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKStitchMontage::className() const
{
  return FilterTraits<ITKStitchMontage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKStitchMontage::uuid() const
{
  return FilterTraits<ITKStitchMontage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKStitchMontage::humanName() const
{
  return "ITK::Stitch Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKStitchMontage::defaultTags() const
{
  return {"#IO", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters ITKStitchMontage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<MontageSelectionFilterParameter>(k_MontageSelection_Key, "Montage Selection", "", {}));
  params.insert(std::make_unique<StringParameter>(k_CommonAttributeMatrixName_Key, "Common Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CommonDataArrayName_Key, "Common Data Array", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MontageDataContainerName_Key, "Montage Data Container Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MontageAttributeMatrixName_Key, "Montage Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MontageDataArrayName_Key, "Montage Data Array Name", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKStitchMontage::clone() const
{
  return std::make_unique<ITKStitchMontage>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ITKStitchMontage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMontageSelectionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);
  auto pMontageDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageDataContainerName_Key);
  auto pMontageAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageAttributeMatrixName_Key);
  auto pMontageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageDataArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKStitchMontageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ITKStitchMontage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMontageSelectionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);
  auto pMontageDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageDataContainerName_Key);
  auto pMontageAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageAttributeMatrixName_Key);
  auto pMontageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
