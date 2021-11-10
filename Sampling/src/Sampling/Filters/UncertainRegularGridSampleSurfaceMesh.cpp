#include "UncertainRegularGridSampleSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMesh::name() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMesh>::name.str();
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMesh::className() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMesh>::className;
}

//------------------------------------------------------------------------------
Uuid UncertainRegularGridSampleSurfaceMesh::uuid() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMesh>::uuid;
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMesh::humanName() const
{
  return "Sample Triangle Geometry on Uncertain Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> UncertainRegularGridSampleSurfaceMesh::defaultTags() const
{
  return {"#Sampling", "#Spacing"};
}

//------------------------------------------------------------------------------
Parameters UncertainRegularGridSampleSurfaceMesh::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<Int32Parameter>(k_XPoints_Key, "X Points", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_YPoints_Key, "Y Points", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_ZPoints_Key, "Z Points", "", 1234356));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Uncertainty_Key, "Uncertainty", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer UncertainRegularGridSampleSurfaceMesh::clone() const
{
  return std::make_unique<UncertainRegularGridSampleSurfaceMesh>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult UncertainRegularGridSampleSurfaceMesh::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pXPointsValue = filterArgs.value<int32>(k_XPoints_Key);
  auto pYPointsValue = filterArgs.value<int32>(k_YPoints_Key);
  auto pZPointsValue = filterArgs.value<int32>(k_ZPoints_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pUncertaintyValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Uncertainty_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

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
  auto action = std::make_unique<UncertainRegularGridSampleSurfaceMeshAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> UncertainRegularGridSampleSurfaceMesh::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pXPointsValue = filterArgs.value<int32>(k_XPoints_Key);
  auto pYPointsValue = filterArgs.value<int32>(k_YPoints_Key);
  auto pZPointsValue = filterArgs.value<int32>(k_ZPoints_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pUncertaintyValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Uncertainty_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
