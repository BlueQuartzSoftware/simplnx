#include "Lesson5.hpp"

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
std::string Lesson5::name() const
{
  return FilterTraits<Lesson5>::name.str();
}

//------------------------------------------------------------------------------
std::string Lesson5::className() const
{
  return FilterTraits<Lesson5>::className;
}

//------------------------------------------------------------------------------
Uuid Lesson5::uuid() const
{
  return FilterTraits<Lesson5>::uuid;
}

//------------------------------------------------------------------------------
std::string Lesson5::humanName() const
{
  return "Lesson5";
}

//------------------------------------------------------------------------------
std::vector<std::string> Lesson5::defaultTags() const
{
  return {"#Unsupported", "#ProgWorkshop"};
}

//------------------------------------------------------------------------------
Parameters Lesson5::parameters() const
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
IFilter::UniquePointer Lesson5::clone() const
{
  return std::make_unique<Lesson5>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult Lesson5::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pValueValue = filterArgs.value<float32>(k_Value_Key);
  auto pOperatorValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operator_Key);
  auto pSelectionValue = filterArgs.value<bool>(k_Selection_Key);
  auto pFloatValueValue = filterArgs.value<float32>(k_FloatValue_Key);

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
  auto action = std::make_unique<Lesson5Action>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> Lesson5::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
