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
  params.insert(std::make_unique<ArraySelectionParameter>(k_TransformPath_Key, "Transform Array", "", DataPath{}));
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
IFilter::PreflightResult ApplyDewarpParameters::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMontageNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageName_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  auto pTransformPathValue = filterArgs.value<DataPath>(k_TransformPath_Key);
  auto pTransformPrefixValue = filterArgs.value<StringParameter::ValueType>(k_TransformPrefix_Key);
  auto pMaskNameValue = filterArgs.value<StringParameter::ValueType>(k_MaskName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ApplyDewarpParametersAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ApplyDewarpParameters::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
