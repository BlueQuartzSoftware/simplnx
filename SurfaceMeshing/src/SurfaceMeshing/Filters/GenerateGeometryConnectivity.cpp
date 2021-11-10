#include "GenerateGeometryConnectivity.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateGeometryConnectivity::name() const
{
  return FilterTraits<GenerateGeometryConnectivity>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateGeometryConnectivity::className() const
{
  return FilterTraits<GenerateGeometryConnectivity>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateGeometryConnectivity::uuid() const
{
  return FilterTraits<GenerateGeometryConnectivity>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateGeometryConnectivity::humanName() const
{
  return "Generate Geometry Connectivity";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateGeometryConnectivity::defaultTags() const
{
  return {"#Surface Meshing", "#Connectivity Arrangement"};
}

//------------------------------------------------------------------------------
Parameters GenerateGeometryConnectivity::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_GenerateVertexTriangleLists_Key, "Generate Per Vertex Element List", "", false));
  params.insert(std::make_unique<BoolParameter>(k_GenerateTriangleNeighbors_Key, "Generate Element Neighbors List", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SurfaceDataContainerName_Key, "Data Container", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateGeometryConnectivity::clone() const
{
  return std::make_unique<GenerateGeometryConnectivity>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateGeometryConnectivity::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pGenerateVertexTriangleListsValue = filterArgs.value<bool>(k_GenerateVertexTriangleLists_Key);
  auto pGenerateTriangleNeighborsValue = filterArgs.value<bool>(k_GenerateTriangleNeighbors_Key);
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateGeometryConnectivityAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> GenerateGeometryConnectivity::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
