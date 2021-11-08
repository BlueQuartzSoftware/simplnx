#include "FindBoundaryAreas.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundaryAreas::name() const
{
  return FilterTraits<FindBoundaryAreas>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundaryAreas::className() const
{
  return FilterTraits<FindBoundaryAreas>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundaryAreas::uuid() const
{
  return FilterTraits<FindBoundaryAreas>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundaryAreas::humanName() const
{
  return "Find Face Feature Boundary Areas";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundaryAreas::defaultTags() const
{
  return {"#Surface Meshing", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters FindBoundaryAreas::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshTriangleAreasArrayPath_Key, "Triangle Areas", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key, "Face Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshBoundaryAreasArrayPath_Key, "Boundary Areas Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundaryAreas::clone() const
{
  return std::make_unique<FindBoundaryAreas>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundaryAreas::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshTriangleAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleAreasArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key);
  auto pSurfaceMeshBoundaryAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshBoundaryAreasArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindBoundaryAreasAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindBoundaryAreas::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshTriangleAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleAreasArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key);
  auto pSurfaceMeshBoundaryAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshBoundaryAreasArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
