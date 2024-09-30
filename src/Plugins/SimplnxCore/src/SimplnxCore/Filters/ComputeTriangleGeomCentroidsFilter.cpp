#include "ComputeTriangleGeomCentroidsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeTriangleGeomCentroids.hpp"

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
std::string ComputeTriangleGeomCentroidsFilter::name() const
{
  return FilterTraits<ComputeTriangleGeomCentroidsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeTriangleGeomCentroidsFilter::className() const
{
  return FilterTraits<ComputeTriangleGeomCentroidsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeTriangleGeomCentroidsFilter::uuid() const
{
  return FilterTraits<ComputeTriangleGeomCentroidsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeTriangleGeomCentroidsFilter::humanName() const
{
  return "Compute Feature Centroids from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeTriangleGeomCentroidsFilter::defaultTags() const
{
  return {className(), "Generic", "Morphological", "SurfaceMesh", "Statistics", "Triangle", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeTriangleGeomCentroidsFilter::parameters() const
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
                                                              "The DataPath to the AttributeMatrix that holds feature data for the faces", DataPath({"TriangleDataContainer", "Face Feature Data"}),
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insertSeparator(Parameters::Separator{"Output Face Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CentroidsArrayName_Key, "Calculated Centroids", "Centroid values created in the Face Feature Data", "Centroids"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeTriangleGeomCentroidsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeTriangleGeomCentroidsFilter::clone() const
{
  return std::make_unique<ComputeTriangleGeomCentroidsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeTriangleGeomCentroidsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pFaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  auto pFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  // Ensure the Face Feature Attribute Matrix is really an AttributeMatrix
  const auto* featureAttrMatrix = dataStructure.getDataAs<AttributeMatrix>(pFeatureAttributeMatrixPath);
  if(featureAttrMatrix == nullptr)
  {
    return IFilter::MakePreflightErrorResult(
        -12901, fmt::format("Feature AttributeMatrix does not exist at path '{}' or the path does not point to an AttributeMatrix.", pFeatureAttributeMatrixPath.toString()));
  }

  // Create the Centroids Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CentroidsArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{3}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // No preflight updated values are generated in this filter
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeTriangleGeomCentroidsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  ComputeTriangleGeomCentroidsInputValues inputValues;
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);

  auto volumesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_CentroidsArrayName_Key);
  inputValues.CentroidsArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(volumesArrayNameValue);

  return ComputeTriangleGeomCentroids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
