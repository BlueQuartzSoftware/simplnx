#include "FeatureFaceCurvatureFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/FeatureFaceCurvature.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
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
  return "Compute Feature Face Curvature";
}

//------------------------------------------------------------------------------
std::vector<std::string> FeatureFaceCurvatureFilter::defaultTags() const
{
  return {"Processing", "Cleanup"};
}

//------------------------------------------------------------------------------
Parameters FeatureFaceCurvatureFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeomPath_Key, "Triangle Geometry", "The input Triangle Geometry to compute the curvature values", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<Int32Parameter>(k_NeighborhoodRing_Key, "Neighborhood Ring Count", "The number of ring neighbors to use", 1));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputePrincipalDirection_Key, "Compute Principal Direction Vectors", "Compute the Principal Direction Vectors", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeGaussianCurvature_Key, "Compute Gaussian Curvature", "Compute the Gaussian Curvature values", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeMeanCurvaturePath_Key, "Compute Mean Curvature", "Compute the Mean Curvature values", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeWeingartenMatrix_Key, "Compute Weingarten Matrix", "Compute the Weingarten Matrix values", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseFaceNormals_Key, "Use Face Normals for Curve Fitting", "Use the face normals for curve fitting.", true));

  params.insertSeparator(Parameters::Separator{"Input Triangle Face Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_FaceAttribMatrixPath_Key, "Face Attribute Matrix", "The AttributeMatrix that holds the triangle face data.", DataPath()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsPath_Key, "Face Labels", "The DataPath to the 'Face Labels' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureFaceIdsPath_Key, "Feature Face Ids", "The DataPath to the 'FeatureIds' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceNormalsPath_Key, "Face Normals", "The DataPath to the 'Feature Normals' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceCentroidsPath_Key, "Face Centroids", "The DataPath to the 'Face Centroids' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{3}}));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalCurvature1Path_Key, "Principal Curvature 1", "Output DataPath to hold the 'Principal Curvature 1' values",
                                                         DataPath({"Principal Curvature 1"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalCurvature2Path_Key, "Principal Curvature 2", "Output DataPath to hold the 'Principal Curvature 2' values",
                                                         DataPath({"Principal Curvature 2"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalDirection1Path_Key, "Principal Direction 1", "Output DataPath to hold the 'Principal Direction 1' values",
                                                         DataPath({"Principal Direction 1"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalDirection2Path_Key, "Principal Direction 2", "Output DataPath to hold the 'Principal Direction 2' values",
                                                         DataPath({"Principal Direction 2"})));
  params.insert(
      std::make_unique<ArrayCreationParameter>(k_GaussianCurvaturePath_Key, "Gaussian Curvature", "Output DataPath to hold the 'Gaussian Curvature' values", DataPath({"Gaussian Curvature"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MeanCurvaturePath_Key, "Mean Curvature", "Output DataPath to hold the 'Mean Curvature' values", DataPath({"Mean Curvature"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_WeingartenMatrixPath_Key, "Weingarten Matrix", "Output DataPath to hold the 'Weingarten Matrix' values", DataPath({"Weingarten Matrix"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalCurvature1Path_Key, true);
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalCurvature2Path_Key, true);
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalDirection1Path_Key, true);
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalDirection2Path_Key, true);
  params.linkParameters(k_ComputeGaussianCurvature_Key, k_GaussianCurvaturePath_Key, true);
  params.linkParameters(k_ComputeMeanCurvaturePath_Key, k_MeanCurvaturePath_Key, true);
  params.linkParameters(k_ComputeWeingartenMatrix_Key, k_WeingartenMatrixPath_Key, true);
  params.linkParameters(k_UseFaceNormals_Key, k_FaceNormalsPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FeatureFaceCurvatureFilter::clone() const
{
  return std::make_unique<FeatureFaceCurvatureFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FeatureFaceCurvatureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto triangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  auto surfaceMeshPrincipalCurvature1Path = filterArgs.value<DataPath>(k_PrincipalCurvature1Path_Key);
  auto surfaceMeshFeatureFaceIdsPath = filterArgs.value<DataPath>(k_FeatureFaceIdsPath_Key);
  auto surfaceMeshFaceLabelsPath = filterArgs.value<DataPath>(k_FaceLabelsPath_Key);
  auto surfaceMeshFaceNormalsPath = filterArgs.value<DataPath>(k_FaceNormalsPath_Key);
  auto surfaceMeshTriangleCentroidsPath = filterArgs.value<DataPath>(k_FaceCentroidsPath_Key);

  auto computePrincipalDirection = filterArgs.value<bool>(k_ComputePrincipalDirection_Key);
  auto computeGaussianCurvature = filterArgs.value<bool>(k_ComputeGaussianCurvature_Key);
  auto computeMeanCurvature = filterArgs.value<bool>(k_ComputeMeanCurvaturePath_Key);
  auto computeWeingartenMatrix = filterArgs.value<bool>(k_ComputeWeingartenMatrix_Key);

  auto surfaceMeshPrincipalCurvature1sPath = filterArgs.value<DataPath>(k_PrincipalCurvature1Path_Key);
  auto surfaceMeshPrincipalCurvature2sPath = filterArgs.value<DataPath>(k_PrincipalCurvature2Path_Key);
  auto surfaceMeshPrincipalDirection1sPath = filterArgs.value<DataPath>(k_PrincipalDirection1Path_Key);
  auto surfaceMeshPrincipalDirection2sPath = filterArgs.value<DataPath>(k_PrincipalDirection2Path_Key);
  auto surfaceMeshMeanCurvaturesPath = filterArgs.value<DataPath>(k_MeanCurvaturePath_Key);
  auto surfaceMeshGaussianCurvaturesPath = filterArgs.value<DataPath>(k_GaussianCurvaturePath_Key);
  auto surfaceMeshWeingartenMatrixPath = filterArgs.value<DataPath>(k_WeingartenMatrixPath_Key);

  std::vector<DataPath> dataArrays;

  const auto* triangles = dataStructure.getDataAs<TriangleGeom>(triangleGeomPath);
  const DataPath triangleMatrixPath = triangles->getFaceAttributeMatrixDataPath();
  const auto* triangleMatrix = dataStructure.getDataAs<AttributeMatrix>(triangleMatrixPath);

  {
    dataArrays.push_back(surfaceMeshPrincipalCurvature1Path);
    dataArrays.push_back(surfaceMeshFeatureFaceIdsPath);
    dataArrays.push_back(surfaceMeshFaceLabelsPath);
    dataArrays.push_back(surfaceMeshFaceNormalsPath);
    dataArrays.push_back(surfaceMeshTriangleCentroidsPath);
  }

  // auto* dataArray = dataStructure.getDataAs<IDataArray>(triangles->getFaceAttributeMatrixDataPath());
  IArray::ShapeType tupleShape = triangleMatrix->getShape();

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, std::vector<usize>{1}, surfaceMeshPrincipalCurvature1sPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, std::vector<usize>{1}, surfaceMeshPrincipalCurvature2sPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  if(computeGaussianCurvature)
  {
    {
      auto action = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, std::vector<usize>{1}, surfaceMeshGaussianCurvaturesPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  if(computeMeanCurvature)
  {
    {
      auto action = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, std::vector<usize>{1}, surfaceMeshMeanCurvaturesPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  if(computeWeingartenMatrix)
  {
    {
      auto action = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, std::vector<usize>{4}, surfaceMeshWeingartenMatrixPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  if(computePrincipalDirection)
  {
    {
      auto action = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, std::vector<usize>{3}, surfaceMeshPrincipalDirection1sPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    {
      auto action = std::make_unique<CreateArrayAction>(DataType::float64, tupleShape, std::vector<usize>{3}, surfaceMeshPrincipalDirection2sPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> FeatureFaceCurvatureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  FeatureFaceCurvatureInputValues inputValues;

  inputValues.NRingCount = filterArgs.value<int32>(k_NeighborhoodRing_Key);
  inputValues.triangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  inputValues.surfaceMeshPrincipalCurvature1Path = filterArgs.value<DataPath>(k_PrincipalCurvature1Path_Key);
  inputValues.surfaceMeshFeatureFaceIdsPath = filterArgs.value<DataPath>(k_FeatureFaceIdsPath_Key);
  inputValues.surfaceMeshFaceLabelsPath = filterArgs.value<DataPath>(k_FaceLabelsPath_Key);
  inputValues.surfaceMeshFaceNormalsPath = filterArgs.value<DataPath>(k_FaceNormalsPath_Key);
  inputValues.surfaceMeshTriangleCentroidsPath = filterArgs.value<DataPath>(k_FaceCentroidsPath_Key);

  inputValues.useNormalsForCurveFitting = filterArgs.value<bool>(k_UseFaceNormals_Key);
  inputValues.surfaceMeshPrincipalCurvature1sPath = filterArgs.value<DataPath>(k_PrincipalCurvature1Path_Key);
  inputValues.surfaceMeshPrincipalCurvature2sPath = filterArgs.value<DataPath>(k_PrincipalCurvature2Path_Key);
  inputValues.surfaceMeshPrincipalDirection1sPath = filterArgs.value<DataPath>(k_PrincipalDirection1Path_Key);
  inputValues.surfaceMeshPrincipalDirection2sPath = filterArgs.value<DataPath>(k_PrincipalDirection2Path_Key);
  inputValues.surfaceMeshMeanCurvaturesPath = filterArgs.value<DataPath>(k_MeanCurvaturePath_Key);
  inputValues.surfaceMeshGaussianCurvaturesPath = filterArgs.value<DataPath>(k_GaussianCurvaturePath_Key);
  inputValues.surfaceMeshWeingartenMatrixPath = filterArgs.value<DataPath>(k_WeingartenMatrixPath_Key);

  return FeatureFaceCurvature(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
