#include "DiscretizeDDDomain.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
std::string DiscretizeDDDomain::name() const
{
  return FilterTraits<DiscretizeDDDomain>::name.str();
}

std::string DiscretizeDDDomain::className() const
{
  return FilterTraits<DiscretizeDDDomain>::className;
}

Uuid DiscretizeDDDomain::uuid() const
{
  return FilterTraits<DiscretizeDDDomain>::uuid;
}

std::string DiscretizeDDDomain::humanName() const
{
  return "Discretize DDD Domain";
}

Parameters DiscretizeDDDomain::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_CellSize_Key, "Cell Size (Microns)", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_EdgeDataContainerName_Key, "Edge Data Container", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_OutputDataContainerName_Key, "Volume Data Container", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArrayName_Key, "Dislocation Line Density Array Name", "", DataPath{}));

  return params;
}

IFilter::UniquePointer DiscretizeDDDomain::clone() const
{
  return std::make_unique<DiscretizeDDDomain>();
}

Result<OutputActions> DiscretizeDDDomain::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCellSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pOutputDataContainerNameValue = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<DiscretizeDDDomainAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> DiscretizeDDDomain::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCellSizeValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pOutputDataContainerNameValue = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  auto pOutputAttributeMatrixNameValue = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
