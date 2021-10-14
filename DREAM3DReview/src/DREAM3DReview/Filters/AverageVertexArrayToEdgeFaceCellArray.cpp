#include "AverageVertexArrayToEdgeFaceCellArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
std::string AverageVertexArrayToEdgeFaceCellArray::name() const
{
  return FilterTraits<AverageVertexArrayToEdgeFaceCellArray>::name.str();
}

Uuid AverageVertexArrayToEdgeFaceCellArray::uuid() const
{
  return FilterTraits<AverageVertexArrayToEdgeFaceCellArray>::uuid;
}

std::string AverageVertexArrayToEdgeFaceCellArray::humanName() const
{
  return "Average Vertex Array to Edge/Face/Cell Array";
}

Parameters AverageVertexArrayToEdgeFaceCellArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_WeightedAverage_Key, "Perform Weighted Average", "", false));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Vertex Array to Average", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge/Face/Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_AverageCellArrayPath_Key, "Average Edge/Face/Cell Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer AverageVertexArrayToEdgeFaceCellArray::clone() const
{
  return std::make_unique<AverageVertexArrayToEdgeFaceCellArray>();
}

Result<OutputActions> AverageVertexArrayToEdgeFaceCellArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pWeightedAverageValue = filterArgs.value<bool>(k_WeightedAverage_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pAverageCellArrayPathValue = filterArgs.value<DataPath>(k_AverageCellArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AverageVertexArrayToEdgeFaceCellArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> AverageVertexArrayToEdgeFaceCellArray::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWeightedAverageValue = filterArgs.value<bool>(k_WeightedAverage_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pAverageCellArrayPathValue = filterArgs.value<DataPath>(k_AverageCellArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
