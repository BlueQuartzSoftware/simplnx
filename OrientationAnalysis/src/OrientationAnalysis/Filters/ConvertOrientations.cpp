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
Result<OutputActions> ConvertOrientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key);
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  auto pInputOrientationArrayPathValue = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  auto pOutputOrientationArrayNameValue = filterArgs.value<DataPath>(k_OutputOrientationArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ConvertOrientationsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
