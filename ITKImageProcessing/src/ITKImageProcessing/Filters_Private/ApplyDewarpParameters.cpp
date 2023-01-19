#include "ApplyDewarpParameters.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/MontageStructureSelectionFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ApplyDewarpParameters::name() const
{
  return FilterTraits<ApplyDewarpParameters>::name.str();
}

//------------------------------------------------------------------------------
std::string ApplyDewarpParameters::className() const
{
  return FilterTraits<ApplyDewarpParameters>::className;
}

//------------------------------------------------------------------------------
Uuid ApplyDewarpParameters::uuid() const
{
  return FilterTraits<ApplyDewarpParameters>::uuid;
}

//------------------------------------------------------------------------------
std::string ApplyDewarpParameters::humanName() const
{
  return "Apply Dewarp Parameters";
}

//------------------------------------------------------------------------------
std::vector<std::string> ApplyDewarpParameters::defaultTags() const
{
  return {"#Processing", "#Processing"};
}

//------------------------------------------------------------------------------
Parameters ApplyDewarpParameters::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<MontageStructureSelectionFilterParameter>(k_MontageName_Key, "Montage Name", "", {}));
  params.insert(std::make_unique<StringParameter>(k_AttributeMatrixName_Key, "Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TransformPath_Key, "Transform Array", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<StringParameter>(k_TransformPrefix_Key, "Transformed Data Container Prefix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MaskName_Key, "Mask Array Name", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApplyDewarpParameters::clone() const
{
  return std::make_unique<ApplyDewarpParameters>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ApplyDewarpParameters::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pMontageNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageName_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  auto pTransformPathValue = filterArgs.value<DataPath>(k_TransformPath_Key);
  auto pTransformPrefixValue = filterArgs.value<StringParameter::ValueType>(k_TransformPrefix_Key);
  auto pMaskNameValue = filterArgs.value<StringParameter::ValueType>(k_MaskName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

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
Result<> ApplyDewarpParameters::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMontageNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageName_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  auto pTransformPathValue = filterArgs.value<DataPath>(k_TransformPath_Key);
  auto pTransformPrefixValue = filterArgs.value<StringParameter::ValueType>(k_TransformPrefix_Key);
  auto pMaskNameValue = filterArgs.value<StringParameter::ValueType>(k_MaskName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
