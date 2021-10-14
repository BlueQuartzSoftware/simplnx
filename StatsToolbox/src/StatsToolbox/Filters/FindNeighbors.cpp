#include "FindNeighbors.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindNeighbors::name() const
{
  return FilterTraits<FindNeighbors>::name.str();
}

std::string FindNeighbors::className() const
{
  return FilterTraits<FindNeighbors>::className;
}

Uuid FindNeighbors::uuid() const
{
  return FilterTraits<FindNeighbors>::uuid;
}

std::string FindNeighbors::humanName() const
{
  return "Find Feature Neighbors";
}

Parameters FindNeighbors::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreBoundaryCells_Key, "Store Boundary Cells Array", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreSurfaceFeatures_Key, "Store Surface Features Array", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_BoundaryCellsArrayName_Key, "Boundary Cells", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumNeighborsArrayName_Key, "Number of Neighbors", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NeighborListArrayName_Key, "Neighbor List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedSurfaceAreaListArrayName_Key, "Shared Surface Area List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceFeaturesArrayName_Key, "Surface Features", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_StoreBoundaryCells_Key, k_BoundaryCellsArrayName_Key, true);
  params.linkParameters(k_StoreSurfaceFeatures_Key, k_SurfaceFeaturesArrayName_Key, true);

  return params;
}

IFilter::UniquePointer FindNeighbors::clone() const
{
  return std::make_unique<FindNeighbors>();
}

Result<OutputActions> FindNeighbors::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pStoreBoundaryCellsValue = filterArgs.value<bool>(k_StoreBoundaryCells_Key);
  auto pStoreSurfaceFeaturesValue = filterArgs.value<bool>(k_StoreSurfaceFeatures_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pBoundaryCellsArrayNameValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayName_Key);
  auto pNumNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NumNeighborsArrayName_Key);
  auto pNeighborListArrayNameValue = filterArgs.value<DataPath>(k_NeighborListArrayName_Key);
  auto pSharedSurfaceAreaListArrayNameValue = filterArgs.value<DataPath>(k_SharedSurfaceAreaListArrayName_Key);
  auto pSurfaceFeaturesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindNeighborsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindNeighbors::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pStoreBoundaryCellsValue = filterArgs.value<bool>(k_StoreBoundaryCells_Key);
  auto pStoreSurfaceFeaturesValue = filterArgs.value<bool>(k_StoreSurfaceFeatures_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pBoundaryCellsArrayNameValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayName_Key);
  auto pNumNeighborsArrayNameValue = filterArgs.value<DataPath>(k_NumNeighborsArrayName_Key);
  auto pNeighborListArrayNameValue = filterArgs.value<DataPath>(k_NeighborListArrayName_Key);
  auto pSharedSurfaceAreaListArrayNameValue = filterArgs.value<DataPath>(k_SharedSurfaceAreaListArrayName_Key);
  auto pSurfaceFeaturesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
