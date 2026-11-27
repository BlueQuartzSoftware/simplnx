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

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_MarkFeature0Neighbors, "Mark Feature 0 Neighbors",
                                                "Marks features that are neighbors with feature 0.  If this option is off, only features that reside on the edge of the geometry will be marked.",
                                                true));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_FeatureGeometryPath_Key, "Feature Geometry", "The geometry in which to find surface features", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
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
IFilter::UniquePointerType ComputeSurfaceFeaturesFilter::clone() const
{
  return std::make_unique<ComputeSurfaceFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeSurfaceFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                     const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pFeatureGeometryPathValue = filterArgs.value<DataPath>(k_FeatureGeometryPath_Key);
  auto pCellFeaturesAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pSurfaceFeaturesArrayNameValue = filterArgs.value<std::string>(k_SurfaceFeaturesArrayName_Key);

  auto tupleCountResult = ValidateCellTupleCount(dataStructure, pFeatureGeometryPathValue, {filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key)}, -1099);
  if(tupleCountResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleCountResult), {})};
  }

  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  usize geometryDimensionality = featureGeometry.getDimensionality();
  if(geometryDimensionality != 3 && geometryDimensionality != 2)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()))};
  }

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  ShapeType tupleDims = std::vector<usize>{1};
  if(const auto& surfaceFeaturesParent = dataStructure.getDataAs<AttributeMatrix>(pCellFeaturesAttributeMatrixPathValue); surfaceFeaturesParent != nullptr)
  {
    tupleDims = surfaceFeaturesParent->getShape();
  }

  auto createSurfaceFeaturesAction =
      std::make_unique<CreateArrayAction>(DataType::uint8, tupleDims, std::vector<usize>{1}, pCellFeaturesAttributeMatrixPathValue.createChildPath(pSurfaceFeaturesArrayNameValue), "", "0");
  resultOutputActions.value().appendAction(std::move(createSurfaceFeaturesAction));

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeSurfaceFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeSurfaceFeaturesInputValues inputValues;
  inputValues.feature_attribute_matrix_path = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.feature_ids_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  inputValues.input_image_geometry_path = filterArgs.value<GeometrySelectionParameter::ValueType>(k_FeatureGeometryPath_Key);
  inputValues.mark_feature0_neighbors = filterArgs.value<BoolParameter::ValueType>(k_MarkFeature0Neighbors);
  inputValues.surface_features_array_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_SurfaceFeaturesArrayName_Key);
  return ComputeSurfaceFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_SurfaceFeaturesArrayPathKey = "SurfaceFeaturesArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> ComputeSurfaceFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeSurfaceFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureGeometryPath_Key));
  results.push_back(
      // The legacy output path supplies the feature AttributeMatrix, not the Feature IDs parent.
      simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_SurfaceFeaturesArrayPathKey, k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, simpl::k_SurfaceFeaturesArrayPathKey,
                                                                                                                                    k_SurfaceFeaturesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
