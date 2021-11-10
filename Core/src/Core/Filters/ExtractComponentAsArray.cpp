#include "ExtractComponentAsArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ExtractComponentAsArray::name() const
{
  return FilterTraits<ExtractComponentAsArray>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractComponentAsArray::className() const
{
  return FilterTraits<ExtractComponentAsArray>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractComponentAsArray::uuid() const
{
  return FilterTraits<ExtractComponentAsArray>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractComponentAsArray::humanName() const
{
  return "Extract Component as Attribute Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractComponentAsArray::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters ExtractComponentAsArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_CompNumber_Key, "Component Number to Extract", "", 1234356));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Multicomponent Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArrayArrayName_Key, "Scalar Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractComponentAsArray::clone() const
{
  return std::make_unique<ExtractComponentAsArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractComponentAsArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCompNumberValue = filterArgs.value<int32>(k_CompNumber_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayArrayNameValue = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);

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
  auto action = std::make_unique<ExtractComponentAsArrayAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ExtractComponentAsArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCompNumberValue = filterArgs.value<int32>(k_CompNumber_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayArrayNameValue = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
