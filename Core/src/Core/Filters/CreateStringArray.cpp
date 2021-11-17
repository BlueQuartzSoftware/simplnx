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

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInitializationValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  auto pNewArrayValue = filterArgs.value<DataPath>(k_NewArray_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateStringArrayAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> CreateStringArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInitializationValue = filterArgs.value<StringParameter::ValueType>(k_InitializationValue_Key);
  auto pNewArrayValue = filterArgs.value<DataPath>(k_NewArray_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
