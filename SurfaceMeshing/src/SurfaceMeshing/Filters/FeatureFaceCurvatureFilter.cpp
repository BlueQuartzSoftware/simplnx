#include "FeatureFaceCurvatureFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
IFilter::PreflightResult FeatureFaceCurvatureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
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

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
