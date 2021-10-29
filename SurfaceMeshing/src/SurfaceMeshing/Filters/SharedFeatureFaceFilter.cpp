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
Result<OutputActions> SharedFeatureFaceFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayName_Key);
  auto pFaceFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceFeatureAttributeMatrixName_Key);
  auto pSurfaceMeshFeatureFaceLabelsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayName_Key);
  auto pSurfaceMeshFeatureFaceNumTrianglesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceNumTrianglesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<SharedFeatureFaceFilterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
