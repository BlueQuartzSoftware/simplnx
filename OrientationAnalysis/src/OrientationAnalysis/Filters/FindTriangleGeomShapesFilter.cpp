#include "FindTriangleGeomShapesFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindTriangleGeomShapes.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindTriangleGeomShapesFilter::name() const
{
  return FilterTraits<FindTriangleGeomShapesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomShapesFilter::className() const
{
  return FilterTraits<FindTriangleGeomShapesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindTriangleGeomShapesFilter::uuid() const
{
  return FilterTraits<FindTriangleGeomShapesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomShapesFilter::humanName() const
{
  return "Find Feature Shapes from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindTriangleGeomShapesFilter::defaultTags() const
{
  return {"#Statistics", "#Morphological", "#SurfaceMesh"};
}

//------------------------------------------------------------------------------
Parameters FindTriangleGeomShapesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Required Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}, ArraySelectionParameter::AllowedTypes{complex::DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Required Face Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixName_Key, "Face Feature Attribute Matrix", "", DataPath({"TriangleDataContainer", "FaceFeatureData"}),
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Face Feature Centroids", "", DataPath({"FaceFeatureData", "Centroids"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VolumesArrayPath_Key, "Face Feature Volumes", "", DataPath({"FaceFeatureData", "Volumes"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}));

  params.insertSeparator(Parameters::Separator{"Created Face Feature Data Arrays"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_Omega3sArrayName_Key, "Omega3s", "", "Omega3s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AxisLengthsArrayName_Key, "Axis Lengths", "", "AxisLengths"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AxisEulerAnglesArrayName_Key, "Axis Euler Angles", "", "AxisEulerAngles"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AspectRatiosArrayName_Key, "Aspect Ratios", "", "AspectRatios"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindTriangleGeomShapesFilter::clone() const
{
  return std::make_unique<FindTriangleGeomShapesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindTriangleGeomShapesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{

  auto pFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  auto pFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pVolumesArrayPathValue = filterArgs.value<DataPath>(k_VolumesArrayPath_Key);
  auto omega3sArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_Omega3sArrayName_Key);
  auto axisLengthsArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName_Key);
  auto axisEulerAnglesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName_Key);
  auto aspectRatiosArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // Ensure the Face Feature Attribute Matrix is really an AttributeMatrix
  const auto* featureAttrMatrix = dataStructure.getDataAs<AttributeMatrix>(pFeatureAttributeMatrixPath);
  if(featureAttrMatrix == nullptr)
  {
    return IFilter::MakePreflightErrorResult(
        -12901, fmt::format("Feature AttributeMatrix does not exist at path '{}' or the path does not point to an AttributeMatrix.", pFeatureAttributeMatrixPath.toString()));
  }

  // Create the Omega3s Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_Omega3sArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{1}, createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Create the Axis Lengths Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{3}, createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // Create the Axis Euler Angles Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{3}, createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // Create the Aspect Ratios Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{2}, createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // No preflight updated values are generated in this filter
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindTriangleGeomShapesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  FindTriangleGeomShapesInputValues inputValues;
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.VolumesArrayPath = filterArgs.value<DataPath>(k_VolumesArrayPath_Key);

  auto omega3sArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_Omega3sArrayName_Key);
  auto axisLengthsArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName_Key);
  auto axisEulerAnglesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName_Key);
  auto aspectRatiosArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName_Key);

  inputValues.Omega3sArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(omega3sArrayNameValue);
  inputValues.AxisLengthsArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(axisLengthsArrayNameValue);
  inputValues.AxisEulerAnglesArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(axisEulerAnglesArrayNameValue);
  inputValues.AspectRatiosArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(aspectRatiosArrayNameValue);

  return FindTriangleGeomShapes(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
