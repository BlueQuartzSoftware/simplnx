#include "ComputeFeatureBoundsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureBounds.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeatureBoundsFilter::name() const
{
  return FilterTraits<ComputeFeatureBoundsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureBoundsFilter::className() const
{
  return FilterTraits<ComputeFeatureBoundsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureBoundsFilter::uuid() const
{
  return FilterTraits<ComputeFeatureBoundsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureBoundsFilter::humanName() const
{
  return "Compute Feature Bounding Boxes";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureBoundsFilter::defaultTags() const
{
  return {className(), "Feature", "Statistics"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureBoundsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_OutputType_Key, "Output Array(s) Type", "If split two three component arrays will be created (Max & Min). If unified one six component array will be created with the XYZXYZ MinMax scheme",
      to_underlying(ComputeFeatureBounds::OutputDataType::Split), ChoicesParameter::Choices{"Split", "Unified"})); // sequence dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "The DataPath to the DataArray that specifies which feature each point belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<GeometrySelectionParameter>(
      k_SelectedGeometryPath_Key, "Selected Geometry", "The DataPath to the Geometry that contains the points/edges/faces for the geometry", DataPath{},
      GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Image, IGeometry::Type::Triangle, IGeometry::Type::Quad}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAMPath_Key, "Feature Data Attribute Matrix",
                                                                    "The DataPath to the Feature Data Attribute Matrix, array(s) will be created here", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_MinArrayName_Key, "Lower Bound Points Array Name",
                                                          "The name of the array containing the min/lower point of the bounding box for each feature", "Feature Lower Bounds"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MaxArrayName_Key, "Upper Bound Points Array Name",
                                                          "The name of the array containing the max/upper point of the bounding box for each feature", "Feature Upper Bounds"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_UnifiedArrayName_Key, "Unified Bounds Array Name",
                                                          "The name of the array containing the min and max point of the bounding box for each feature", "Feature Bounds"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputType_Key, k_MinArrayName_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
  params.linkParameters(k_OutputType_Key, k_MaxArrayName_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Split)));
  params.linkParameters(k_OutputType_Key, k_UnifiedArrayName_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeFeatureBounds::OutputDataType::Unified)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeatureBoundsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureBoundsFilter::clone() const
{
  return std::make_unique<ComputeFeatureBoundsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureBoundsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  auto pFeatureAMPathValue = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAMPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsArrayPath_Key);
  auto pSelectedGeomPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedGeometryPath_Key);

  const auto& geom = dataStructure.getDataRefAs<IGeometry>(pSelectedGeomPathValue);

  usize expectedFeatureSize = geom.getNumberOfCells();

  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  if(featureIds.getNumberOfTuples() != expectedFeatureSize)
  {
    return MakePreflightErrorResult(-89474, fmt::format("Expected Feature Ids size: {} | Actual Feature Ids size: {} | Feature Ids should be equivalent to the number of {}.", expectedFeatureSize,
                                                        featureIds.getNumberOfTuples(), targetStr));
  }

  nx::core::Result<OutputActions> resultOutputActions;
  auto* featureAM = dataStructure.getDataAs<AttributeMatrix>(pFeatureAMPathValue);
  if(featureAM == nullptr)
  {
    return MakePreflightErrorResult(-89470, fmt::format("Object at path {} must be a valid Attribute Matrix", pFeatureAMPathValue.toString()));
  }
  AttributeMatrix::ShapeType tupleShape = featureAM->getShape();
  switch(static_cast<ComputeFeatureBounds::OutputDataType>(pOutputTypeValue))
  {
  case ComputeFeatureBounds::OutputDataType::Split: {
    {
      auto minPath = pFeatureAMPathValue.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MinArrayName_Key));
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, minPath);
      resultOutputActions.value().appendAction(std::move(createAction));
    }
    {
      auto maxPath = pFeatureAMPathValue.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MaxArrayName_Key));
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, maxPath);
      resultOutputActions.value().appendAction(std::move(createAction));
    }
    break;
  }
  case ComputeFeatureBounds::OutputDataType::Unified: {
    {
      auto unifiedPath = pFeatureAMPathValue.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_UnifiedArrayName_Key));
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{6}, unifiedPath);
      resultOutputActions.value().appendAction(std::move(createAction));
    }
    break;
  }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureBoundsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeFeatureBoundsInputValues inputValues;

  inputValues.OutputType = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  inputValues.GeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedGeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureAMPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAMPath_Key);
  inputValues.MinArrayPath = inputValues.FeatureAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MinArrayName_Key));
  inputValues.MaxArrayPath = inputValues.FeatureAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MaxArrayName_Key));
  inputValues.UnifiedArrayPath = inputValues.FeatureAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_UnifiedArrayName_Key));

  return ComputeFeatureBounds(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
