#include "Lesson1.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string Lesson1::name() const
{
  return FilterTraits<Lesson1>::name.str();
}

//------------------------------------------------------------------------------
std::string Lesson1::className() const
{
  return FilterTraits<Lesson1>::className;
}

//------------------------------------------------------------------------------
Uuid Lesson1::uuid() const
{
  return FilterTraits<Lesson1>::uuid;
}

//------------------------------------------------------------------------------
std::string Lesson1::humanName() const
{
  return "Lesson1";
}

//------------------------------------------------------------------------------
std::vector<std::string> Lesson1::defaultTags() const
{
  return {"#Unsupported", "#ProgWorkshop"};
}

//------------------------------------------------------------------------------
Parameters Lesson1::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputDataArrayPath_Key, "InputDataArrayPath", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "OutputDataArrayPath", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_Value_Key, "Value", "", 1.23345f));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Lesson1::clone() const
{
  return std::make_unique<Lesson1>();
}

//------------------------------------------------------------------------------
Result<OutputActions> Lesson1::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pValueValue = filterArgs.value<float32>(k_Value_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<Lesson1Action>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> Lesson1::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputDataArrayPathValue = filterArgs.value<DataPath>(k_InputDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pValueValue = filterArgs.value<float32>(k_Value_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
