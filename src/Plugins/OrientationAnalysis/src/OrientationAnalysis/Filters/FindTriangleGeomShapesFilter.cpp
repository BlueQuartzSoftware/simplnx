#include "FindTriangleGeomShapesFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/FindTriangleGeomShapes.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
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
  return {className(), "Statistics", "Morphological", "SurfaceMesh"};
}

//------------------------------------------------------------------------------
Parameters FindTriangleGeomShapesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Input Triangle Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FaceLabelsArrayPath_Key, "Face Labels", "The DataPath to the FaceLabels values.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Input Face Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Face Feature Attribute Matrix",
                                                              "The DataPath to the AttributeMatrix that holds feature data for the faces", DataPath({"TriangleDataContainer", "FaceFeatureData"}),
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Face Feature Centroids", "Input DataPath to the **Feature Centroids** for the face data",
                                                          DataPath({"FaceFeatureData", "Centroids"}), ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VolumesArrayPath_Key, "Face Feature Volumes", "Input DataPath to the **Feature Volumes** for the face data",
                                                          DataPath({"FaceFeatureData", "Volumes"}), ArraySelectionParameter::AllowedTypes{DataType::float32}));

  params.insertSeparator(Parameters::Separator{"Output Face Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_Omega3sArrayName_Key, "Omega3s", "The name of the DataArray that holds the calculated Omega3 values", "Omega3s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AxisLengthsArrayName_Key, "Axis Lengths", "The name of the DataArray that holds the calculated Axis Lengths values", "AxisLengths"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AxisEulerAnglesArrayName_Key, "Axis Euler Angles", "The name of the DataArray that holds the calculated Axis Euler Angles values",
                                                          "AxisEulerAngles"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AspectRatiosArrayName_Key, "Aspect Ratios", "The name of the DataArray that holds the calculated Aspect Ratios values", "AspectRatios"));

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
  auto pFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pVolumesArrayPathValue = filterArgs.value<DataPath>(k_VolumesArrayPath_Key);
  auto omega3sArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_Omega3sArrayName_Key);
  auto axisLengthsArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName_Key);
  auto axisEulerAnglesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName_Key);
  auto aspectRatiosArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

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
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{1}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Create the Axis Lengths Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisLengthsArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{3}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create the Axis Euler Angles Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AxisEulerAnglesArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{3}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  // Create the Aspect Ratios Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AspectRatiosArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{2}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
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
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
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
} // namespace nx::core
