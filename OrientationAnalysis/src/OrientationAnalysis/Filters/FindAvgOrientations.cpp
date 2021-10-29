#include "FindAvgOrientations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindAvgOrientations::name() const
{
  return FilterTraits<FindAvgOrientations>::name.str();
}

//------------------------------------------------------------------------------
std::string FindAvgOrientations::className() const
{
  return FilterTraits<FindAvgOrientations>::className;
}

//------------------------------------------------------------------------------
Uuid FindAvgOrientations::uuid() const
{
  return FilterTraits<FindAvgOrientations>::uuid;
}

//------------------------------------------------------------------------------
std::string FindAvgOrientations::humanName() const
{
  return "Find Feature Average Orientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindAvgOrientations::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindAvgOrientations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AvgEulerAnglesArrayPath_Key, "Average Euler Angles", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindAvgOrientations::clone() const
{
  return std::make_unique<FindAvgOrientations>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindAvgOrientations::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pAvgEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_AvgEulerAnglesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindAvgOrientationsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindAvgOrientations::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pAvgEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_AvgEulerAnglesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
