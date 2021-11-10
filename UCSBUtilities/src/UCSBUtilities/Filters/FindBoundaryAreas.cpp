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

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSurfaceMeshTriangleAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleAreasArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key);
  auto pSurfaceMeshBoundaryAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshBoundaryAreasArrayPath_Key);

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
  auto action = std::make_unique<FindBoundaryAreasAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
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
