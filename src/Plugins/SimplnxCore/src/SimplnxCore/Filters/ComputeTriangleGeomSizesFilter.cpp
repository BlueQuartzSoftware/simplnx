#include "ComputeTriangleGeomSizesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeTriangleGeomSizes.hpp"

#include "simplnx/Common/Types.hpp"
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
std::string ComputeTriangleGeomSizesFilter::name() const
{
  return FilterTraits<ComputeTriangleGeomSizesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeTriangleGeomSizesFilter::className() const
{
  return FilterTraits<ComputeTriangleGeomSizesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeTriangleGeomSizesFilter::uuid() const
{
  return FilterTraits<ComputeTriangleGeomSizesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeTriangleGeomSizesFilter::humanName() const
{
  return "Compute Feature Volumes from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeTriangleGeomSizesFilter::defaultTags() const
{
  return {className(), "Generic", "Morphological", "SurfaceMesh", "Statistics", "Triangle"};
}

//------------------------------------------------------------------------------
Parameters ComputeTriangleGeomSizesFilter::parameters() const
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
  params.insert(std::make_unique<DataObjectNameParameter>(k_VolumesArrayName_Key, "Calculated Volumes", "Calculated volumes data created in the Face Feature Data Attribute Matrix", "Volumes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeTriangleGeomSizesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeTriangleGeomSizesFilter::clone() const
{
  return std::make_unique<ComputeTriangleGeomSizesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeTriangleGeomSizesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
        -12801, fmt::format("Feature AttributeMatrix does not exist at path '{}' or the path does not point to an AttributeMatrix.", pFeatureAttributeMatrixPath.toString()));
  }

  // Create the Volumes Output Array
  {
    auto createdArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VolumesArrayName_Key);
    DataPath createdArrayPath = pFeatureAttributeMatrixPath.createChildPath(createdArrayName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, featureAttrMatrix->getShape(), std::vector<usize>{1}, createdArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // No preflight updated values are generated in this filter
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeTriangleGeomSizesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  ComputeTriangleGeomSizesInputValues inputValues;
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);

  auto volumesArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_VolumesArrayName_Key);
  inputValues.VolumesArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(volumesArrayNameValue);

  return ComputeTriangleGeomSizes(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
