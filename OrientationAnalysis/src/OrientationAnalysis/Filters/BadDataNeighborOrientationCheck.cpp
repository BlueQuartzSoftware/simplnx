#include "BadDataNeighborOrientationCheck.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheck::name() const
{
  return FilterTraits<BadDataNeighborOrientationCheck>::name.str();
}

//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheck::className() const
{
  return FilterTraits<BadDataNeighborOrientationCheck>::className;
}

//------------------------------------------------------------------------------
Uuid BadDataNeighborOrientationCheck::uuid() const
{
  return FilterTraits<BadDataNeighborOrientationCheck>::uuid;
}

//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheck::humanName() const
{
  return "Neighbor Orientation Comparison (Bad Data)";
}

//------------------------------------------------------------------------------
std::vector<std::string> BadDataNeighborOrientationCheck::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters BadDataNeighborOrientationCheck::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "", 1.23345f));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfNeighbors_Key, "Required Number of Neighbors", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer BadDataNeighborOrientationCheck::clone() const
{
  return std::make_unique<BadDataNeighborOrientationCheck>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult BadDataNeighborOrientationCheck::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pNumberOfNeighborsValue = filterArgs.value<int32>(k_NumberOfNeighbors_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<BadDataNeighborOrientationCheckAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> BadDataNeighborOrientationCheck::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pNumberOfNeighborsValue = filterArgs.value<int32>(k_NumberOfNeighbors_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
