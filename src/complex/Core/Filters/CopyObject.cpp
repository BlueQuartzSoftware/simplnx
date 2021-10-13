#include "CopyObject.hpp"

#include "complex/Core/Parameters/ArraySelectionParameter.hpp"
#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Core/Parameters/StringParameter.hpp"
#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
std::string CopyObject::name() const
{
  return FilterTraits<CopyObject>::name.str();
}

Uuid CopyObject::uuid() const
{
  return FilterTraits<CopyObject>::uuid;
}

std::string CopyObject::humanName() const
{
  return "Copy Object";
}

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

IFilter::UniquePointer CopyObject::clone() const
{
  return std::make_unique<CopyObject>();
}

Result<OutputActions> CopyObject::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pObjectToCopyValue = filterArgs.value<ChoicesParameter::ValueType>(k_ObjectToCopy_Key);
  auto pDataContainerToCopyValue = filterArgs.value<DataPath>(k_DataContainerToCopy_Key);
  auto pAttributeMatrixToCopyValue = filterArgs.value<DataPath>(k_AttributeMatrixToCopy_Key);
  auto pAttributeArrayToCopyValue = filterArgs.value<DataPath>(k_AttributeArrayToCopy_Key);
  auto pCopiedObjectNameValue = filterArgs.value<StringParameter::ValueType>(k_CopiedObjectName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CopyObjectAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CopyObject::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
