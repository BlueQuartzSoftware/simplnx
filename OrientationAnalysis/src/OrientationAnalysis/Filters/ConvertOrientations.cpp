#include "ConvertOrientations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertOrientations::name() const
{
  return FilterTraits<ConvertOrientations>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertOrientations::className() const
{
  return FilterTraits<ConvertOrientations>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertOrientations::uuid() const
{
  return FilterTraits<ConvertOrientations>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertOrientations::humanName() const
{
  return "Convert Orientation Representation";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertOrientations::defaultTags() const
{
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertOrientations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_InputType_Key, "Input Orientation Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<ChoicesParameter>(k_OutputType_Key, "Output Orientation Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputOrientationArrayPath_Key, "Input Orientations", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputOrientationArrayName_Key, "Output Orientations", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertOrientations::clone() const
{
  return std::make_unique<ConvertOrientations>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertOrientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key);
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  auto pInputOrientationArrayPathValue = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  auto pOutputOrientationArrayNameValue = filterArgs.value<DataPath>(k_OutputOrientationArrayName_Key);

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
  auto action = std::make_unique<ConvertOrientationsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ConvertOrientations::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key);
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  auto pInputOrientationArrayPathValue = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  auto pOutputOrientationArrayNameValue = filterArgs.value<DataPath>(k_OutputOrientationArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
