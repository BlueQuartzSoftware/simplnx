#include "ITKStitchMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
  /*[x]*/ params.insert(std::make_unique<MontageSelectionFilterParameter>(k_MontageSelection_Key, "Montage Selection", "", {}));
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
IFilter::PreflightResult ITKStitchMontage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMontageSelectionValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  auto pCommonAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);
  auto pMontageDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageDataContainerName_Key);
  auto pMontageAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageAttributeMatrixName_Key);
  auto pMontageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageDataArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKStitchMontage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
