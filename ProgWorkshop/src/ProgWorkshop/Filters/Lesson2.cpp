#include "Lesson2.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string Lesson2::name() const
{
  return FilterTraits<Lesson2>::name.str();
}

//------------------------------------------------------------------------------
std::string Lesson2::className() const
{
  return FilterTraits<Lesson2>::className;
}

//------------------------------------------------------------------------------
Uuid Lesson2::uuid() const
{
  return FilterTraits<Lesson2>::uuid;
}

//------------------------------------------------------------------------------
std::string Lesson2::humanName() const
{
  return "Lesson2";
}

//------------------------------------------------------------------------------
std::vector<std::string> Lesson2::defaultTags() const
{
  return {"#Unsupported", "#ProgWorkshop"};
}

//------------------------------------------------------------------------------
Parameters Lesson2::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputDataArrayPath_Key, "InputDataArrayPath", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "OutputDataArrayPath", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_Value_Key, "Value", "", 1.23345f));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Lesson2::clone() const
{
  return std::make_unique<Lesson2>();
}

//------------------------------------------------------------------------------
Result<OutputActions> Lesson2::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto action = std::make_unique<Lesson2Action>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> Lesson2::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
