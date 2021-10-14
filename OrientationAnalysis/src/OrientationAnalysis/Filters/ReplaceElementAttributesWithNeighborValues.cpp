#include "ReplaceElementAttributesWithNeighborValues.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
std::string ReplaceElementAttributesWithNeighborValues::name() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValues>::name.str();
}

Uuid ReplaceElementAttributesWithNeighborValues::uuid() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValues>::uuid;
}

std::string ReplaceElementAttributesWithNeighborValues::humanName() const
{
  return "Replace Element Attributes with Neighbor (Threshold)";
}

Parameters ReplaceElementAttributesWithNeighborValues::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_MinConfidence_Key, "Threshold Value", "", 1.23345f));
  params.insert(std::make_unique<ChoicesParameter>(k_SelectedComparison_Key, "Comparison Operator", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConfidenceIndexArrayPath_Key, "Comparison Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ReplaceElementAttributesWithNeighborValues::clone() const
{
  return std::make_unique<ReplaceElementAttributesWithNeighborValues>();
}

Result<OutputActions> ReplaceElementAttributesWithNeighborValues::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMinConfidenceValue = filterArgs.value<float32>(k_MinConfidence_Key);
  auto pSelectedComparisonValue = filterArgs.value<ChoicesParameter::ValueType>(k_SelectedComparison_Key);
  auto pLoopValue = filterArgs.value<bool>(k_Loop_Key);
  auto pConfidenceIndexArrayPathValue = filterArgs.value<DataPath>(k_ConfidenceIndexArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReplaceElementAttributesWithNeighborValuesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ReplaceElementAttributesWithNeighborValues::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMinConfidenceValue = filterArgs.value<float32>(k_MinConfidence_Key);
  auto pSelectedComparisonValue = filterArgs.value<ChoicesParameter::ValueType>(k_SelectedComparison_Key);
  auto pLoopValue = filterArgs.value<bool>(k_Loop_Key);
  auto pConfidenceIndexArrayPathValue = filterArgs.value<DataPath>(k_ConfidenceIndexArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
