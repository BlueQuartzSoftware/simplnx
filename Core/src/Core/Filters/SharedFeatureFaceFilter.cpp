#include "SharedFeatureFaceFilter.hpp"

#include "Core/Filters/Algorithms/SharedFeatureFace.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

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
  return {"#Surface Meshing", "#Connectivity Arrangement", "#SurfaceMesh"};
}

//------------------------------------------------------------------------------
Parameters SharedFeatureFaceFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Required Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}, ArraySelectionParameter::AllowedTypes{complex::DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Created Face Data Arrays"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureFaceIdsArrayName_Key, "Feature Face Ids", "", "SharedFeatureFaceId"));

  params.insertSeparator(Parameters::Separator{"Created Face Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_GrainBoundaryAttributeMatrixName_Key, "Face Feature Attribute Matrix", "", "SharedFeatureFace"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureFaceLabelsArrayName_Key, "Feature Face Labels", "", "FaceLabels"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureNumTrianglesArrayName_Key, "Feature Number of Triangles", "", "NumTriangles"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SharedFeatureFaceFilter::clone() const
{
  return std::make_unique<SharedFeatureFaceFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SharedFeatureFaceFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto triangleGeometryPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  auto pFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);

  auto featureFaceIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureFaceIdsArrayName_Key);
  auto grainBoundaryAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_GrainBoundaryAttributeMatrixName_Key);
  auto featureNumTrianglesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureNumTrianglesArrayName_Key);
  auto featureFaceLabelsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureFaceLabelsArrayName_Key);

  PreflightResult preflightResult;
  const std::vector<size_t> tDims = {1};

  complex::Result<OutputActions> resultOutputActions;

  // Create the Face Data/FeatureIds Array
  {
    const auto* faceLabelDataArray = dataStructure.getDataAs<Int32Array>(pFaceLabelsArrayPathValue);
    if(nullptr == faceLabelDataArray)
    {
      return IFilter::MakePreflightErrorResult(-12901, fmt::format("Face Labels does not exist at path '{}' or the path does not point to an Int32 Array", pFaceLabelsArrayPathValue.toString()));
    }
    auto tupleShape = faceLabelDataArray->getTupleShape();
    auto faceDataGroupPathVector = pFaceLabelsArrayPathValue.getPathVector();
    faceDataGroupPathVector.pop_back();
    DataPath faceDataGroupPath = DataPath(faceDataGroupPathVector);

    DataPath createdArrayPath = faceDataGroupPath.createChildPath(featureFaceIdsArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::int32, faceLabelDataArray->getTupleShape(), std::vector<usize>{1}, createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Create the new Feature Attribute Matrix
  DataPath grainBoundaryAttributeMatrixPath = triangleGeometryPath.createChildPath(grainBoundaryAttributeMatrixName);
  resultOutputActions.value().actions.push_back(std::make_unique<CreateAttributeMatrixAction>(grainBoundaryAttributeMatrixPath, tDims));

  // Create the Feature Num Triangles Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureNumTrianglesArrayName_Key);
    DataPath createdArrayPath = grainBoundaryAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::int32, tDims, std::vector<usize>{1}, createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Create the Feature Face Labels Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureFaceLabelsArrayName_Key);
    DataPath createdArrayPath = grainBoundaryAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::int32, tDims, std::vector<usize>{2}, createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // No preflight updated values are generated in this filter
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> SharedFeatureFaceFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  SharedFeatureFaceInputValues inputValues;
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);

  auto faceDataGroupPathVector = inputValues.FaceLabelsArrayPath.getPathVector();
  faceDataGroupPathVector.pop_back();
  auto featureFaceIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureFaceIdsArrayName_Key);
  faceDataGroupPathVector.push_back(featureFaceIdsArrayName);
  inputValues.FeatureFaceIdsArrayPath = DataPath(faceDataGroupPathVector);

  auto grainBoundaryAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_GrainBoundaryAttributeMatrixName_Key);
  inputValues.GrainBoundaryAttributeMatrixPath = inputValues.TriangleGeometryPath.createChildPath(grainBoundaryAttributeMatrixName);

  auto featureNumTrianglesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureNumTrianglesArrayName_Key);
  inputValues.FeatureFaceNumTrianglesArrayPath = inputValues.GrainBoundaryAttributeMatrixPath.createChildPath(featureNumTrianglesArrayName);

  auto featureFaceLabelsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureFaceLabelsArrayName_Key);
  inputValues.FeatureFaceLabelsArrayPath = inputValues.GrainBoundaryAttributeMatrixPath.createChildPath(featureFaceLabelsArrayName);

  return SharedFeatureFace(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
