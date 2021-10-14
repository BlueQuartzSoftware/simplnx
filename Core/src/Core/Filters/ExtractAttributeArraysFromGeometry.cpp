#include "ExtractAttributeArraysFromGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string ExtractAttributeArraysFromGeometry::name() const
{
  return FilterTraits<ExtractAttributeArraysFromGeometry>::name.str();
}

Uuid ExtractAttributeArraysFromGeometry::uuid() const
{
  return FilterTraits<ExtractAttributeArraysFromGeometry>::uuid;
}

std::string ExtractAttributeArraysFromGeometry::humanName() const
{
  return "Extract Attribute Arrays from Geometry";
}

Parameters ExtractAttributeArraysFromGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_XBoundsArrayPath_Key, "X Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_YBoundsArrayPath_Key, "Y Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ZBoundsArrayPath_Key, "Z Bounds", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath0_Key, "Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath1_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedEdgeListArrayPath_Key, "Edge List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath2_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedTriListArrayPath_Key, "Triangle List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath3_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedQuadListArrayPath_Key, "Quadrilateral List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath4_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedTetListArrayPath_Key, "Tetrahedral List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedVertexListArrayPath5_Key, "Shared Vertex List", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SharedHexListArrayPath_Key, "Hexahedral List", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ExtractAttributeArraysFromGeometry::clone() const
{
  return std::make_unique<ExtractAttributeArraysFromGeometry>();
}

Result<OutputActions> ExtractAttributeArraysFromGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pXBoundsArrayPathValue = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  auto pYBoundsArrayPathValue = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  auto pZBoundsArrayPathValue = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  auto pSharedVertexListArrayPath0Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  auto pSharedVertexListArrayPath1Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  auto pSharedEdgeListArrayPathValue = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  auto pSharedVertexListArrayPath2Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  auto pSharedTriListArrayPathValue = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  auto pSharedVertexListArrayPath3Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  auto pSharedQuadListArrayPathValue = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  auto pSharedVertexListArrayPath4Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  auto pSharedTetListArrayPathValue = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  auto pSharedVertexListArrayPath5Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  auto pSharedHexListArrayPathValue = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ExtractAttributeArraysFromGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ExtractAttributeArraysFromGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pXBoundsArrayPathValue = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  auto pYBoundsArrayPathValue = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  auto pZBoundsArrayPathValue = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  auto pSharedVertexListArrayPath0Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  auto pSharedVertexListArrayPath1Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  auto pSharedEdgeListArrayPathValue = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  auto pSharedVertexListArrayPath2Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  auto pSharedTriListArrayPathValue = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  auto pSharedVertexListArrayPath3Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  auto pSharedQuadListArrayPathValue = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  auto pSharedVertexListArrayPath4Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  auto pSharedTetListArrayPathValue = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  auto pSharedVertexListArrayPath5Value = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  auto pSharedHexListArrayPathValue = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
