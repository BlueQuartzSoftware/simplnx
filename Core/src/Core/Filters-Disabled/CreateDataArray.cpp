#include "CreateDataArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/RangeFilterParameter.hpp"
#include "complex/Parameters/ScalarTypeFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string CreateDataArray::name() const
{
  return FilterTraits<CreateDataArray>::name.str();
}

std::string CreateDataArray::className() const
{
  return FilterTraits<CreateDataArray>::className;
}

Uuid CreateDataArray::uuid() const
{
  return FilterTraits<CreateDataArray>::uuid;
}

std::string CreateDataArray::humanName() const
{
  return "Create Data Array";
}

Parameters CreateDataArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ScalarTypeFilterParameter>(k_ScalarType_Key, "Scalar Type", "", {}));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfComponents_Key, "Number of Components", "", 1234356));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitializationType_Key, "Initialization Type", "", 0, ChoicesParameter::Choices{"Manual", "Random With Range"}));
  params.insert(std::make_unique<StringParameter>(k_InitializationValue_Key, "Initialization Value", "", "SomeString"));
  params.insert(std::make_unique<RangeFilterParameter>(k_InitializationRange_Key, "Initialization Range", "", {}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArray_Key, "Created Attribute Array", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_InitializationType_Key, k_InitializationValue_Key, 0);
  params.linkParameters(k_InitializationType_Key, k_InitializationRange_Key, 1);

  return params;
}

IFilter::UniquePointer CreateDataArray::clone() const
{
  return std::make_unique<CreateDataArray>();
}

Result<OutputActions> CreateDataArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pScalarTypeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<int32>(k_NumberOfComponents_Key);
  auto pInitializationTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InitializationType_Key);
  auto pInitializationValueValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  auto pInitializationRangeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InitializationRange_Key);
  auto pNewArrayValue = filterArgs.value<DataPath>(k_NewArray_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateDataArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CreateDataArray::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pScalarTypeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ScalarType_Key);
  auto pNumberOfComponentsValue = filterArgs.value<int32>(k_NumberOfComponents_Key);
  auto pInitializationTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InitializationType_Key);
  auto pInitializationValueValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  auto pInitializationRangeValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InitializationRange_Key);
  auto pNewArrayValue = filterArgs.value<DataPath>(k_NewArray_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
