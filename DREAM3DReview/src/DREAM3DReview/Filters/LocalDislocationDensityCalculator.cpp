#include "LocalDislocationDensityCalculator.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
std::string LocalDislocationDensityCalculator::name() const
{
  return FilterTraits<LocalDislocationDensityCalculator>::name.str();
}

std::string LocalDislocationDensityCalculator::className() const
{
  return FilterTraits<LocalDislocationDensityCalculator>::className;
}

Uuid LocalDislocationDensityCalculator::uuid() const
{
  return FilterTraits<LocalDislocationDensityCalculator>::uuid;
}

std::string LocalDislocationDensityCalculator::humanName() const
{
  return "Calculate Local Dislocation Densities";
}

Parameters LocalDislocationDensityCalculator::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_CellSize_Key, "Cell Size (Microns)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_EdgeDataContainerName_Key, "Edge DataContainer", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BurgersVectorsArrayPath_Key, "Burgers Vectors Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SlipPlaneNormalsArrayPath_Key, "Slip Plane Normals Array", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputDataContainerName_Key, "Volume Data Container", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputAttributeMatrixName_Key, "Cell AttributeMatrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArrayName_Key, "Dislocation Line Density Array Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DominantSystemArrayName_Key, "Dominant System Array Name", "", DataPath{}));

  return params;
}

IFilter::UniquePointer LocalDislocationDensityCalculator::clone() const
{
  return std::make_unique<LocalDislocationDensityCalculator>();
}

Result<OutputActions> LocalDislocationDensityCalculator::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCellSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pBurgersVectorsArrayPathValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  auto pSlipPlaneNormalsArrayPathValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  auto pOutputDataContainerNameValue = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);
  auto pDominantSystemArrayNameValue = filterArgs.value<DataPath>(k_DominantSystemArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<LocalDislocationDensityCalculatorAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> LocalDislocationDensityCalculator::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCellSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pBurgersVectorsArrayPathValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  auto pSlipPlaneNormalsArrayPathValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  auto pOutputDataContainerNameValue = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);
  auto pDominantSystemArrayNameValue = filterArgs.value<DataPath>(k_DominantSystemArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
