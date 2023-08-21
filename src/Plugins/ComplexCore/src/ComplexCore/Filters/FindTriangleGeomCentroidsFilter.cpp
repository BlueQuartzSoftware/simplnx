#include "FindTriangleGeomCentroidsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindTriangleGeomCentroids.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindTriangleGeomCentroidsFilter::name() const
{
  return FilterTraits<FindTriangleGeomCentroidsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomCentroidsFilter::className() const
{
  return FilterTraits<FindTriangleGeomCentroidsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindTriangleGeomCentroidsFilter::uuid() const
{
  return FilterTraits<FindTriangleGeomCentroidsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindTriangleGeomCentroidsFilter::humanName() const
{
  return "Find Feature Centroids from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindTriangleGeomCentroidsFilter::defaultTags() const
{
  return {className(), "Generic", "Morphological", "SurfaceMesh", "Statistics", "Triangle"};
}

//------------------------------------------------------------------------------
Parameters FindTriangleGeomCentroidsFilter::parameters() const
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
  params.insertSeparator(Parameters::Separator{"Created Face Feature Data Arrays"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CentroidsArrayName_Key, "Calculated Centroids", "Centroid values created in the Face Feature Data", "Centroids"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindTriangleGeomCentroidsFilter::clone() const
{
  return std::make_unique<FindTriangleGeomCentroidsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindTriangleGeomCentroidsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto pFaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  auto pFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

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
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{3}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // No preflight updated values are generated in this filter
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindTriangleGeomCentroidsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  FindTriangleGeomCentroidsInputValues inputValues;
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);

  auto volumesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_CentroidsArrayName_Key);
  inputValues.CentroidsArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(volumesArrayNameValue);

  return FindTriangleGeomCentroids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
