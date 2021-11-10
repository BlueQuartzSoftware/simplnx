#include "SharedFeatureFaceFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string SharedFeatureFaceFilter::name() const
{
  return FilterTraits<SharedFeatureFaceFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string SharedFeatureFaceFilter::className() const
{
  return FilterTraits<SharedFeatureFaceFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SharedFeatureFaceFilter::uuid() const
{
  return FilterTraits<SharedFeatureFaceFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SharedFeatureFaceFilter::humanName() const
{
  return "Generate Triangle Face Ids";
}

//------------------------------------------------------------------------------
std::vector<std::string> SharedFeatureFaceFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Connectivity Arrangement"};
}

//------------------------------------------------------------------------------
Parameters SharedFeatureFaceFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshFeatureFaceIdsArrayName_Key, "Feature Face Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceFeatureAttributeMatrixName_Key, "Face Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshFeatureFaceLabelsArrayName_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshFeatureFaceNumTrianglesArrayName_Key, "Number of Triangles", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SharedFeatureFaceFilter::clone() const
{
  return std::make_unique<SharedFeatureFaceFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SharedFeatureFaceFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayName_Key);
  auto pFaceFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceFeatureAttributeMatrixName_Key);
  auto pSurfaceMeshFeatureFaceLabelsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayName_Key);
  auto pSurfaceMeshFeatureFaceNumTrianglesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceNumTrianglesArrayName_Key);

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
  auto action = std::make_unique<SharedFeatureFaceFilterAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> SharedFeatureFaceFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayName_Key);
  auto pFaceFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceFeatureAttributeMatrixName_Key);
  auto pSurfaceMeshFeatureFaceLabelsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayName_Key);
  auto pSurfaceMeshFeatureFaceNumTrianglesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceNumTrianglesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
