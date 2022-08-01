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
//------------------------------------------------------------------------------
std::string NormalizeArrays::name() const
{
  return FilterTraits<NormalizeArrays>::name.str();
}

//------------------------------------------------------------------------------
std::string NormalizeArrays::className() const
{
  return FilterTraits<NormalizeArrays>::className;
}

//------------------------------------------------------------------------------
Uuid NormalizeArrays::uuid() const
{
  return FilterTraits<NormalizeArrays>::uuid;
}

//------------------------------------------------------------------------------
std::string NormalizeArrays::humanName() const
{
  return "Normalize Attribute Arrays";
}

//------------------------------------------------------------------------------
std::vector<std::string> NormalizeArrays::defaultTags() const
{
  return {"#DREAM3DReview", "#Statistics"};
}

//------------------------------------------------------------------------------
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
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Normalize", "",
                                                               MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}, MultiArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_NormalizeType_Key, k_RangeMin_Key, 0);
  params.linkParameters(k_NormalizeType_Key, k_RangeMax_Key, 0);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_UseMask_Key, k_DefaultValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer NormalizeArrays::clone() const
{
  return std::make_unique<NormalizeArrays>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult NormalizeArrays::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNormalizeTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_NormalizeType_Key);
  auto pPostfixValue = filterArgs.value<StringParameter::ValueType>(k_Postfix_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pDefaultValue = filterArgs.value<float64>(k_DefaultValue_Key);
  auto pRangeMinValue = filterArgs.value<float64>(k_RangeMin_Key);
  auto pRangeMaxValue = filterArgs.value<float64>(k_RangeMax_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

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
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> NormalizeArrays::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNormalizeTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_NormalizeType_Key);
  auto pPostfixValue = filterArgs.value<StringParameter::ValueType>(k_Postfix_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pDefaultValue = filterArgs.value<float64>(k_DefaultValue_Key);
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
