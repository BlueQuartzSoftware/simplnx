#include "FeatureFaceCurvatureFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FeatureFaceCurvatureFilter::name() const
{
  return FilterTraits<FeatureFaceCurvatureFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FeatureFaceCurvatureFilter::className() const
{
  return FilterTraits<FeatureFaceCurvatureFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FeatureFaceCurvatureFilter::uuid() const
{
  return FilterTraits<FeatureFaceCurvatureFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FeatureFaceCurvatureFilter::humanName() const
{
  return "Find Feature Face Curvature";
}

//------------------------------------------------------------------------------
std::vector<std::string> FeatureFaceCurvatureFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Curvature"};
}

//------------------------------------------------------------------------------
Parameters FeatureFaceCurvatureFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_NRing_Key, "Neighborhood Ring Count", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputePrincipalDirectionVectors_Key, "Compute Principal Direction Vectors", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeGaussianCurvature_Key, "Compute Gaussian Curvature", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeMeanCurvature_Key, "Compute Mean Curvature", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNormalsForCurveFitting_Key, "Use Face Normals for Curve Fitting", "", false));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FaceAttributeMatrixPath_Key, "Face Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key, "Feature Face Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshTriangleCentroidsArrayPath_Key, "Face Centroids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshPrincipalCurvature1sArrayName_Key, "Principal Curvature 1", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshPrincipalCurvature2sArrayName_Key, "Principal Curvature 2", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshPrincipalDirection1sArrayName_Key, "Principal Direction 1", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshPrincipalDirection2sArrayName_Key, "Principal Direction 2", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshGaussianCurvaturesArrayName_Key, "Gaussian Curvature", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshMeanCurvaturesArrayName_Key, "Mean Curvature", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ComputePrincipalDirectionVectors_Key, k_SurfaceMeshPrincipalCurvature1sArrayName_Key, true);
  params.linkParameters(k_ComputePrincipalDirectionVectors_Key, k_SurfaceMeshPrincipalCurvature2sArrayName_Key, true);
  params.linkParameters(k_ComputePrincipalDirectionVectors_Key, k_SurfaceMeshPrincipalDirection1sArrayName_Key, true);
  params.linkParameters(k_ComputePrincipalDirectionVectors_Key, k_SurfaceMeshPrincipalDirection2sArrayName_Key, true);
  params.linkParameters(k_ComputeGaussianCurvature_Key, k_SurfaceMeshGaussianCurvaturesArrayName_Key, true);
  params.linkParameters(k_ComputeMeanCurvature_Key, k_SurfaceMeshMeanCurvaturesArrayName_Key, true);
  params.linkParameters(k_UseNormalsForCurveFitting_Key, k_SurfaceMeshFaceNormalsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FeatureFaceCurvatureFilter::clone() const
{
  return std::make_unique<FeatureFaceCurvatureFilter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FeatureFaceCurvatureFilter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pNRingValue = filterArgs.value<int32>(k_NRing_Key);
  auto pComputePrincipalDirectionVectorsValue = filterArgs.value<bool>(k_ComputePrincipalDirectionVectors_Key);
  auto pComputeGaussianCurvatureValue = filterArgs.value<bool>(k_ComputeGaussianCurvature_Key);
  auto pComputeMeanCurvatureValue = filterArgs.value<bool>(k_ComputeMeanCurvature_Key);
  auto pUseNormalsForCurveFittingValue = filterArgs.value<bool>(k_UseNormalsForCurveFitting_Key);
  auto pFaceAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshTriangleCentroidsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleCentroidsArrayPath_Key);
  auto pSurfaceMeshPrincipalCurvature1sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalCurvature1sArrayName_Key);
  auto pSurfaceMeshPrincipalCurvature2sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalCurvature2sArrayName_Key);
  auto pSurfaceMeshPrincipalDirection1sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalDirection1sArrayName_Key);
  auto pSurfaceMeshPrincipalDirection2sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalDirection2sArrayName_Key);
  auto pSurfaceMeshGaussianCurvaturesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshGaussianCurvaturesArrayName_Key);
  auto pSurfaceMeshMeanCurvaturesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshMeanCurvaturesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FeatureFaceCurvatureFilterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FeatureFaceCurvatureFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNRingValue = filterArgs.value<int32>(k_NRing_Key);
  auto pComputePrincipalDirectionVectorsValue = filterArgs.value<bool>(k_ComputePrincipalDirectionVectors_Key);
  auto pComputeGaussianCurvatureValue = filterArgs.value<bool>(k_ComputeGaussianCurvature_Key);
  auto pComputeMeanCurvatureValue = filterArgs.value<bool>(k_ComputeMeanCurvature_Key);
  auto pUseNormalsForCurveFittingValue = filterArgs.value<bool>(k_UseNormalsForCurveFitting_Key);
  auto pFaceAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FaceAttributeMatrixPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFeatureFaceIdsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshTriangleCentroidsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleCentroidsArrayPath_Key);
  auto pSurfaceMeshPrincipalCurvature1sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalCurvature1sArrayName_Key);
  auto pSurfaceMeshPrincipalCurvature2sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalCurvature2sArrayName_Key);
  auto pSurfaceMeshPrincipalDirection1sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalDirection1sArrayName_Key);
  auto pSurfaceMeshPrincipalDirection2sArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshPrincipalDirection2sArrayName_Key);
  auto pSurfaceMeshGaussianCurvaturesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshGaussianCurvaturesArrayName_Key);
  auto pSurfaceMeshMeanCurvaturesArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshMeanCurvaturesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
