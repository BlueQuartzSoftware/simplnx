#include "FeatureFaceCurvatureFilter.hpp"

#include "ComplexCore/Filters/Algorithms/CalculateTriangleGroupCurvatures.hpp"
#include "ComplexCore/Filters/Algorithms/FindNRingNeighbors.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

#include "fmt/format.h"

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
  return "Fill Bad Data";
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
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeom_Key, "Triangle Geometry", "The input Triangle Geometry to compute the curvature values", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<Int32Parameter>(k_NeighborhoodRing_Key, "Neighborhood Ring Count", "The number of ring neighbors to use", 1));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputePrincipalDirection_Key, "Compute Principal Direction Vectors", "Compute the Principal Direction Vectors", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeGaussianCurvature_Key, "Compute Gaussian Curvature", "Compute the Gaussian Curvature values", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeMeanCurvature_Key, "Compute Mean Curvature", "Compute the Mean Curvature values", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeWeingartenMatrix_Key, "Compute Weingarten Matrix", "Compute the Weingarten Matrix values", true));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseFaceNormals_Key, "Use Face Normals for Curve Fitting", "Use the face normals for curve fitting.", true));

  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_FaceAttribMatrix_Key, "Face Attribute Matrix", "The AttributeMatrix that holds the triangle face data.", DataPath()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabels_Key, "Face Labels", "The DataPath to the 'Face Labels' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureFaceIds_Key, "Feature Face IDs", "The DataPath to the 'FeatureIds' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceNormals_Key, "Face Normals", "The DataPath to the 'Feature Normals' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceCentroids_Key, "Face Centroids", "The DataPath to the 'Face Centroids' DataArray", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{IArray::ShapeType{3}}));

  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalCurvature1_Key, "Principal Curvature 1", "Output DataPath to hold the 'Principal Curvature 1' values", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalCurvature2_Key, "Principal Curvature 2", "Output DataPath to hold the 'Principal Curvature 2' values", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalDirection1_Key, "Principal Direction 1", "Output DataPath to hold the 'Principal Direction 1' values", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_PrincipalDirection2_Key, "Principal Direction 2", "Output DataPath to hold the 'Principal Direction 2' values", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_GaussianCurvature_Key, "Gaussian Curvature", "Output DataPath to hold the 'Gaussian Curvature' values", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MeanCurvature_Key, "Mean Curvature", "Output DataPath to hold the 'Mean Curvature' values", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_WeingartenMatrix_Key, "Weingarten Matrix", "Output DataPath to hold the 'Weingarten Matrix' values", DataPath()));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalCurvature1_Key, true);
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalCurvature2_Key, true);
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalDirection1_Key, true);
  params.linkParameters(k_ComputePrincipalDirection_Key, k_PrincipalDirection2_Key, true);
  params.linkParameters(k_ComputeGaussianCurvature_Key, k_GaussianCurvature_Key, true);
  params.linkParameters(k_ComputeMeanCurvature_Key, k_MeanCurvature_Key, true);
  params.linkParameters(k_ComputeWeingartenMatrix_Key, k_WeingartenMatrix_Key, true);
  params.linkParameters(k_UseFaceNormals_Key, k_FaceNormals_Key, true);

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
  auto triangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeom_Key);
  auto surfaceMeshPrincipalCurvature1Path = filterArgs.value<DataPath>(k_PrincipalCurvature1_Key);
  auto surfaceMeshFeatureFaceIdsPath = filterArgs.value<DataPath>(k_FeatureFaceIds_Key);
  auto surfaceMeshFaceLabelsPath = filterArgs.value<DataPath>(k_FaceLabels_Key);
  auto surfaceMeshFaceNormalsPath = filterArgs.value<DataPath>(k_FaceNormals_Key);
  auto surfaceMeshTriangleCentroidsPath = filterArgs.value<DataPath>(k_FaceCentroids_Key);

  auto computePrincipalCurvature = filterArgs.value<bool>(k_ComputePrincipalDirection_Key);
  auto computePrincipalDirection = filterArgs.value<bool>(k_ComputePrincipalDirection_Key);
  auto computeGaussianCurvature = filterArgs.value<bool>(k_ComputeGaussianCurvature_Key);
  auto computeMeanCurvature = filterArgs.value<bool>(k_ComputeMeanCurvature_Key);
  auto computeWeingartenMatrix = filterArgs.value<bool>(k_ComputeWeingartenMatrix_Key);

  auto surfaceMeshPrincipalCurvature1sPath = filterArgs.value<DataPath>(k_PrincipalCurvature1_Key);
  auto surfaceMeshPrincipalCurvature2sPath = filterArgs.value<DataPath>(k_PrincipalCurvature2_Key);
  auto surfaceMeshPrincipalDirection1sPath = filterArgs.value<DataPath>(k_PrincipalDirection1_Key);
  auto surfaceMeshPrincipalDirection2sPath = filterArgs.value<DataPath>(k_PrincipalDirection2_Key);
  auto surfaceMeshMeanCurvaturesPath = filterArgs.value<DataPath>(k_MeanCurvature_Key);
  auto surfaceMeshGaussianCurvaturesPath = filterArgs.value<DataPath>(k_GaussianCurvature_Key);
  auto surfaceMeshWeingartenMatrixPath = filterArgs.value<DataPath>(k_WeingartenMatrix_Key);

  std::vector<DataPath> dataArrays;

  const TriangleGeom* triangles = dataStructure.getDataAs<TriangleGeom>(triangleGeomPath);
  const DataPath triangleMatrixPath = triangles->getFaceAttributeMatrixDataPath();
  const AttributeMatrix* triangleMatrix = dataStructure.getDataAs<AttributeMatrix>(triangleMatrixPath);

  {
    dataArrays.push_back(surfaceMeshPrincipalCurvature1Path);
    dataArrays.push_back(surfaceMeshFeatureFaceIdsPath);
    dataArrays.push_back(surfaceMeshFaceLabelsPath);
    dataArrays.push_back(surfaceMeshFaceNormalsPath);
    dataArrays.push_back(surfaceMeshTriangleCentroidsPath);
  }

  // auto* dataArray = dataStructure.getDataAs<IDataArray>(triangles->getFaceAttributeMatrixDataPath());
  IArray::ShapeType tupleShape = triangleMatrix->getShape();

  complex::Result<OutputActions> resultOutputActions;
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
  int64 nRingCount = filterArgs.value<int32>(k_NeighborhoodRing_Key);
  auto triangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeom_Key);
  auto surfaceMeshPrincipalCurvature1Path = filterArgs.value<DataPath>(k_PrincipalCurvature1_Key);
  auto surfaceMeshFeatureFaceIdsPath = filterArgs.value<DataPath>(k_FeatureFaceIds_Key);
  auto surfaceMeshFaceLabelsPath = filterArgs.value<DataPath>(k_FaceLabels_Key);
  auto surfaceMeshFaceNormalsPath = filterArgs.value<DataPath>(k_FaceNormals_Key);
  auto surfaceMeshTriangleCentroidsPath = filterArgs.value<DataPath>(k_FaceCentroids_Key);

  auto useNormalsForCurveFitting = filterArgs.value<bool>(k_UseFaceNormals_Key);
  auto surfaceMeshPrincipalCurvature1sPath = filterArgs.value<DataPath>(k_PrincipalCurvature1_Key);
  auto surfaceMeshPrincipalCurvature2sPath = filterArgs.value<DataPath>(k_PrincipalCurvature2_Key);
  auto surfaceMeshPrincipalDirection1sPath = filterArgs.value<DataPath>(k_PrincipalDirection1_Key);
  auto surfaceMeshPrincipalDirection2sPath = filterArgs.value<DataPath>(k_PrincipalDirection2_Key);
  auto surfaceMeshMeanCurvaturesPath = filterArgs.value<DataPath>(k_MeanCurvature_Key);
  auto surfaceMeshGaussianCurvaturesPath = filterArgs.value<DataPath>(k_GaussianCurvature_Key);
  auto surfaceMeshWeingartenMatrixPath = filterArgs.value<DataPath>(k_WeingartenMatrix_Key);

  // Get DataObjects
  auto* surfaceMeshPrincipalCurvature1sArray = dataStructure.getDataAs<Float64Array>(surfaceMeshPrincipalCurvature1sPath);
  auto* surfaceMeshPrincipalCurvature2sArray = dataStructure.getDataAs<Float64Array>(surfaceMeshPrincipalCurvature2sPath);
  auto* surfaceMeshPrincipalDirection1sArray = dataStructure.getDataAs<Float64Array>(surfaceMeshPrincipalDirection1sPath);
  auto* surfaceMeshPrincipalDirection2sArray = dataStructure.getDataAs<Float64Array>(surfaceMeshPrincipalDirection2sPath);
  auto* surfaceMeshMeanCurvaturesArray = dataStructure.getDataAs<Float64Array>(surfaceMeshMeanCurvaturesPath);
  auto* surfaceMeshGaussianCurvaturesArray = dataStructure.getDataAs<Float64Array>(surfaceMeshGaussianCurvaturesPath);
  auto* surfaceMeshFaceLabelsArray = dataStructure.getDataAs<Int32Array>(surfaceMeshFaceLabelsPath);
  auto* surfaceMeshFaceNormalsArray = dataStructure.getDataAs<Float64Array>(surfaceMeshFaceNormalsPath);
  auto* surfaceMeshTriangleCentroidsArray = dataStructure.getDataAs<Float64Array>(surfaceMeshTriangleCentroidsPath);
  auto* surfaceMeshWeingartenMatrixArray = dataStructure.getDataAs<Float64Array>(surfaceMeshWeingartenMatrixPath);

  TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(triangleGeomPath);
  Int32Array* surfaceMeshFeatureFaceIdsArray = dataStructure.getDataAs<Int32Array>(surfaceMeshFeatureFaceIdsPath);
  auto& surfaceMeshFeatureFaceIds = surfaceMeshFeatureFaceIdsArray->getDataStoreRef();

  // Algorithm
  int32 totalFeatureFaces = -1;
  int32 completedFeatureFaces = -1;

  // Just to double check we have everything.
  int64 numTriangles = triangleGeom->getNumberOfFaces();
  int32 totalTriangles = numTriangles;

  // Make sure the Face Connectivity is created because the FindNRing algorithm needs this and will
  // assert if the data is NOT in the SurfaceMesh Data Container
  const IGeometry::ElementDynamicList* vertLinks = triangleGeom->getElementsContainingVert();
  if(nullptr == vertLinks)
  {
    triangleGeom->findElementsContainingVert();
  }

  // get the QMap from the SharedFeatureFaces filter
  SharedFeatureFaces_t sharedFeatureFaces;

  int32_t maxFaceId = 0;
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    if(surfaceMeshFeatureFaceIds[t] > maxFaceId)
    {
      maxFaceId = surfaceMeshFeatureFaceIds[t];
    }
  }

  std::vector<int32_t> faceSizes(maxFaceId + 1, 0);
  // Loop through all the Triangles and assign each one to a unique Feature Face Id.
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    faceSizes[surfaceMeshFeatureFaceIds[t]]++;
  }

  // Allocate all the vectors that we need
  for(size_t iter = 0; iter < faceSizes.size(); ++iter)
  {
    FaceIds_t v;
    v.reserve(faceSizes[iter]);
    sharedFeatureFaces[iter] = v;
  }

  // Loop through all the Triangles and assign each one to a unique Feature Face Id.
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    sharedFeatureFaces[surfaceMeshFeatureFaceIds[t]].push_back(t);
  }

  totalFeatureFaces = sharedFeatureFaces.size();
  completedFeatureFaces = 0;
  totalTriangles = numTriangles;
  std::string ss;

