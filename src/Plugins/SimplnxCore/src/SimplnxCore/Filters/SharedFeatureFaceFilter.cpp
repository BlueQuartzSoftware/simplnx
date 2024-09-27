#include "SharedFeatureFaceFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/SharedFeatureFace.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
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
  return "Compute Triangle Face Ids";
}

//------------------------------------------------------------------------------
std::vector<std::string> SharedFeatureFaceFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Connectivity Arrangement", "SurfaceMesh", "Feature Face Ids"};
}

//------------------------------------------------------------------------------
Parameters SharedFeatureFaceFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeatures_Key, "Randomize Face IDs", "Specifies if feature IDs should be randomized. Can be helpful when visualizing the faces.", false));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "The DataPath to the FaceLabels values.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureFaceIdsArrayName_Key, "Feature Face Ids", "The name of the calculated Feature Face Ids DataArray", "SharedFeatureFaceId"));

  params.insertSeparator(Parameters::Separator{"Output Face Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_GrainBoundaryAttributeMatrixName_Key, "Face Feature Attribute Matrix",
                                                          "The name of the AttributeMatrix that holds the **Feature Face** data", "SharedFeatureFace"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_FeatureFaceLabelsArrayName_Key, "Feature Face Labels", "The name of the array that holds the calculated Feature Face Labels", "FaceLabels"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureNumTrianglesArrayName_Key, "Feature Number of Triangles",
                                                          "The name of the array that holds the calculated number of triangles for each feature face", "NumTriangles"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SharedFeatureFaceFilter::parametersVersion() const
{
  return 1;
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

  const std::vector<size_t> tDims = {1};

  nx::core::Result<OutputActions> resultOutputActions;

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
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, faceLabelDataArray->getTupleShape(), std::vector<usize>{1}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Create the new Feature Attribute Matrix
  DataPath grainBoundaryAttributeMatrixPath = triangleGeometryPath.createChildPath(grainBoundaryAttributeMatrixName);
  resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(grainBoundaryAttributeMatrixPath, tDims));

  // Create the Feature Num Triangles Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureNumTrianglesArrayName_Key);
    DataPath createdArrayPath = grainBoundaryAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tDims, std::vector<usize>{1}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Create the Feature Face Labels Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_FeatureFaceLabelsArrayName_Key);
    DataPath createdArrayPath = grainBoundaryAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int32, tDims, std::vector<usize>{2}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
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
  inputValues.ShouldRandomizeFeatureIds = filterArgs.value<bool>(k_RandomizeFeatures_Key);

  return SharedFeatureFace(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
