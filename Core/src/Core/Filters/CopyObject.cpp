#include "CopyObject.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CopyObject::name() const
{
  return FilterTraits<CopyObject>::name.str();
}

//------------------------------------------------------------------------------
std::string CopyObject::className() const
{
  return FilterTraits<CopyObject>::className;
}

//------------------------------------------------------------------------------
Uuid CopyObject::uuid() const
{
  return FilterTraits<CopyObject>::uuid;
}

//------------------------------------------------------------------------------
std::string CopyObject::humanName() const
{
  return "Copy Object";
}

//------------------------------------------------------------------------------
std::vector<std::string> CopyObject::defaultTags() const
{
  return {"#Core", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters CopyObject::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_ObjectToCopy_Key, "Object Type to Copy", "", 0, ChoicesParameter::Choices{"Data Container", "Attribute Matrix", "Attribute Array"}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerToCopy_Key, "Data Container to Copy", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_AttributeMatrixToCopy_Key, "Attribute Matrix to Copy", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AttributeArrayToCopy_Key, "Attribute Array to Copy", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_CopiedObjectName_Key, "Copied Object Name", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ObjectToCopy_Key, k_DataContainerToCopy_Key, 0);
  params.linkParameters(k_ObjectToCopy_Key, k_AttributeMatrixToCopy_Key, 1);
  params.linkParameters(k_ObjectToCopy_Key, k_AttributeArrayToCopy_Key, 2);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CopyObject::clone() const
{
  return std::make_unique<CopyObject>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CopyObject::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pObjectToCopyValue = filterArgs.value<ChoicesParameter::ValueType>(k_ObjectToCopy_Key);
  auto pDataContainerToCopyValue = filterArgs.value<DataPath>(k_DataContainerToCopy_Key);
  auto pAttributeMatrixToCopyValue = filterArgs.value<DataPath>(k_AttributeMatrixToCopy_Key);
  auto pAttributeArrayToCopyValue = filterArgs.value<DataPath>(k_AttributeArrayToCopy_Key);
  auto pCopiedObjectNameValue = filterArgs.value<StringParameter::ValueType>(k_CopiedObjectName_Key);

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
Result<> CopyObject::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pObjectToCopyValue = filterArgs.value<ChoicesParameter::ValueType>(k_ObjectToCopy_Key);
  auto pDataContainerToCopyValue = filterArgs.value<DataPath>(k_DataContainerToCopy_Key);
  auto pAttributeMatrixToCopyValue = filterArgs.value<DataPath>(k_AttributeMatrixToCopy_Key);
  auto pAttributeArrayToCopyValue = filterArgs.value<DataPath>(k_AttributeArrayToCopy_Key);
  auto pCopiedObjectNameValue = filterArgs.value<StringParameter::ValueType>(k_CopiedObjectName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