/*********************************
 * We are going to specfically invoke TBB directly instead of using ParallelTaskAlgorithm since we can just queue up all
 * the tasks while the first tasks start up. TBB will then grab a new task from it's own queue to work on it up to the
 * number of hardware limit of the machine. If we use ParallelTaskAlgorithm then at most we can only work on the number
 * of tasks that equal the number of cores on the machine. We spend a lot of overhead spinning up new threads to do each
 * task. This takes much longer (3x longer) in some cases
 */
#ifdef COMPLEX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> g(new tbb::task_group);
  ss = fmt::format("Adding {} Feature Faces to the work queue....", maxFaceId);
  messageHandler(ss);
#else

#endif

  // typedef here for conveneince
  typedef SharedFeatureFaces_t::iterator SharedFeatureFaceIterator_t;
  for(SharedFeatureFaceIterator_t iter = sharedFeatureFaces.begin(); iter != sharedFeatureFaces.end(); ++iter)
  {
    FaceIds_t& triangleIds = (*iter).second;

    CalculateTriangleGroupCurvatures func(nRingCount, triangleIds, useNormalsForCurveFitting, surfaceMeshPrincipalCurvature1sArray, surfaceMeshPrincipalCurvature2sArray,
                                          surfaceMeshPrincipalDirection1sArray, surfaceMeshPrincipalDirection2sArray, surfaceMeshGaussianCurvaturesArray, surfaceMeshMeanCurvaturesArray,
                                          surfaceMeshWeingartenMatrixArray, triangleGeom, surfaceMeshFaceLabelsArray, surfaceMeshFaceNormalsArray, surfaceMeshTriangleCentroidsArray, messageHandler,
                                          shouldCancel);

#ifdef COMPLEX_ENABLE_MULTICORE
    {
      g->run(func);
    }
#else
    ss = fmt::format("Working on Face Id {}/{}", std::to_string((*iter).first), std::to_string(maxFaceId));
    messageHandler(ss);
    {
      func();
    }
#endif
  }

#ifdef COMPLEX_ENABLE_MULTICORE
  ss = fmt::format("Waiting on computations to complete.");
  messageHandler(ss);
  g->wait(); // Wait for all the threads to complete before moving on.
#endif

  // return FillBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
  return {};
}
} // namespace complex
