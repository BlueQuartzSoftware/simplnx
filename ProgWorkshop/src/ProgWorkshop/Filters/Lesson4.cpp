#include "Lesson4.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string Lesson4::name() const
{
  return FilterTraits<Lesson4>::name.str();
}

//------------------------------------------------------------------------------
std::string Lesson4::className() const
{
  return FilterTraits<Lesson4>::className;
}

//------------------------------------------------------------------------------
Uuid Lesson4::uuid() const
{
  return FilterTraits<Lesson4>::uuid;
}

//------------------------------------------------------------------------------
std::string Lesson4::humanName() const
{
  return "Lesson4";
}

//------------------------------------------------------------------------------
std::vector<std::string> Lesson4::defaultTags() const
{
  return {"#Unsupported", "#ProgWorkshop"};
}

//------------------------------------------------------------------------------
Parameters Lesson4::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputDataArrayPath_Key, "InputDataArrayPath", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "OutputDataArrayPath", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_Value_Key, "Value", "", 1.23345f));
  params.insert(std::make_unique<ChoicesParameter>(k_Operator_Key, "Operator", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Selection_Key, "Linked Bool", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_FloatValue_Key, "Float Value", "", 1.23345f));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_Selection_Key, k_FloatValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Lesson4::clone() const
{
  return std::make_unique<Lesson4>();
}

//------------------------------------------------------------------------------
Result<OutputActions> Lesson4::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pValueValue = filterArgs.value<float32>(k_Value_Key);
  auto pOperatorValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  auto pSelectionValue = filterArgs.value<bool>(k_Selection_Key);
  auto pFloatValueValue = filterArgs.value<float32>(k_FloatValue_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<Lesson4Action>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> Lesson4::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pValueValue = filterArgs.value<float32>(k_Value_Key);
  auto pOperatorValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  auto pSelectionValue = filterArgs.value<bool>(k_Selection_Key);
  auto pFloatValueValue = filterArgs.value<float32>(k_FloatValue_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
