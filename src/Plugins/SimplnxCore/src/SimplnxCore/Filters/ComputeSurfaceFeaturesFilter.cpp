#include "ComputeSurfaceFeaturesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeSurfaceFeatures.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeSurfaceFeaturesFilter::name() const
{
  return FilterTraits<ComputeSurfaceFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeSurfaceFeaturesFilter::className() const
{
  return FilterTraits<ComputeSurfaceFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeSurfaceFeaturesFilter::uuid() const
{
  return FilterTraits<ComputeSurfaceFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeSurfaceFeaturesFilter::humanName() const
{
  return "Compute Surface Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeSurfaceFeaturesFilter::defaultTags() const
{
  return {className(), "Generic", "Spatial", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeSurfaceFeaturesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_MarkFeature0Neighbors, "Mark Feature 0 Neighbors",
                                                "Marks features that are neighbors with feature 0.  If this option is off, only features that reside on the edge of the geometry will be marked.",
                                                true));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_FeatureGeometryPath_Key, "Feature Geometry", "The geometry in which to find surface features", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Cell Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                                    "The path to the cell feature attribute matrix associated with the input feature ids array", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceFeaturesArrayName_Key, "Surface Features",
                                                          "The created surface features array. Flag of 1 if Feature touches an outer surface or of 0 if it does not", "SurfaceFeatures"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeSurfaceFeaturesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeSurfaceFeaturesFilter::clone() const
{
  return std::make_unique<ComputeSurfaceFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeSurfaceFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pFeatureGeometryPathValue = filterArgs.value<DataPath>(k_FeatureGeometryPath_Key);
  auto pCellFeaturesAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pSurfaceFeaturesArrayNameValue = filterArgs.value<std::string>(k_SurfaceFeaturesArrayName_Key);

  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  usize geometryDimensionality = featureGeometry.getDimensionality();
  if(geometryDimensionality != 3 && geometryDimensionality != 2)
  {
    return {MakeErrorResult<OutputActions>(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()))};
  }

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  ShapeType tupleDims = std::vector<usize>{1};
  if(const auto& surfaceFeaturesParent = dataStructure.getDataAs<AttributeMatrix>(pCellFeaturesAttributeMatrixPathValue); surfaceFeaturesParent != nullptr)
  {
    tupleDims = surfaceFeaturesParent->getShape();
  }

  auto createSurfaceFeaturesAction = std::make_unique<CreateArrayAction>(
      DataType::uint8, tupleDims, std::vector<usize>{1}, pCellFeaturesAttributeMatrixPathValue.createChildPath(pSurfaceFeaturesArrayNameValue), CreateArrayAction::k_DefaultDataFormat, "0");
  resultOutputActions.value().appendAction(std::move(createSurfaceFeaturesAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeSurfaceFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeSurfaceFeaturesInputValues inputValues;
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_FeatureGeometryPath_Key);
  inputValues.MarkFeature0Neighbors = filterArgs.value<BoolParameter::ValueType>(k_MarkFeature0Neighbors);
  inputValues.SurfaceFeaturesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SurfaceFeaturesArrayName_Key);
  return ComputeSurfaceFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_SurfaceFeaturesArrayPathKey = "SurfaceFeaturesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeSurfaceFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeSurfaceFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_SurfaceFeaturesArrayPathKey, k_SurfaceFeaturesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
