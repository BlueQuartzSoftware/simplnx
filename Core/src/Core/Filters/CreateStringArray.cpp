#include "CreateStringArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateStringArray::name() const
{
  return FilterTraits<CreateStringArray>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateStringArray::className() const
{
  return FilterTraits<CreateStringArray>::className;
}

//------------------------------------------------------------------------------
Uuid CreateStringArray::uuid() const
{
  return FilterTraits<CreateStringArray>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateStringArray::humanName() const
{
  return "Create String Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateStringArray::defaultTags() const
{
  return {"#Core", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters CreateStringArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<StringParameter>(k_InitializationValue_Key, "Initialization Value", "", "SomeString"));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArray_Key, "Created Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateStringArray::clone() const
{
  return std::make_unique<CreateStringArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateStringArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInitializationValueValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  auto pNewArrayValue = filterArgs.value<DataPath>(k_NewArray_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateStringArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateStringArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInitializationValueValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  auto pNewArrayValue = filterArgs.value<DataPath>(k_NewArray_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
