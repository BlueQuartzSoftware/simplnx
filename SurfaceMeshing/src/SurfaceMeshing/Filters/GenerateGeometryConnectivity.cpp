#include "GenerateGeometryConnectivity.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string GenerateGeometryConnectivity::name() const
{
  return FilterTraits<GenerateGeometryConnectivity>::name.str();
}

std::string GenerateGeometryConnectivity::className() const
{
  return FilterTraits<GenerateGeometryConnectivity>::className;
}

Uuid GenerateGeometryConnectivity::uuid() const
{
  return FilterTraits<GenerateGeometryConnectivity>::uuid;
}

std::string GenerateGeometryConnectivity::humanName() const
{
  return "Generate Geometry Connectivity";
}

Parameters GenerateGeometryConnectivity::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_GenerateVertexTriangleLists_Key, "Generate Per Vertex Element List", "", false));
  params.insert(std::make_unique<BoolParameter>(k_GenerateTriangleNeighbors_Key, "Generate Element Neighbors List", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SurfaceDataContainerName_Key, "Data Container", "", DataPath{}));

  return params;
}

IFilter::UniquePointer GenerateGeometryConnectivity::clone() const
{
  return std::make_unique<GenerateGeometryConnectivity>();
}

Result<OutputActions> GenerateGeometryConnectivity::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pGenerateVertexTriangleListsValue = filterArgs.value<bool>(k_GenerateVertexTriangleLists_Key);
  auto pGenerateTriangleNeighborsValue = filterArgs.value<bool>(k_GenerateTriangleNeighbors_Key);
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateGeometryConnectivityAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> GenerateGeometryConnectivity::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pGenerateVertexTriangleListsValue = filterArgs.value<bool>(k_GenerateVertexTriangleLists_Key);
  auto pGenerateTriangleNeighborsValue = filterArgs.value<bool>(k_GenerateTriangleNeighbors_Key);
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
