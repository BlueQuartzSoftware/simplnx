#include "QuickSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string QuickSurfaceMesh::name() const
{
  return FilterTraits<QuickSurfaceMesh>::name.str();
}

//------------------------------------------------------------------------------
std::string QuickSurfaceMesh::className() const
{
  return FilterTraits<QuickSurfaceMesh>::className;
}

//------------------------------------------------------------------------------
Uuid QuickSurfaceMesh::uuid() const
{
  return FilterTraits<QuickSurfaceMesh>::uuid;
}

//------------------------------------------------------------------------------
std::string QuickSurfaceMesh::humanName() const
{
  return "Quick Surface Mesh";
}

//------------------------------------------------------------------------------
std::vector<std::string> QuickSurfaceMesh::defaultTags() const
{
  return {"#Surface Meshing", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters QuickSurfaceMesh::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_FixProblemVoxels_Key, "Attempt to Fix Problem Voxels", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(
      std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Transfer", "", MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_SurfaceDataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NodeTypesArrayName_Key, "Node Types", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceAttributeMatrixName_Key, "Face Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceLabelsArrayName_Key, "Face Labels", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureAttributeMatrixName_Key, "Face Feature Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer QuickSurfaceMesh::clone() const
{
  return std::make_unique<QuickSurfaceMesh>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult QuickSurfaceMesh::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFixProblemVoxelsValue = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pNodeTypesArrayNameValue = filterArgs.value<DataPath>(k_NodeTypesArrayName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  auto pFaceLabelsArrayNameValue = filterArgs.value<DataPath>(k_FaceLabelsArrayName_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);

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
  auto action = std::make_unique<QuickSurfaceMeshAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> QuickSurfaceMesh::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFixProblemVoxelsValue = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pSurfaceDataContainerNameValue = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pNodeTypesArrayNameValue = filterArgs.value<DataPath>(k_NodeTypesArrayName_Key);
  auto pFaceAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  auto pFaceLabelsArrayNameValue = filterArgs.value<DataPath>(k_FaceLabelsArrayName_Key);
  auto pFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
